#ifndef BOARD_H
#define BOARD_H

/*********************RS485 MODBUS******************/
#define MB_UART_HANDLE       USART1
#define MB_UART_IRQ          USART1_IRQn    
#define MB_UART_DE_PORT      GPIOB
#define MB_UART_DE_PIN       LL_GPIO_PIN_2
#define MB_TIMER_HANDLE      TIM14
#define MB_TIMER_IRQ         TIM14_IRQn

/*********************SPI******************/
#define SPI_HANDLE          SPI1
#define SPI_IRQ             SPI1_IRQn

#define SPI_CS_PORT         NULL
#define SPI_CS_PIN          NULL

/*********************LED******************/
#define LED_PORT            GPIOB
#define LED_PIN             LL_GPIO_PIN_10

#endif // BOARD_H