/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Include~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "adc_task.h"
#include "app.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define ADC_CHANNEL_COUNT    7  // Bao gồm ADC0 (offset) và ADC1-6

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static uint8_t  ADC_current_phase = 1;       // Current phase (1-3)
static uint16_t ADC0_Offset = 2048;          // Current offset value
static uint8_t  reading_step = 0;            // 0=offset, 1=voltage, 2=current

static bool is_ADC_read_completed = false;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Public Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
int32_t RMS_Sum_Square[6] = {0};
uint16_t g_Readvalue[ADC_CHANNEL_COUNT] = {0};
int16_t  g_ADC_Samples[6][MAX_SAMPLE_COUNT] = {0}; // Mảng lưu tối đa 80 mẫu/kênh, hỗ trợ số âm
uint16_t g_Sample_Count = 0; // Biến đếm số mẫu thực tế

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static void ADC_Switch_Channel(uint8_t channel)
{
    uint32_t channel_mask;
    
    switch(channel)
    {
        case 0: channel_mask = ADC_NEUTRAL; break;       // Offset/neutral
        case 1: channel_mask = ADC_CURRENT_L1; break;    // Channel 1
        case 2: channel_mask = ADC_CURRENT_L2; break;    // Channel 2  
        case 3: channel_mask = ADC_CURRENT_L3; break;    // Channel 3
        case 4: channel_mask = ADC_VOLTAGE_L1; break;    // Channel 4
        case 5: channel_mask = ADC_VOLTAGE_L2; break;    // Channel 5
        case 6: channel_mask = ADC_VOLTAGE_L3; break;    // Channel 6
        default: channel_mask = ADC_NEUTRAL; break;
    }
    
    LL_ADC_REG_SetSequencerChannels(ADC_FEEDBACK_HANDLE, channel_mask);
}

static uint8_t Get_Voltage_Channel(uint8_t phase)
{
    // Phase 1 = L1 = Channel 4 (Voltage L1)
    // Phase 2 = L2 = Channel 5 (Voltage L2)  
    // Phase 3 = L3 = Channel 6 (Voltage L3)
    return 3 + phase; // phase 1→4, phase 2→5, phase 3→6
}

static uint8_t Get_Current_Channel(uint8_t phase)
{
    // Phase 1 = L1 = Channel 1 (Current L1)
    // Phase 2 = L2 = Channel 2 (Current L2)
    // Phase 3 = L3 = Channel 3 (Current L3)
    return phase; // phase 1→1, phase 2→2, phase 3→3
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Public Function ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* :::::::::: ADC Task Init :::::::: */
void ADC_Task_Init(uint32_t Sampling_Time)
{
    ADC_Init(ADC_FEEDBACK_HANDLE, Sampling_Time);

    // Start with neutral/offset channel
    ADC_Switch_Channel(0);
    LL_ADC_REG_SetSequencerDiscont(ADC_FEEDBACK_HANDLE, LL_ADC_REG_SEQ_DISCONT_1RANK);

    LL_ADC_EnableIT_EOC(ADC_FEEDBACK_HANDLE);

    LL_ADC_REG_StartConversion(ADC_FEEDBACK_HANDLE);
    
    // Initialize state
    reading_step = 0;        // Start with offset
    ADC_current_phase = 1;   // Start with phase 1
}

/* :::::::::: ADC Task ::::::::::::: */
void ADC_Task(void*)
{
    if (is_ADC_read_completed == true)
    {
        is_ADC_read_completed = false;

        LL_ADC_REG_StartConversion(ADC_FEEDBACK_HANDLE);
    }
}

/* :::::::::: ADC Interrupt Handler ::::::::::::: */
void ADC_Task_IRQHandler(void)
{
    if (LL_ADC_IsActiveFlag_EOC(ADC_FEEDBACK_HANDLE) == true)
    {
        is_ADC_read_completed = true;
        LL_ADC_ClearFlag_EOC(ADC_FEEDBACK_HANDLE);

        uint16_t adc_value = LL_ADC_REG_ReadConversionData12(ADC_FEEDBACK_HANDLE);

        switch (reading_step)
        {
            case 0: // Đọc offset
            {
                ADC0_Offset = adc_value;
                g_Readvalue[0] = adc_value;
                
                // Chuyển ngay sang đọc voltage của phase hiện tại
                uint8_t voltage_channel = Get_Voltage_Channel(ADC_current_phase);
                ADC_Switch_Channel(voltage_channel);
                reading_step = 1;
                
                // Tự động start conversion tiếp
                is_ADC_read_completed = false;
                LL_ADC_REG_StartConversion(ADC_FEEDBACK_HANDLE);
                break;
            }
            
            case 1: // Đọc voltage
            {
                uint8_t voltage_channel = Get_Voltage_Channel(ADC_current_phase);
                g_Readvalue[voltage_channel] = adc_value;
                
                if (g_Sample_Count < MAX_SAMPLE_COUNT)
                {
                    uint8_t channel_index = voltage_channel - 1; // Convert to 0-5 index
                    
                    // Tính differential: voltage - offset (vừa đọc)
                    int16_t sample = (int16_t)adc_value - (int16_t)ADC0_Offset;
                    g_ADC_Samples[channel_index][g_Sample_Count] = sample;
                    RMS_Sum_Square[channel_index] += (int32_t)sample * sample;
                }
                
                // Chuyển ngay sang đọc current của cùng phase
                uint8_t current_channel = Get_Current_Channel(ADC_current_phase);
                ADC_Switch_Channel(current_channel);
                reading_step = 2;
                
                // Tự động start conversion tiếp
                is_ADC_read_completed = false;
                LL_ADC_REG_StartConversion(ADC_FEEDBACK_HANDLE);
                break;
            }
            
            case 2: // Đọc current
            {
                uint8_t current_channel = Get_Current_Channel(ADC_current_phase);
                g_Readvalue[current_channel] = adc_value;
                
                if (g_Sample_Count < MAX_SAMPLE_COUNT)
                {
                    uint8_t channel_index = current_channel - 1; // Convert to 0-5 index
                    
                    // Tính differential: current - offset (cùng offset với voltage)
                    int16_t sample = (int16_t)adc_value - (int16_t)ADC0_Offset;
                    g_ADC_Samples[channel_index][g_Sample_Count] = sample;
                    RMS_Sum_Square[channel_index] += (int32_t)sample * sample;
                }
                
                // Hoàn thành một phase (offset + voltage + current)
                // Chuyển sang phase tiếp theo
                ADC_current_phase++;
                if (ADC_current_phase > 3)
                {
                    ADC_current_phase = 1;    // Reset về phase 1
                    g_Sample_Count++;         // Hoàn thành 1 cycle đầy đủ của tất cả 3 phases
                }
                
                // Chuẩn bị cho phase tiếp theo - bắt đầu với offset
                ADC_Switch_Channel(0);
                reading_step = 0;
                break;
            }
        }
    }
}

/* :::::::::: ADC Reset Samples ::::::::::::: */
void ADC_Reset_Samples(void)
{
    // Disable interrupt để tránh race condition
    LL_ADC_DisableIT_EOC(ADC_FEEDBACK_HANDLE);
    
    for (uint8_t i = 0; i < 6; i++)
    {
        RMS_Sum_Square[i] = 0;
    }
    g_Sample_Count = 0;
    ADC_current_phase = 1;
    reading_step = 0;
    
    // Switch về offset channel và re-enable interrupt
    ADC_Switch_Channel(0);
    LL_ADC_EnableIT_EOC(ADC_FEEDBACK_HANDLE);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End of the program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */