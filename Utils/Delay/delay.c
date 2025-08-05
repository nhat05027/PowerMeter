#include "delay.h"
#include "stm32f0xx_ll_tim.h"

/*
* UNCOMMENT THE DEFINE YOU WANT TO USE
*/
#define DELAY_USE_TIMER     
//#define DELAY_USE_SYSTICK

#ifdef DELAY_USE_TIMER
    extern TIM_TypeDef* delay_timebase_ms;
    extern TIM_TypeDef* delay_timebase_us;
#endif

#ifdef DELAY_USE_SYSTICK
    volatile uint8_t delay_update_flag = 0;
#endif

#ifdef DELAY_USE_TIMER
    TIM_TypeDef* delay_timer_us_handle;
    TIM_TypeDef* delay_timer_ms_handle;

    void Delay_Init(TIM_TypeDef* us_handle, TIM_TypeDef* ms_handle)
    {
        delay_timer_us_handle = us_handle;
        delay_timer_ms_handle = ms_handle;
    }
#endif

void Delay_US(uint32_t delay_time_us)
{
    // Util use SysTick
    #ifdef DELAY_USE_SYSTICK
        delay_update_flag = 0;
        /*
        * The HAL_SYSTICK_Config function by default uses the AHB clock without division by 8.
        * Therefore, T = 1 / 8*10^6 = 1.25*10^-7, so to get a tick for 1us, SysTick_load = 1*10^-6/1.25*10^-7 = 8
        */
        uint32_t systick_load = delay_time_us * 8;
        HAL_SYSTICK_Config(systick_load);
        while (delay_update_flag == 0);
    #endif

    #ifdef DELAY_USE_TIMER
        delay_timer_us_handle->CNT = 0;
        LL_TIM_EnableCounter(delay_timer_us_handle);
        while (delay_timer_us_handle->CNT < delay_time_us);
        LL_TIM_DisableCounter(delay_timer_us_handle);

    #endif
}

void Delay_MS(uint32_t delay_time_ms)
{   
    // Util use SysTick
    #ifdef DELAY_USE_SYSTICK
        delay_update_flag = 0;
        /*
        * The HAL_SYSTICK_Config function by default uses the AHB clock without division by 8.
        * Therefore, T = 1 / 8*10^6 = 1.25*10^-7, so to get a tick for 1ms, SysTick_load = 1*10^-3/1.25*10^-7 = 8000
        */
        uint32_t systick_load = delay_time_ms * 8000;
        HAL_SYSTICK_Config(systick_load);
        while (delay_update_flag == 0);
    #endif

    #ifdef DELAY_USE_TIMER
        delay_timer_ms_handle->CNT = 0;
        LL_TIM_EnableCounter(delay_timer_ms_handle);
        while (delay_timer_ms_handle->CNT < delay_time_ms);
        LL_TIM_DisableCounter(delay_timer_ms_handle);

    #endif
}

#ifdef DELAY_USE_SYSTICK
void util_delay_handle(void)
{
    delay_update_flag = 1;
    SysTick->CTRL &= !SysTick_CTRL_ENABLE_Msk;
}
#endif