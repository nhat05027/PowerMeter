/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Include ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "calculate_task.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define NUM_CHANNELS 6
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static float custom_sqrt(float number);
// static uint32_t last_interrupt_time = 0;
static float signal_frequency = 0; // Signal frequency (Hz)

// Deprecated functions - use Calculate_All_Power_Parameters instead
static void Calculate_Active_Power_Deprecated(void);
static void Calculate_Reactive_Power_Deprecated(void);  
static void Calculate_Power_Factor_Deprecated(void);
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Public Function ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
float g_Signal_Frequency = 0;
float g_RMS_Value[6] = {0}; // RMS values for 6 channels
float g_Active_Power[3] = {0}; // Active Power for 3 phases (W)
float g_Reactive_Power[3] = {0}; // Reactive Power for 3 phases (VAR)
float g_Apparent_Power[3] = {0}; // Apparent Power for 3 phases (VA)
float g_Power_Factor[3] = {0}; // Power Factor for 3 phases

/* :::::::::: RMS Task Init :::::::: */
void Calculate_Task_Init(void)
{
    // Cấu hình GPIO ngắt
    // LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Kích hoạt clock cho GPIO
    // LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
    
    // Cấu hình pin GPIO
    // GPIO_InitStruct.Pin = GPIO_TRIGGER_PIN;
    // GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    // GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN; // Pull-down cho cạnh lên
    // LL_GPIO_Init(GPIO_TRIGGER_PORT, &GPIO_InitStruct);
    
    // // Cấu hình ngắt EXTI
    // LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
    // EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_1; // Thay X bằng số line tương ứng với pin
    // EXTI_InitStruct.LineCommand = ENABLE;
    // EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    // EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING; // Kích hoạt ngắt ở cạnh lên
    // LL_EXTI_Init(&EXTI_InitStruct);
    
    // // Kích hoạt ngắt trong NVIC
    // NVIC_SetPriority(EXTI0_1_IRQn, 0); // Thay X bằng số ngắt tương ứng
    // NVIC_EnableIRQ(EXTI0_1_IRQn);

    // Timer
    // Cấu hình timer 16-bit chạy ở 320kHz
    // LL_TIM_InitTypeDef TIM_InitStruct = {0};
    // LL_TIM_StructInit(&TIM_InitStruct);
    // TIM_InitStruct.Prescaler = (SystemCoreClock / TIMER_FREQ_HZ) - 1; // Prescaler = 100
    // TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    // LL_TIM_Init(TIMER_HANDLE , &TIM_InitStruct); // Thay TIMx bằng timer cụ thể (ví dụ: TIM2)
    LL_TIM_EnableCounter(TIMER_HANDLE);
}

void Calculate_Freq(void){
    // Tính tần số
        uint32_t current_time = LL_TIM_GetCounter(TIMER_HANDLE);
        signal_frequency = (float)TIMER_FREQ_HZ / current_time; // Tần số = 1/T
        g_Signal_Frequency = signal_frequency;
        TIMER_HANDLE -> CNT = 0;
}


void Calculate_Active_Power(void)
{
    // Wrapper function for backward compatibility
    Calculate_Active_Power_Deprecated();
}

void Calculate_Reactive_Power(void)
{
    // Wrapper function for backward compatibility  
    Calculate_Reactive_Power_Deprecated();
}

void Calculate_Power_Factor(void)
{
    // Wrapper function for backward compatibility
    Calculate_Power_Factor_Deprecated();
}

