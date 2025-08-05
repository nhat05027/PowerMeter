#ifndef CALCULATE_TASK_H_
#define CALCULATE_TASK_H_

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Include ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_exti.h"
#include "stm32f0xx_ll_tim.h"
#include <stdint.h>
#include "board.h"
#include "adc_task.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define MINIMUM_SAMPLE 60
// #define GPIO_TRIGGER_PIN        LL_GPIO_PIN_1 // Thay X bằng số pin cụ thể
// #define GPIO_TRIGGER_PORT      GPIOB         // Thay X bằng port cụ thể (A, B, C,...)
#define TIMER_HANDLE      TIM16        // Thay X bằng port cụ thể (A, B, C,...)
#define TIMER_FREQ_HZ          320000        // Timer chạy ở 320kHz (Prescaler = 100)
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
extern float g_RMS_Value[6];
extern float g_Signal_Frequency;
extern float g_Active_Power[3]; // Active Power for 3 phases (W)
extern float g_Reactive_Power[3]; // Reactive Power for 3 phases (VAR)
extern float g_Apparent_Power[3]; // Apparent Power for 3 phases (VA)
extern float g_Power_Factor[3]; // Power Factor for 3 phases
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Enum ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Struct ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Class ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* :::::::::: Calculate Task Init :::::::: */
void Calculate_Task_Init(void);


/* :::::::::: Interupt Handler ::::::::::::: */
void Calculate_Freq(void);
void Calculate_Active_Power(void);            // Deprecated - use Calculate_All_Power_Parameters
void Calculate_Reactive_Power(void);          // Deprecated - use Calculate_All_Power_Parameters  
void Calculate_Power_Factor(void);            // Deprecated - use Calculate_All_Power_Parameters
void Calculate_All_Power_Parameters(void);    // Optimized integrated calculation task
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End of the program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#endif /* CALCULATE_TASK_H_ */
