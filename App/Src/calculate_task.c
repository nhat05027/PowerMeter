/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Include ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "calculate_task.h"
#include <math.h>
#include <stddef.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define NUM_CHANNELS 6
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static float custom_sqrt(float number);
static void simple_dft(float* input, uint16_t N, uint8_t harmonic, float* magnitude);
static float calculate_phase_thd(uint8_t phase);
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

// THD analysis variables
float g_THD_Voltage[3] = {0.0f, 0.0f, 0.0f};     // THD for 3 phase voltages (%)
float g_Fundamental_Voltage[3] = {0.0f, 0.0f, 0.0f}; // Fundamental voltage component (V)
float g_Harmonic_Voltage[3][THD_HARMONICS_COUNT] = {0}; // Individual harmonics (V)

// Private variables for phase detection
static uint32_t phase_timeout_counter[3] = {0, 0, 0}; // Timeout counters for phase detection

/* :::::::::: RMS Task Init :::::::: */
void Calculate_Task_Init(void)
{
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
    
    g_RMS_Value[3] = (custom_sqrt((float)RMS_Sum_Square[3] * inv_sample_count) + Voltage_Beta_Coeff) * Voltage_Alpha_Coeff * Voltage_Transform_Ratio;
    g_RMS_Value[0] = (custom_sqrt((float)RMS_Sum_Square[0] * inv_sample_count) + Current_Beta_Coeff) * Current_Alpha_Coeff * Current_Transform_Ratio;
    g_RMS_Value[4] = (custom_sqrt((float)RMS_Sum_Square[4] * inv_sample_count) + Voltage_Beta_Coeff) * Voltage_Alpha_Coeff * Voltage_Transform_Ratio;
    g_RMS_Value[1] = (custom_sqrt((float)RMS_Sum_Square[1] * inv_sample_count) + Current_Beta_Coeff) * Current_Alpha_Coeff * Current_Transform_Ratio;
    g_RMS_Value[5] = (custom_sqrt((float)RMS_Sum_Square[5] * inv_sample_count) + Voltage_Beta_Coeff) * Voltage_Alpha_Coeff * Voltage_Transform_Ratio;
    g_RMS_Value[2] = (custom_sqrt((float)RMS_Sum_Square[2] * inv_sample_count) + Current_Beta_Coeff) * Current_Alpha_Coeff * Current_Transform_Ratio;
    // Bước 1: Tính RMS values - chỉ tính cho phase active
    // L1 Phase (Voltage[3], Current[0])
    // if (g_Phase_Active[0])
    // {
    //     g_RMS_Value[3] = (custom_sqrt((float)RMS_Sum_Square[3] * inv_sample_count) + Voltage_Beta_Coeff) * Voltage_Alpha_Coeff * Voltage_Transform_Ratio;
    //     g_RMS_Value[0] = (custom_sqrt((float)RMS_Sum_Square[0] * inv_sample_count) + Current_Beta_Coeff) * Current_Alpha_Coeff * Current_Transform_Ratio;
    // }
    // else
    // {
    //     g_RMS_Value[3] = 0.0f;
    //     g_RMS_Value[0] = 0.0f;
    // }
    
    // // L2 Phase (Voltage[4], Current[1])
    // if (g_Phase_Active[1])
    // {
    //     g_RMS_Value[4] = (custom_sqrt((float)RMS_Sum_Square[4] * inv_sample_count) + Voltage_Beta_Coeff) * Voltage_Alpha_Coeff * Voltage_Transform_Ratio;
    //     g_RMS_Value[1] = (custom_sqrt((float)RMS_Sum_Square[1] * inv_sample_count) + Current_Beta_Coeff) * Current_Alpha_Coeff * Current_Transform_Ratio;
    // }
    // else
    // {
    //     g_RMS_Value[4] = 0.0f;
    //     g_RMS_Value[1] = 0.0f;
    // }
    
    // // L3 Phase (Voltage[5], Current[2])
    // if (g_Phase_Active[2])
    // {
    //     g_RMS_Value[5] = (custom_sqrt((float)RMS_Sum_Square[5] * inv_sample_count) + Voltage_Beta_Coeff) * Voltage_Alpha_Coeff * Voltage_Transform_Ratio;
    //     g_RMS_Value[2] = (custom_sqrt((float)RMS_Sum_Square[2] * inv_sample_count) + Current_Beta_Coeff) * Current_Alpha_Coeff * Current_Transform_Ratio;
    // }
    // else
    // {
    //     g_RMS_Value[5] = 0.0f;
    //     g_RMS_Value[2] = 0.0f;
    // }

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
            // switch(phase)
            // {
            //     case 0: // L1
            //         g_RMS_Value[3] = 0.0f; // L1 voltage
            //         g_RMS_Value[0] = 0.0f; // L1 current
            //         break;
            //     case 1: // L2
            //         g_RMS_Value[4] = 0.0f; // L2 voltage
            //         g_RMS_Value[1] = 0.0f; // L2 current
            //         break;
            //     case 2: // L3
            //         g_RMS_Value[5] = 0.0f; // L3 voltage
            //         g_RMS_Value[2] = 0.0f; // L3 current
            //         break;
            // }
            
            g_Active_Power[phase] = 0.0f;
            g_Reactive_Power[phase] = 0.0f;
            g_Apparent_Power[phase] = 0.0f;
            g_Power_Factor[phase] = 0.0f;
        }
    }
}