void Calculate_All_Power_Parameters(void)
{
    if (g_Sample_Count < MINIMUM_SAMPLE) return;

    // Reset tất cả giá trị power
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        g_Active_Power[phase] = 0.0f;
        g_Reactive_Power[phase] = 0.0f;
        g_Apparent_Power[phase] = 0.0f;
        g_Power_Factor[phase] = 0.0f;
    }

    // Tính RMS values và Active Power trong cùng một vòng lặp
    float inv_sample_count = 1.0f / g_Sample_Count; // Tính một lần để tái sử dụng
    
    // Bước 1: Tính RMS values
    for (uint8_t i = 0; i < NUM_CHANNELS; i++)
    {
        g_RMS_Value[i] = custom_sqrt((float)RMS_Sum_Square[i] * inv_sample_count);
    }
    
    // Bước 2: Tính Active Power - Tối ưu bằng cách tính trực tiếp
    for (uint16_t sample = 0; sample < g_Sample_Count; sample++)
    {
        // Phase L1: Voltage[4] * Current[2]
        g_Active_Power[0] += (float)g_ADC_Samples[3][sample] * (float)g_ADC_Samples[2][sample];
        
        // Phase L2: Voltage[5] * Current[1]  
        g_Active_Power[1] += (float)g_ADC_Samples[4][sample] * (float)g_ADC_Samples[1][sample];
        
        // Phase L3: Voltage[3] * Current[0]
        g_Active_Power[2] += (float)g_ADC_Samples[5][sample] * (float)g_ADC_Samples[0][sample];
    }
    
    // Tính trung bình Active Power và tính toán Apparent/Reactive/PF cùng lúc
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        // Hoàn thành tính Active Power
        g_Active_Power[phase] *= inv_sample_count;
        
        // Tính Apparent Power = V_rms * I_rms
        float v_rms, i_rms;
        switch(phase)
        {
            case 0: // L1
                v_rms = g_RMS_Value[3];
                i_rms = g_RMS_Value[2];
                break;
            case 1: // L2
                v_rms = g_RMS_Value[4];
                i_rms = g_RMS_Value[1];
                break;
            case 2: // L3
                v_rms = g_RMS_Value[5];
                i_rms = g_RMS_Value[0];
                break;
            default:
                v_rms = 0;
                i_rms = 0;
                break;
        }
        
        g_Apparent_Power[phase] = v_rms * i_rms;
        
        // Tính Reactive Power: Q = sqrt(S² - P²)
        float apparent_squared = g_Apparent_Power[phase] * g_Apparent_Power[phase];
        float active_squared = g_Active_Power[phase] * g_Active_Power[phase];
        
        if (apparent_squared >= active_squared)
        {
            g_Reactive_Power[phase] = custom_sqrt(apparent_squared - active_squared);
        }
        
        // Tính Power Factor = P / S
        if (g_Apparent_Power[phase] > 0.001f)
        {
            g_Power_Factor[phase] = g_Active_Power[phase] / g_Apparent_Power[phase];
            
            // Giới hạn PF trong khoảng [0, 1]
            if (g_Power_Factor[phase] > 1.0f)
                g_Power_Factor[phase] = 1.0f;
            else if (g_Power_Factor[phase] < 0.0f)
                g_Power_Factor[phase] = 0.0f;
        }
    }
    
    // Reset mảng mẫu và các biến lũy tiến
    ADC_Reset_Samples();
}



/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Function ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* :::::::::: Custom Square Root (Fast Inverse Square Root) ::::::::::::: */
static float custom_sqrt(float number)
{
    if (number <= 0) return 0;

    // Fast Inverse Square Root
    long i;
    float x2, y;
    const float threehalfs = 1.5f;

    x2 = number * 0.5f;
    y = number;
    i = *(long*)&y;                   // Lấy biểu diễn bit của float
    i = 0x5f3759df - (i >> 1);      // Magic number và dịch bit
    y = *(float*)&i;                  // Chuyển lại thành float
    y = y * (threehalfs - (x2 * y * y)); // Lặp Newton lần 1
    // y = y * (threehalfs - (x2 * y * y)); // Lặp Newton lần 2 (bỏ để nhanh hơn)

    return number * y; // sqrt(x) = x * (1/sqrt(x))
}

/* :::::::::: Deprecated Functions - Do not use directly ::::::::::::: */
static void Calculate_Active_Power_Deprecated(void)
{
    if (g_Sample_Count < MINIMUM_SAMPLE) return;
    
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        g_Active_Power[phase] = 0.0f;
    }
    
    for (uint16_t sample = 0; sample < g_Sample_Count; sample++)
    {
        g_Active_Power[0] += (float)g_ADC_Samples[4][sample] * g_ADC_Samples[2][sample];
        g_Active_Power[1] += (float)g_ADC_Samples[5][sample] * g_ADC_Samples[1][sample];
        g_Active_Power[2] += (float)g_ADC_Samples[3][sample] * g_ADC_Samples[0][sample];
    }
    
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        g_Active_Power[phase] = g_Active_Power[phase] / g_Sample_Count;
    }
}

static void Calculate_Reactive_Power_Deprecated(void)
{
    if (g_Sample_Count < MINIMUM_SAMPLE) return;
    
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        float v_rms, i_rms;
        
        switch(phase)
        {
            case 0: v_rms = g_RMS_Value[4]; i_rms = g_RMS_Value[2]; break;
            case 1: v_rms = g_RMS_Value[5]; i_rms = g_RMS_Value[1]; break;
            case 2: v_rms = g_RMS_Value[3]; i_rms = g_RMS_Value[0]; break;
            default: v_rms = 0; i_rms = 0; break;
        }
        
        g_Apparent_Power[phase] = v_rms * i_rms;
        
        float apparent_squared = g_Apparent_Power[phase] * g_Apparent_Power[phase];
        float active_squared = g_Active_Power[phase] * g_Active_Power[phase];
        
        if (apparent_squared >= active_squared)
        {
            g_Reactive_Power[phase] = custom_sqrt(apparent_squared - active_squared);
        }
        else
        {
            g_Reactive_Power[phase] = 0;
        }
    }
}

static void Calculate_Power_Factor_Deprecated(void)
{
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        if (g_Apparent_Power[phase] > 0.001f)
        {
            g_Power_Factor[phase] = g_Active_Power[phase] / g_Apparent_Power[phase];
            
            if (g_Power_Factor[phase] > 1.0f)
                g_Power_Factor[phase] = 1.0f;
            else if (g_Power_Factor[phase] < 0.0f)
                g_Power_Factor[phase] = 0.0f;
        }
        else
        {
            g_Power_Factor[phase] = 0.0f;
        }
    }
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End of the program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */