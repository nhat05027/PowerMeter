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
#define MINIMUM_SAMPLE 150
#define TIMER_HANDLE      TIM16        
#define TIMER_FREQ_HZ         320000.0f * 0.99f      // Timer chạy ở 310kHz (Prescaler = 100)

#define Voltage_Alpha_Coeff   0.242505f * 1.02f// Vrms * 3.3/(3.3+660+330) * 4096/3.3 = ADC value
#define Current_Alpha_Coeff   0.003747f * 1.31f // Irms * 1/2000 * (100+330) * 4096/3.3 = ADC value
#define Voltage_Beta_Coeff    -3.0f
#define Current_Beta_Coeff    -3.0f

#define Voltage_Transform_Ratio  1.0f
#define Current_Transform_Ratio  1.0f

// Phase detection timing constants
#define PHASE_TIMEOUT_CYCLES    5     // Phase timeout in scheduler cycles (no ZCD = no voltage)
#define PHASE_DETECTION_WINDOW  50     // Detection window for current leading/lagging (in timer ticks)

// THD calculation constants
#define THD_HARMONICS_COUNT     15    // Number of harmonics to analyze (up to 15th harmonic)
#define THD_SAMPLES_PER_CYCLE   128   // Samples per fundamental cycle for FFT
#define THD_MIN_FUNDAMENTAL     10.0f // Minimum fundamental voltage for valid THD calculation
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
extern float g_RMS_Value[6];
extern float g_Signal_Frequency;
extern float g_Active_Power[3]; // Active Power for 3 phases (W)
extern float g_Reactive_Power[3]; // Reactive Power for 3 phases (VAR)
extern float g_Apparent_Power[3]; // Apparent Power for 3 phases (VA)
extern float g_Power_Factor[3]; // Power Factor for 3 phases

// Phase status flags for zero crossing detection
extern uint8_t g_Phase_Active[3]; // Phase activity status (0=no voltage, 1=active)
extern uint8_t g_Phase_Leading[3]; // Phase relationship (0=lagging, 1=leading)

// THD analysis variables
extern float g_THD_Voltage[3];     // THD for 3 phase voltages (%)
extern float g_Fundamental_Voltage[3]; // Fundamental voltage component (V)
extern float g_Harmonic_Voltage[3][THD_HARMONICS_COUNT]; // Individual harmonics (V)
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Enum ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Struct ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Class ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* :::::::::: Calculate Task Init :::::::: */
void Calculate_Task_Init(void);


/* :::::::::: Interupt Handler ::::::::::::: */
void Calculate_Freq(void);
void Calculate_All_Power_Parameters(void);    // Optimized integrated calculation task

/* :::::::::: Phase Detection Functions (for GPIO ZCD interrupts) ::::::::::::: */
void Phase_L1_ZCD_Handler(void);  // Call this in L1 zero crossing interrupt
void Phase_L2_ZCD_Handler(void);  // Call this in L2 zero crossing interrupt  
void Phase_L3_ZCD_Handler(void);  // Call this in L3 zero crossing interrupt

/* :::::::::: Phase Status Functions ::::::::::::: */
void Reset_Phase_Status(void);
uint8_t Get_Phase_Active_Status(uint8_t phase);
uint8_t Get_Phase_Leading_Status(uint8_t phase);
void Check_Phase_Timeouts(void *pvParameters); // Call this periodically to check for phase timeouts

/* :::::::::: THD Analysis Functions ::::::::::::: */
void Calculate_THD_Task(void *pvParameters);  // Main THD calculation task
void THD_Analyze_Voltage_Phase(uint8_t phase); // Analyze THD for specific phase
void THD_Reset_Data(void);                    // Reset THD analysis data
float THD_Get_Voltage_THD(uint8_t phase);     // Get THD percentage for phase
float THD_Get_Harmonic_Voltage(uint8_t phase, uint8_t harmonic); // Get specific harmonic value
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End of the program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#endif /* CALCULATE_TASK_H_ */
