#ifndef DELAY_H_
#define DELAY_H_

#include <stdint.h>
#include "stm32f030x8.h"

void Delay_Init(TIM_TypeDef* delay_us_handle, TIM_TypeDef* delay_ms_handle);
void Delay_US(uint32_t delay_time_us);
void Delay_MS(uint32_t delay_time_ms);
void util_delay_handle(void);

#endif /* DELAY_H_ */
