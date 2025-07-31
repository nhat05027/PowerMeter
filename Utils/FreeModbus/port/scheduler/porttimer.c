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
 * File: $Id: porttimer.c,v 1.1 2006/08/22 21:35:13 wolti Exp $
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_tim.h"

/* ----------------------- static functions ---------------------------------*/
//static void prvvTIMERExpiredISR( void );

/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortTimersInit( USHORT usTim1Timerout50us )
{
  // LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM14);
  LL_TIM_DisableCounter(TIMER_HANDLE);
  LL_TIM_SetPrescaler(TIMER_HANDLE, (SystemCoreClock / 1000000UL) - 1);
  uint32_t autoreload = 50 * usTim1Timerout50us;
  LL_TIM_SetAutoReload(TIMER_HANDLE, autoreload - 1);
  LL_TIM_SetCounter(TIMER_HANDLE, 0);
  LL_TIM_SetCounterMode(TIMER_HANDLE, LL_TIM_COUNTERMODE_UP);
  LL_TIM_EnableIT_UPDATE(TIMER_HANDLE);
  LL_TIM_DisableARRPreload(TIMER_HANDLE);
  NVIC_SetPriority(TIM14_IRQn, 0);
  return TRUE;
}


inline void
vMBPortTimersEnable(  )
{
  /* Enable the timer with the timeout passed to xMBPortTimersInit( ) */
  LL_TIM_SetCounter(TIMER_HANDLE, 0);
  LL_TIM_ClearFlag_UPDATE(TIMER_HANDLE);
  LL_TIM_EnableCounter(TIMER_HANDLE);  
}

inline void
vMBPortTimersDisable(  )
{
  /* Disable any pending timers. */
  LL_TIM_DisableCounter(TIMER_HANDLE);
  LL_TIM_ClearFlag_UPDATE(TIMER_HANDLE);
}

/* Create an ISR which is called whenever the timer has expired. This function
 * must then call pxMBPortCBTimerExpired( ) to notify the protocol stack that
 * the timer has expired.
 */
/*
 * call in irq_callbacj.c
static void prvvTIMERExpiredISR( void )
{
    ( void )pxMBPortCBTimerExpired(  );
    
}
*/

