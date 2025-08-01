#ifndef APP_H_
#define APP_H_

#include <stdint.h>
#include <stdbool.h>

#include "stm32f030x8.h"


// USER DRIVER //
#include "scheduler.h"

// INCLUDE TASK //

// INCLUDE LIB //
#include "modbus_rs485.h"

void App_Main(void);


#endif /* APP_H_ */