/* :::::::::: THD Analysis Functions ::::::::::::: */
void Calculate_THD_Task(void *pvParameters)
{
    // Chỉ tính THD khi có đủ sample và tần số ổn định
    if (g_Sample_Count < THD_SAMPLES_PER_CYCLE || g_Signal_Frequency < 45.0f || g_Signal_Frequency > 65.0f)
    {
        return;
    }

    // Phân tích THD cho từng pha điện áp
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        if (g_Phase_Active[phase])
        {
            THD_Analyze_Voltage_Phase(phase);
        }
        else
        {
            // Reset THD data cho pha không hoạt động
            g_THD_Voltage[phase] = 0.0f;
            g_Fundamental_Voltage[phase] = 0.0f;
            for (uint8_t h = 0; h < THD_HARMONICS_COUNT; h++)
            {
                g_Harmonic_Voltage[phase][h] = 0.0f;
            }
        }
    }
}

void THD_Analyze_Voltage_Phase(uint8_t phase)
{
    if (phase >= 3) return;

    // Xác định channel điện áp tương ứng với phase
    uint8_t voltage_channel;
    switch(phase)
    {
        case 0: voltage_channel = 3; break; // L1 -> Channel 3
        case 1: voltage_channel = 4; break; // L2 -> Channel 4  
        case 2: voltage_channel = 5; break; // L3 -> Channel 5
        default: return;
    }

    // Lấy samples cho phân tích (sử dụng samples gần đây nhất)
    uint16_t samples_to_analyze = (g_Sample_Count < THD_SAMPLES_PER_CYCLE) ? g_Sample_Count : THD_SAMPLES_PER_CYCLE;
    uint16_t start_index = (g_Sample_Count > THD_SAMPLES_PER_CYCLE) ? (g_Sample_Count - THD_SAMPLES_PER_CYCLE) : 0;

    // Tính fundamental frequency component (harmonic 1)
    float fundamental_magnitude = 0.0f;
    simple_dft(&g_ADC_Samples[voltage_channel][start_index], samples_to_analyze, 1, &fundamental_magnitude);
    
    // Chuyển đổi từ ADC value sang voltage
    g_Fundamental_Voltage[phase] = (fundamental_magnitude + Voltage_Beta_Coeff) * Voltage_Alpha_Coeff * Voltage_Transform_Ratio;

    // Kiểm tra fundamental có đủ lớn để tính THD không
    if (g_Fundamental_Voltage[phase] < THD_MIN_FUNDAMENTAL)
    {
        g_THD_Voltage[phase] = 0.0f;
        return;
    }

    // Tính các harmonics từ 2 đến THD_HARMONICS_COUNT
    float harmonic_sum_squared = 0.0f;
    for (uint8_t h = 2; h <= THD_HARMONICS_COUNT; h++)
    {
        float harmonic_magnitude = 0.0f;
        simple_dft(&g_ADC_Samples[voltage_channel][start_index], samples_to_analyze, h, &harmonic_magnitude);
        
        // Chuyển đổi sang voltage và lưu trữ
        g_Harmonic_Voltage[phase][h-1] = (harmonic_magnitude + Voltage_Beta_Coeff) * Voltage_Alpha_Coeff * Voltage_Transform_Ratio;
        
        // Cộng dồn bình phương để tính THD
        harmonic_sum_squared += g_Harmonic_Voltage[phase][h-1] * g_Harmonic_Voltage[phase][h-1];
    }

    // Tính THD = sqrt(sum of harmonics²) / fundamental * 100%
    if (g_Fundamental_Voltage[phase] > 0.0f)
    {
        g_THD_Voltage[phase] = (custom_sqrt(harmonic_sum_squared) / g_Fundamental_Voltage[phase]) * 100.0f;
        
        // Giới hạn THD trong khoảng hợp lý
        if (g_THD_Voltage[phase] > 100.0f)
            g_THD_Voltage[phase] = 100.0f;
    }
}

