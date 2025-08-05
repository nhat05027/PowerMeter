/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Include~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "adc_task.h"
#include "app.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define ADC_CHANNEL_COUNT    7  // Bao gồm ADC0 (offset) và ADC1-6

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static uint8_t  ADC_channel_index = 0;
static uint16_t ADC0_Offset = 2048;

static bool is_ADC_read_completed = false;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Public Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
int32_t RMS_Sum_Square[6] = {0};
uint16_t g_Readvalue[ADC_CHANNEL_COUNT] = {0};
int16_t  g_ADC_Samples[6][MAX_SAMPLE_COUNT] = {0}; // Mảng lưu tối đa 80 mẫu/kênh, hỗ trợ số âm
uint16_t g_Sample_Count = 0; // Biến đếm số mẫu thực tế

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

        g_Readvalue[ADC_channel_index] = LL_ADC_REG_ReadConversionData12(ADC_FEEDBACK_HANDLE);

        // Lưu giá trị offset từ ADC0 (Neutral)
        if (ADC_channel_index == 0)
        {
            ADC0_Offset = (int16_t)g_Readvalue[0];
        }
        // Lưu giá trị đã trừ offset cho ADC1-6 và tính lũy tiến RMS
        else
        {
            if (g_Sample_Count < MAX_SAMPLE_COUNT) // Giới hạn kích thước mảng
            {
                uint8_t channel = ADC_channel_index - 1;
                // Lưu giá trị sau khi trừ offset, cho phép số âm
                int16_t sample = (int16_t)g_Readvalue[ADC_channel_index] - ADC0_Offset;
                g_ADC_Samples[channel][g_Sample_Count] = sample;

                // Cập nhật lũy tiến cho RMS
                RMS_Sum_Square[channel] += (int32_t)(sample) * (int32_t)(sample);
            }
        }

        if (ADC_channel_index >= 6)
        {
            ADC_channel_index = 0;
            g_Sample_Count++;
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
        RMS_Sum_Square[i] = 0;
    }
    g_Sample_Count = 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End of the program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */