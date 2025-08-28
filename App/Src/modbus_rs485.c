#include "modbus_rs485.h"
#include "mbport.h"
#include "mb.h"


void Modbus_RS485_Init(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // RS485 DE pin
    GPIO_InitStruct.Pin = MB_UART_DE_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MB_UART_DE_PORT, &GPIO_InitStruct);

    // Initialize DE pin to RX mode (LOW)
    LL_GPIO_ResetOutputPin(MB_UART_DE_PORT, MB_UART_DE_PIN);
}


void App_ModbusInit(void)
{
    eMBErrorCode eStatus;
    
    // Initialize Modbus RTU slave with:
    // - Slave address: 1
    // - UART port: 1 (USART1)
    // - Baud rate: 9600
    // - Parity: None
    eStatus = eMBInit(MB_RTU, 8, 1, 9600, MB_PAR_NONE);
    
    if (eStatus == MB_ENOERR)
    {
        // Enable Modbus slave
        eStatus = eMBEnable();
        
        if (eStatus != MB_ENOERR)
        {
            // Handle error - just return, don't hang
            return;
        }
    }
    else
    {
        // Handle initialization error - just return, don't hang
        return;
    }
    
    // Initialize holding registers (firmware version, etc.)
    usSRegHoldBuf[0] = 0x0001;  // Firmware version major
    usSRegHoldBuf[1] = 0x0000;  // Firmware version minor

    // Clear measurement input registers range used by App_ModbusTask (10..49)
    for (uint16_t addr = 10; addr <= 49 && addr < S_REG_INPUT_NREGS; addr++) {
        usSRegInBuf[addr] = 0;
    }

    // Initialize coils (all off by default)
    // Coil 0 controls RELAY: 1=ON, 0=OFF
    ucSCoilBuf[0] = 0x00;
}


void Modbus_UART_IRQHandler(void)
{
    // Handle RX interrupt
  if (LL_USART_IsActiveFlag_RXNE(MB_UART_HANDLE) && LL_USART_IsEnabledIT_RXNE(MB_UART_HANDLE))
  {
#if MB_SLAVE_RTU_ENABLED > 0 || MB_SLAVE_ASCII_ENABLED > 0 
    pxMBFrameCBByteReceived();
#endif
    return;
  }
  
  // Handle TX interrupt
  if (LL_USART_IsActiveFlag_TXE(MB_UART_HANDLE) && LL_USART_IsEnabledIT_TXE(MB_UART_HANDLE))
  {
    // Ensure RS485 driver enabled for TX
    LL_GPIO_SetOutputPin(MB_UART_DE_PORT, MB_UART_DE_PIN);
#if MB_SLAVE_RTU_ENABLED > 0 || MB_SLAVE_ASCII_ENABLED > 0 
    pxMBFrameCBTransmitterEmpty();
#endif
    return;
  }

  // Handle TC interrupt (transmission complete)
  if (LL_USART_IsActiveFlag_TC(MB_UART_HANDLE) && LL_USART_IsEnabledIT_TC(MB_UART_HANDLE))
  {
    // Done sending last byte, switch DE back to RX mode
    LL_GPIO_ResetOutputPin(MB_UART_DE_PORT, MB_UART_DE_PIN);
    LL_USART_ClearFlag_TC(MB_UART_HANDLE);
    LL_USART_DisableIT_TC(MB_UART_HANDLE);
    return;
  }
}

void Modbus_Timer_IRQHandler(void)
{
    // Handle timer interrupt
    if (LL_TIM_IsActiveFlag_UPDATE(MB_TIMER_HANDLE))
    {
        LL_TIM_ClearFlag_UPDATE(MB_TIMER_HANDLE);
        pxMBPortCBTimerExpired();
    }
}