void THD_Reset_Data(void)
{
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        g_THD_Voltage[phase] = 0.0f;
        g_Fundamental_Voltage[phase] = 0.0f;
        for (uint8_t h = 0; h < THD_HARMONICS_COUNT; h++)
        {
            g_Harmonic_Voltage[phase][h] = 0.0f;
        }
    }
}

float THD_Get_Voltage_THD(uint8_t phase)
{
    if (phase < 3)
        return g_THD_Voltage[phase];
    return 0.0f;
}

float THD_Get_Harmonic_Voltage(uint8_t phase, uint8_t harmonic)
{
    if (phase < 3 && harmonic > 0 && harmonic <= THD_HARMONICS_COUNT)
        return g_Harmonic_Voltage[phase][harmonic-1];
    return 0.0f;
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

/* :::::::::: Simple DFT for Harmonic Analysis ::::::::::::: */
static void simple_dft(float* input, uint16_t N, uint8_t harmonic, float* magnitude)
{
    if (input == NULL || magnitude == NULL || N == 0 || harmonic == 0)
    {
        *magnitude = 0.0f;
        return;
    }

    float real_sum = 0.0f;
    float imag_sum = 0.0f;
    
    // Tính DFT cho harmonic cụ thể
    // X[k] = sum(x[n] * e^(-j*2*pi*k*n/N)) cho k = harmonic
    for (uint16_t n = 0; n < N; n++)
    {
        // Sử dụng công thức Euler: e^(-j*theta) = cos(theta) - j*sin(theta)
        float angle = -2.0f * 3.14159265359f * (float)harmonic * (float)n / (float)N;
        float cos_val = cosf(angle);
        float sin_val = sinf(angle);
        
        real_sum += input[n] * cos_val;
        imag_sum += input[n] * sin_val;
    }
    
    // Magnitude = sqrt(real² + imag²)
    *magnitude = custom_sqrt(real_sum * real_sum + imag_sum * imag_sum) * 2.0f / (float)N;
}

/* :::::::::: Calculate THD for specific phase ::::::::::::: */
static float calculate_phase_thd(uint8_t phase)
{
    if (phase >= 3 || !g_Phase_Active[phase])
        return 0.0f;
        
    float fundamental = g_Fundamental_Voltage[phase];
    if (fundamental < THD_MIN_FUNDAMENTAL)
        return 0.0f;
    
    float harmonic_sum_squared = 0.0f;
    for (uint8_t h = 1; h < THD_HARMONICS_COUNT; h++) // Start from index 1 (2nd harmonic)
    {
        harmonic_sum_squared += g_Harmonic_Voltage[phase][h] * g_Harmonic_Voltage[phase][h];
    }
    
    return (custom_sqrt(harmonic_sum_squared) / fundamental) * 100.0f;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End of the program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */