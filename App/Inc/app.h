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


void App_Main(void);


#endif /* APP_H_ */
