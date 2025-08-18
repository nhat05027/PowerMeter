/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Include ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "calculate_task.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define NUM_CHANNELS 6
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static float custom_sqrt(float number);
// static uint32_t last_interrupt_time = 0;
static float signal_frequency = 0; // Signal frequency (Hz)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Public Function ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
float g_Signal_Frequency = 0;
float g_RMS_Value[6] = {0}; // RMS values for 6 channels
float g_Active_Power[3] = {0}; // Active Power for 3 phases (W)
float g_Reactive_Power[3] = {0}; // Reactive Power for 3 phases (VAR)
float g_Apparent_Power[3] = {0}; // Apparent Power for 3 phases (VA)
float g_Power_Factor[3] = {0}; // Power Factor for 3 phases

// Phase status variables
uint8_t g_Phase_Active[3] = {0, 0, 0}; // Phase activity status (0=no voltage, 1=active)
uint8_t g_Phase_Leading[3] = {0, 0, 0}; // Phase relationship (0=lagging, 1=leading)

// Private variables for phase detection
static uint32_t phase_timeout_counter[3] = {0, 0, 0}; // Timeout counters for phase detection

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
        signal_frequency = TIMER_FREQ_HZ / current_time; // Tần số = 1/T
        g_Signal_Frequency = signal_frequency;
        TIMER_HANDLE -> CNT = 0;
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
    
    // g_RMS_Value[3] = (custom_sqrt((float)RMS_Sum_Square[3] * inv_sample_count) + Voltage_Beta_Coeff) * Voltage_Alpha_Coeff * Voltage_Transform_Ratio;
    // g_RMS_Value[0] = (custom_sqrt((float)RMS_Sum_Square[0] * inv_sample_count) + Current_Beta_Coeff) * Current_Alpha_Coeff * Current_Transform_Ratio;
    // g_RMS_Value[4] = (custom_sqrt((float)RMS_Sum_Square[4] * inv_sample_count) + Voltage_Beta_Coeff) * Voltage_Alpha_Coeff * Voltage_Transform_Ratio;
    // g_RMS_Value[1] = (custom_sqrt((float)RMS_Sum_Square[1] * inv_sample_count) + Current_Beta_Coeff) * Current_Alpha_Coeff * Current_Transform_Ratio;
    // g_RMS_Value[5] = (custom_sqrt((float)RMS_Sum_Square[5] * inv_sample_count) + Voltage_Beta_Coeff) * Voltage_Alpha_Coeff * Voltage_Transform_Ratio;
    // g_RMS_Value[2] = (custom_sqrt((float)RMS_Sum_Square[2] * inv_sample_count) + Current_Beta_Coeff) * Current_Alpha_Coeff * Current_Transform_Ratio;
    // Bước 1: Tính RMS values - chỉ tính cho phase active
    // L1 Phase (Voltage[3], Current[0])
    if (g_Phase_Active[0])
    {
        g_RMS_Value[3] = (custom_sqrt((float)RMS_Sum_Square[3] * inv_sample_count) + Voltage_Beta_Coeff) * Voltage_Alpha_Coeff * Voltage_Transform_Ratio;
        g_RMS_Value[0] = (custom_sqrt((float)RMS_Sum_Square[0] * inv_sample_count) + Current_Beta_Coeff) * Current_Alpha_Coeff * Current_Transform_Ratio;
    }
    else
    {
        g_RMS_Value[3] = 0.0f;
        g_RMS_Value[0] = 0.0f;
    }
    
    // L2 Phase (Voltage[4], Current[1])
    if (g_Phase_Active[1])
    {
        g_RMS_Value[4] = (custom_sqrt((float)RMS_Sum_Square[4] * inv_sample_count) + Voltage_Beta_Coeff) * Voltage_Alpha_Coeff * Voltage_Transform_Ratio;
        g_RMS_Value[1] = (custom_sqrt((float)RMS_Sum_Square[1] * inv_sample_count) + Current_Beta_Coeff) * Current_Alpha_Coeff * Current_Transform_Ratio;
    }
    else
    {
        g_RMS_Value[4] = 0.0f;
        g_RMS_Value[1] = 0.0f;
    }
    
    // L3 Phase (Voltage[5], Current[2])
    if (g_Phase_Active[2])
    {
        g_RMS_Value[5] = (custom_sqrt((float)RMS_Sum_Square[5] * inv_sample_count) + Voltage_Beta_Coeff) * Voltage_Alpha_Coeff * Voltage_Transform_Ratio;
        g_RMS_Value[2] = (custom_sqrt((float)RMS_Sum_Square[2] * inv_sample_count) + Current_Beta_Coeff) * Current_Alpha_Coeff * Current_Transform_Ratio;
    }
    else
    {
        g_RMS_Value[5] = 0.0f;
        g_RMS_Value[2] = 0.0f;
    }

    // Bước 2: Tính Active Power - chỉ tính cho phase active
    for (uint16_t sample = 0; sample < g_Sample_Count; sample++)
    {
        // Phase L1: Voltage[3] * Current[2] - chỉ tính nếu active
        if (g_Phase_Active[0])
            g_Active_Power[0] += (float)g_ADC_Samples[3][sample] * (float)g_ADC_Samples[0][sample];
        
        // Phase L2: Voltage[4] * Current[1] - chỉ tính nếu active
        if (g_Phase_Active[1])
            g_Active_Power[1] += (float)g_ADC_Samples[4][sample] * (float)g_ADC_Samples[1][sample];
        
        // Phase L3: Voltage[5] * Current[0] - chỉ tính nếu active
        if (g_Phase_Active[2])
            g_Active_Power[2] += (float)g_ADC_Samples[5][sample] * (float)g_ADC_Samples[2][sample];
    }
    
    // Tính trung bình Active Power và tính toán Apparent/Reactive/PF chỉ cho phase active
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        // Chỉ tính toán nếu phase đang active
        if (!g_Phase_Active[phase])
        {
            // Phase không active - đã reset giá trị ở đầu hàm
            continue;
        }
        
        // Hoàn thành tính Active Power
        g_Active_Power[phase] *= inv_sample_count * Voltage_Alpha_Coeff * Current_Alpha_Coeff; // Chia cho số mẫu và nhân với hệ số alpha
        
        // Tính Apparent Power = V_rms * I_rms
        float v_rms, i_rms;
        switch(phase)
        {
            case 0: // L1
                v_rms = g_RMS_Value[3];
                i_rms = g_RMS_Value[0];
                break;
            case 1: // L2
                v_rms = g_RMS_Value[4];
                i_rms = g_RMS_Value[1];
                break;
            case 2: // L3
                v_rms = g_RMS_Value[5];
                i_rms = g_RMS_Value[2];
                break;
            default:
                v_rms = 0;
                i_rms = 0;
                break;
        }
        
        g_Apparent_Power[phase] = v_rms * i_rms;
        
        // Tính Reactive Power: Q = sqrt(S² - P²) with sign based on phase relationship
        float apparent_squared = g_Apparent_Power[phase] * g_Apparent_Power[phase];
        float active_squared = g_Active_Power[phase] * g_Active_Power[phase];
        
        if (apparent_squared >= active_squared)
        {
            float reactive_magnitude = custom_sqrt(apparent_squared - active_squared);
            
            // Apply sign based on phase relationship
            // Leading current (capacitive load) = negative reactive power
            // Lagging current (inductive load) = positive reactive power
            g_Reactive_Power[phase] = g_Phase_Leading[phase] ? -reactive_magnitude : reactive_magnitude;
        }
        else
        {
            g_Reactive_Power[phase] = 0.0f;
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

/* :::::::::: Phase Detection Functions for GPIO ZCD Interrupts ::::::::::::: */
void Phase_L1_ZCD_Handler(void)
{
    // Mark phase as active
    g_Phase_Active[0] = 1;
    phase_timeout_counter[0] = 0; // Reset timeout counter
    
    // Detect current leading/lagging by checking current zero crossing timing
    // If current zero crosses within detection window after voltage zero crossing, current is lagging
    // If current zero crosses before voltage zero crossing, current is leading
    
    // Simple approximation: check current RMS value trend
    // If current is high when voltage crosses zero, assume leading
    // This is a simplified approach - for accurate detection, need separate current ZCD
    if (g_RMS_Value[2] > 1.0f) // L1 current channel
    {
        // Use instantaneous current sample to determine phase relationship
        int16_t current_sample = g_ADC_Samples[2][g_Sample_Count > 0 ? g_Sample_Count-1 : 0];
        g_Phase_Leading[0] = (current_sample > 0) ? 1 : 0; // Simplified phase detection
    }
}

void Phase_L2_ZCD_Handler(void)
{
    // Mark phase as active
    g_Phase_Active[1] = 1;
    phase_timeout_counter[1] = 0; // Reset timeout counter
    
    // Phase detection for L2
    if (g_RMS_Value[1] > 1.0f) // L2 current channel
    {
        int16_t current_sample = g_ADC_Samples[1][g_Sample_Count > 0 ? g_Sample_Count-1 : 0];
        g_Phase_Leading[1] = (current_sample > 0) ? 1 : 0;
    }
}

void Phase_L3_ZCD_Handler(void)
{
    // Mark phase as active
    g_Phase_Active[2] = 1;
    phase_timeout_counter[2] = 0; // Reset timeout counter
    
    // Phase detection for L3
    if (g_RMS_Value[0] > 1.0f) // L3 current channel
    {
        int16_t current_sample = g_ADC_Samples[0][g_Sample_Count > 0 ? g_Sample_Count-1 : 0];
        g_Phase_Leading[2] = (current_sample > 0) ? 1 : 0;
    }
}

/* :::::::::: Phase Status Management Functions ::::::::::::: */
void Reset_Phase_Status(void)
{
    for (uint8_t i = 0; i < 3; i++)
    {
        g_Phase_Active[i] = 0;
        g_Phase_Leading[i] = 0;
        phase_timeout_counter[i] = 0;
    }
}

uint8_t Get_Phase_Active_Status(uint8_t phase)
{
    if (phase < 3)
        return g_Phase_Active[phase];
    return 0;
}

uint8_t Get_Phase_Leading_Status(uint8_t phase)
{
    if (phase < 3)
        return g_Phase_Leading[phase];
    return 0;
}

/* :::::::::: Phase Timeout Check (call this periodically from scheduler) ::::::::::::: */
void Check_Phase_Timeouts(void *pvParameters)
{
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        phase_timeout_counter[phase]++;
        
        // If no zero crossing for PHASE_TIMEOUT_CYCLES, mark phase as inactive
        // This counter will be reset in ZCD handlers when phase is active
        if (phase_timeout_counter[phase] > PHASE_TIMEOUT_CYCLES)
        {
            g_Phase_Active[phase] = 0;
            
            // Clear all parameters for inactive phase
            switch(phase)
            {
                case 0: // L1
                    g_RMS_Value[3] = 0.0f; // L1 voltage
                    g_RMS_Value[0] = 0.0f; // L1 current
                    break;
                case 1: // L2
                    g_RMS_Value[4] = 0.0f; // L2 voltage
                    g_RMS_Value[1] = 0.0f; // L2 current
                    break;
                case 2: // L3
                    g_RMS_Value[5] = 0.0f; // L3 voltage
                    g_RMS_Value[2] = 0.0f; // L3 current
                    break;
            }
            
            g_Active_Power[phase] = 0.0f;
            g_Reactive_Power[phase] = 0.0f;
            g_Apparent_Power[phase] = 0.0f;
            g_Power_Factor[phase] = 0.0f;
        }
    }
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

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End of the program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */