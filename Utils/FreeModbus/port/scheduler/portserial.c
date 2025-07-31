/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: portserial.c,v 1.1 2006/08/22 21:35:13 wolti Exp $
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_usart.h"
#include "stm32_assert.h"


/* ----------------------- Static variables ---------------------------------*/
static BOOL bTxEnabled = FALSE;

/* ----------------------- static functions ---------------------------------*/
//static void prvvUARTTxReadyISR( void );
//static void prvvUARTRxISR( void );

/* ----------------------- Start implementation -----------------------------*/
void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
  /* If xRXEnable enable serial receive interrupts. If xTxENable enable
   * transmitter empty interrupts.
   */
  if (xRxEnable) {
    LL_USART_EnableIT_RXNE(MB_UART_HANDLE);
    // Set DE to RX mode
    // LL_GPIO_ResetOutputPin(MB_UART_DE_PORT, MB_UART_DE_PIN);
  } else {
    LL_USART_DisableIT_RXNE(MB_UART_HANDLE);
  }

  if (xTxEnable) {
    LL_USART_EnableIT_TXE(MB_UART_HANDLE);
    // Set DE to TX mode
    LL_GPIO_SetOutputPin(MB_UART_DE_PORT, MB_UART_DE_PIN);
    // Đảm bảo ngắt TC được bật để xử lý cuối truyền
    LL_USART_ClearFlag_TC(MB_UART_HANDLE);
    LL_USART_EnableIT_TC(MB_UART_HANDLE);
  } else {
    LL_USART_DisableIT_TXE(MB_UART_HANDLE);
    // KHÔNG reset DE ở đây, sẽ reset khi truyền xong (TC)
    // LL_GPIO_ResetOutputPin(MB_UART_DE_PORT, MB_UART_DE_PIN);
    // Enable TC interrupt để chuyển DE về RX khi truyền xong
    LL_USART_ClearFlag_TC(MB_UART_HANDLE);
    LL_USART_EnableIT_TC(MB_UART_HANDLE);
  }
}

BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
  // UART is already configured in main.c
  (void)ucPORT;
  (void)ulBaudRate;
  (void)ucDataBits;
  (void)eParity;
  
  return TRUE;
}

BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
  /* Put a byte in the UARTs transmit buffer. This function is called
    * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
    * called. */
  
  while (!LL_USART_IsActiveFlag_TXE(MB_UART_HANDLE));
  LL_USART_TransmitData8(MB_UART_HANDLE, (uint8_t)ucByte);
  
  return TRUE;
}

BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
    /* Return the byte in the UARTs receive buffer. This function is called
     * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
     */
  
  if (LL_USART_IsActiveFlag_RXNE(MB_UART_HANDLE))
  {
      *pucByte = (uint8_t)LL_USART_ReceiveData8(MB_UART_HANDLE);
      return TRUE;
  }
  return FALSE;
}

/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */

/*
 *move to irq_callback.c
static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty(  );
}
*/

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
/*
static void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived(  );
}
*/

/* ----------------------- RS485 DE Control Function -----------------------*/
// Simple DE control - PA5 pin
// You can modify this section as needed
