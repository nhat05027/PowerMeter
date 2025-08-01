#include "modbus_rs485.h"
#include "mbport.h"
#include "mb.h"


void Modbus_RS485_Init(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    
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
    eStatus = eMBInit(MB_RTU, 1, 1, 9600, MB_PAR_NONE);
    
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
    
    // Initialize system information in holding registers
    usSRegHoldBuf[0] = 0x1234;  // Example data 1
    usSRegHoldBuf[1] = 0x5678;  // Example data 2  
    usSRegHoldBuf[2] = 0x9ABC;  // Example data 3
    usSRegHoldBuf[3] = 0x0000;  // Live counter (will be updated)
    usSRegHoldBuf[4] = 0x0001;  // Firmware version major
    usSRegHoldBuf[5] = 0x0000;  // Firmware version minor
    
    // Initialize input registers with system status
    usSRegInBuf[0] = 0x1111;    // System status 1
    usSRegInBuf[1] = 0x2222;    // System status 2
    usSRegInBuf[2] = 0x3333;    // System status 3
    usSRegInBuf[3] = 0x0000;    // Live counter x2 (will be updated)
    usSRegInBuf[4] = (uint16_t)(SystemCoreClock / 1000000); // System clock in MHz
    usSRegInBuf[5] = 0x0030;    // MCU type identifier (STM32F030)
    
    // Initialize coils (all off by default)
    ucSCoilBuf[0] = 0x00;       // LED control and other coils
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
    // LL_GPIO_SetOutputPin(MB_UART_DE_PORT, MB_UART_DE_PIN);
#if MB_SLAVE_RTU_ENABLED > 0 || MB_SLAVE_ASCII_ENABLED > 0 
    pxMBFrameCBTransmitterEmpty();
#endif
    return;
  }

  // Handle TC interrupt (transmission complete)
  if (LL_USART_IsActiveFlag_TC(MB_UART_HANDLE) && LL_USART_IsEnabledIT_TC(MB_UART_HANDLE))
  {
    // Đã truyền xong byte cuối, chuyển DE về RX mode
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
