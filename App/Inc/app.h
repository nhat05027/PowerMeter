#ifndef APP_H_
#define APP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


#include "stm32f030x8.h"


// USER DRIVER //
#include "scheduler.h"
#include "tg12864.h"
#include "tg12864_font.h"
#include "lcd_ui.h"
#include "button_handler.h"

// INCLUDE TASK //
#include "adc_task.h"
#include "calculate_task.h"
#include "spi_task.h"

// INCLUDE LIB //
#include "modbus_rs485.h"
#include "button_handler.h"

// INCLUDE BOARD //
#include "board.h"

// Global variables from calculate_task.c for measurements
extern float g_RMS_Value[6];         // RMS values for 6 channels
extern float g_Active_Power[3];      // Active power for 3 phases  
extern float g_Reactive_Power[3];    // Reactive power for 3 phases
extern float g_Apparent_Power[3];    // Apparent power for 3 phases
extern float g_Power_Factor[3];      // Power factor for 3 phases
extern float g_Signal_Frequency;     // Signal frequency


void App_Main(void);


#endif /* APP_H_ */
