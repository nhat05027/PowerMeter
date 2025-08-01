#ifndef MODBUS_RS485_H
#define MODBUS_RS485_H

#include <stdbool.h>
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_usart.h"
#include "stm32f0xx_ll_tim.h"
#include "stm32f0xx_ll_bus.h"
#include "board.h"
#include "user_mb_app.h"


// External declarations for Modbus buffers from user_mb_app.c
extern USHORT usSRegHoldBuf[];
extern USHORT usSRegInBuf[];
extern UCHAR ucSCoilBuf[];

void Modbus_RS485_Init(void);
void App_ModbusInit(void);
void Modbus_UART_IRQHandler(void);
void Modbus_Timer_IRQHandler(void);

#endif // MODBUS_RS485_H