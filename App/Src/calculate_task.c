/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Include ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "calculate_task.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static float custom_sqrt(float number);
// static uint32_t last_interrupt_time = 0;
static float signal_frequency = 0; // Tần số tín hiệu (Hz)
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Public Function ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
float g_Signal_Frequency = 0;
/* :::::::::: RMS Task Init :::::::: */
void RMS_Task_Init(void)
{
    // Cấu hình GPIO ngắt
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Kích hoạt clock cho GPIO
    // LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
    
    // Cấu hình pin GPIO
    GPIO_InitStruct.Pin = GPIO_TRIGGER_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN; // Pull-down cho cạnh lên
    LL_GPIO_Init(GPIO_TRIGGER_PORT, &GPIO_InitStruct);
    
    // Cấu hình ngắt EXTI
    LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_1; // Thay X bằng số line tương ứng với pin
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING; // Kích hoạt ngắt ở cạnh lên
    LL_EXTI_Init(&EXTI_InitStruct);
    
    // Kích hoạt ngắt trong NVIC
    NVIC_SetPriority(EXTI0_1_IRQn, 0); // Thay X bằng số ngắt tương ứng
    NVIC_EnableIRQ(EXTI0_1_IRQn);

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
        // if (last_interrupt_time != 0) // Bỏ qua lần ngắt đầu tiên
        // {   
        //     uint32_t period_ticks = 0;
        //     if (current_time > last_interrupt_time){
        //         period_ticks = current_time - last_interrupt_time;
        //     }
        //     else {
        //         period_ticks = current_time + 65535 - last_interrupt_time;
        //     }
        //     signal_frequency = (float)TIMER_FREQ_HZ / period_ticks; // Tần số = 1/T
        //     g_Signal_Frequency = signal_frequency;
        // }
        // last_interrupt_time = current_time;
}

void Calculate_RMS(void)
{
    if (g_Sample_Count<MINIMUM_SAMPLE) return; // Tránh chia cho 0

    for (uint8_t i = 0; i < 6; i++)
    {
        // Tính RMS từ giá trị lũy tiến
        g_RMS_Value[i] = custom_sqrt(RMS_Sum_Square[i] / g_Sample_Count);
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
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End of the program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */