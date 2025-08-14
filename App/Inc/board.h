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
#define SPI_HANDLE          SPI2
#define SPI_IRQ             SPI2_IRQn

#define SPI_CS_PORT         GPIOB
#define SPI_CS_PIN          LL_GPIO_PIN_12

/*********************LED******************/
#define LED_PORT            GPIOB
#define LED_PIN             LL_GPIO_PIN_10

/*********************ADC******************/
#define ADC_FEEDBACK_HANDLE     ADC1
#define ADC_FEEDBACK_IRQ        ADC1_IRQn

#define ADC_CHANNEL_COUNT       7

#define ADC_NEUTRAL           LL_ADC_CHANNEL_0

#define ADC_CURRENT_L1        LL_ADC_CHANNEL_1
#define ADC_CURRENT_L2        LL_ADC_CHANNEL_2
#define ADC_CURRENT_L3        LL_ADC_CHANNEL_3

#define ADC_VOLTAGE_L1        LL_ADC_CHANNEL_4
#define ADC_VOLTAGE_L2        LL_ADC_CHANNEL_5
#define ADC_VOLTAGE_L3        LL_ADC_CHANNEL_6

/*********************LCD******************/
#define LCD_BL_PORT          GPIOB
#define LCD_BL_PIN           LL_GPIO_PIN_11
#define LCD_CS1_PORT         GPIOA
#define LCD_CS1_PIN          LL_GPIO_PIN_9
#define LCD_CS2_PORT         GPIOA
#define LCD_CS2_PIN          LL_GPIO_PIN_8
#define LCD_DI_PORT          GPIOB
#define LCD_DI_PIN           LL_GPIO_PIN_9
#define LCD_EN_PORT          GPIOB
#define LCD_EN_PIN           LL_GPIO_PIN_8
#define LCD_NRST_PORT        NULL
#define LCD_NRST_PIN         NULL

#define LCD_D0_PORT          GPIOB
#define LCD_D0_PIN           LL_GPIO_PIN_5
#define LCD_D1_PORT          GPIOB
#define LCD_D1_PIN           LL_GPIO_PIN_4
#define LCD_D2_PORT          GPIOB
#define LCD_D2_PIN           LL_GPIO_PIN_3
#define LCD_D3_PORT          GPIOA
#define LCD_D3_PIN           LL_GPIO_PIN_15
#define LCD_D4_PORT          GPIOF
#define LCD_D4_PIN           LL_GPIO_PIN_1
#define LCD_D5_PORT          GPIOA
#define LCD_D5_PIN           LL_GPIO_PIN_12
#define LCD_D6_PORT          GPIOA
#define LCD_D6_PIN           LL_GPIO_PIN_11     
#define LCD_D7_PORT          GPIOA
#define LCD_D7_PIN           LL_GPIO_PIN_10

/*********************I2C******************/
#define I2C_HANDLE           I2C2
#define I2C_IRQ              I2C2_IRQn
#define I2C_SDA_PORT         GPIOF
#define I2C_SDA_PIN          LL_GPIO_PIN_7
#define I2C_SCL_PORT         GPIOF
#define I2C_SCL_PIN          LL_GPIO_PIN_6

/*********************TIMER******************/
// #define TIMER_HANDLE         TIM16

/*********************ZCD******************/
#define ZCD_L1_PORT             GPIOB
#define ZCD_L1_PIN              LL_GPIO_PIN_1
#define ZCD_L2_PORT             GPIOB
#define ZCD_L2_PIN              LL_GPIO_PIN_0
#define ZCD_L3_PORT             GPIOA
#define ZCD_L3_PIN              LL_GPIO_PIN_7

/*********************BUTTON******************/
#define BUTTON_0_PORT           GPIOC
#define BUTTON_0_PIN            LL_GPIO_PIN_13
#define BUTTON_1_PORT           GPIOC
#define BUTTON_1_PIN            LL_GPIO_PIN_14
#define BUTTON_2_PORT           GPIOC
#define BUTTON_2_PIN            LL_GPIO_PIN_15
#define BUTTON_3_PORT           GPIOF
#define BUTTON_3_PIN            LL_GPIO_PIN_0

/*********************RELAY******************/
#define RELAY_PORT              GPIOB
#define RELAY_PIN               LL_GPIO_PIN_10

#endif // BOARD_H