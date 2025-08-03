/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Include~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "adc_task.h"
#include "app.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define ADC_CHANNEL_COUNT    7  // Bao gồm ADC0 (offset) và ADC1-6
// #define MAX_SAMPLE_COUNT     80 // Tối đa 100 mẫu mỗi kênh

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void ADC_Reset_Samples(void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static uint8_t  ADC_channel_index = 0;
static uint16_t ADC_Value[ADC_CHANNEL_COUNT] = {0};
static uint16_t ADC_Sample_Index = 0;
static uint16_t ADC0_Offset = 0;

static bool is_ADC_read_completed = false;

float RMS_Sum_Square[6] = {0}; // Lưu tổng bình phương lũy tiến

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Public Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
uint16_t g_Readvalue[ADC_CHANNEL_COUNT] = {0};
int16_t  g_ADC_Samples[6][MAX_SAMPLE_COUNT] = {0}; // Mảng lưu tối đa 100 mẫu/kênh, hỗ trợ số âm
uint16_t g_Sample_Count = 0; // Biến đếm số mẫu thực tế
float g_RMS_Value[6] = {0}; // Kết quả RMS

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Public Function ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* :::::::::: ADC Task Init :::::::: */
void ADC_Task_Init(uint32_t Sampling_Time)
{
    ADC_Init(ADC_FEEDBACK_HANDLE, Sampling_Time);

    LL_ADC_REG_SetSequencerChannels(ADC_FEEDBACK_HANDLE, ADC_VOLTAGE_L1 | ADC_VOLTAGE_L2 | ADC_VOLTAGE_L3 | ADC_CURRENT_L1 | ADC_CURRENT_L2 | ADC_CURRENT_L3 | ADC_NEUTRAL);
    LL_ADC_REG_SetSequencerDiscont(ADC_FEEDBACK_HANDLE, LL_ADC_REG_SEQ_DISCONT_1RANK);

    LL_ADC_EnableIT_EOC(ADC_FEEDBACK_HANDLE);

    LL_ADC_REG_StartConversion(ADC_FEEDBACK_HANDLE);
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

        ADC_Value[ADC_channel_index] = LL_ADC_REG_ReadConversionData12(ADC_FEEDBACK_HANDLE);
        g_Readvalue[ADC_channel_index] = 
            __LL_ADC_CALC_DATA_TO_VOLTAGE(3300, ADC_Value[ADC_channel_index], LL_ADC_RESOLUTION_12B);

        // Lưu giá trị offset từ ADC0 (Neutral)
        if (ADC_channel_index == 0)
        {
            ADC0_Offset = g_Readvalue[0];
        }
        // Lưu giá trị đã trừ offset cho ADC1-6 và tính lũy tiến RMS
        else
        {
            if (ADC_Sample_Index < MAX_SAMPLE_COUNT) // Giới hạn kích thước mảng
            {
                uint8_t channel = ADC_channel_index - 1;
                // Lưu giá trị sau khi trừ offset, cho phép số âm
                int16_t sample = (int16_t)g_Readvalue[ADC_channel_index] - (int16_t)ADC0_Offset;
                g_ADC_Samples[channel][ADC_Sample_Index] = sample;

                // Cập nhật lũy tiến cho RMS
                RMS_Sum_Square[channel] += (float)(sample * sample);
            }
        }

        if (ADC_channel_index >= (ADC_CHANNEL_COUNT - 1))
        {
            ADC_channel_index = 0;
            ADC_Sample_Index++;
            g_Sample_Count = ADC_Sample_Index; // Cập nhật số mẫu thực tế
        }
        else
        {
            ADC_channel_index++;
        }
    }
}

/* :::::::::: ADC Reset Samples ::::::::::::: */
void ADC_Reset_Samples(void)
{
    for (uint8_t i = 0; i < 6; i++)
    {
        for (uint16_t j = 0; j < MAX_SAMPLE_COUNT; j++)
        {
            g_ADC_Samples[i][j] = 0;
        }
        RMS_Sum_Square[i] = 0;
        // g_RMS_Value[i] = 0;
    }
    ADC_Sample_Index = 0;
    g_Sample_Count = 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End of the program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */