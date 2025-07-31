// APP HEADER //
#include "app.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_usart.h"

static void Status_Led(void *pvParameters);

// External declarations for Modbus buffers from user_mb_app.c
// extern USHORT usSRegHoldBuf[];
// extern USHORT usSRegInBuf[];
// extern UCHAR ucSCoilBuf[];

#define         SCHEDULER_TASK_COUNT  1
uint32_t        g_ui32SchedulerNumTasks = SCHEDULER_TASK_COUNT;
tSchedulerTask 	g_psSchedulerTable[SCHEDULER_TASK_COUNT] =
                {
                    {
                        &Status_Led,
                        (void *) 0,
                        1000,                        //call every 500ms
                        0,                          //count from start
                        true                        //is active
                    },
                    // {
                    //     &App_ModbusTask,
                    //     (void *) 0,
                    //     5,                          //call every 5ms (200Hz)
                    //     0,                          //count from start
                    //     true                        //is active
                    // },
                };

void App_Main(void)
{   
    // Initialize Modbus slave
    // App_ModbusInit();
    // RS485_Init();
    // Initialize scheduler 
    SchedulerInit(1000);  // 1000 ticks per second = 1ms per tick

    while (1)
    {
        SchedulerRun();
    }
}

// void App_ModbusInit(void)
// {
//     eMBErrorCode eStatus;
    
//     // Initialize Modbus RTU slave with:
//     // - Slave address: 1
//     // - UART port: 1 (USART1)
//     // - Baud rate: 9600
//     // - Parity: None
//     eStatus = eMBInit(MB_RTU, 1, 1, 9600, MB_PAR_NONE);
    
//     if (eStatus == MB_ENOERR)
//     {
//         // Enable Modbus slave
//         eStatus = eMBEnable();
        
//         if (eStatus != MB_ENOERR)
//         {
//             // Handle error - just return, don't hang
//             return;
//         }
//     }
//     else
//     {
//         // Handle initialization error - just return, don't hang
//         return;
//     }
    
//     // Initialize system information in holding registers
//     usSRegHoldBuf[0] = 0x1234;  // Example data 1
//     usSRegHoldBuf[1] = 0x5678;  // Example data 2  
//     usSRegHoldBuf[2] = 0x9ABC;  // Example data 3
//     usSRegHoldBuf[3] = 0x0000;  // Live counter (will be updated)
//     usSRegHoldBuf[4] = 0x0001;  // Firmware version major
//     usSRegHoldBuf[5] = 0x0000;  // Firmware version minor
    
//     // Initialize input registers with system status
//     usSRegInBuf[0] = 0x1111;    // System status 1
//     usSRegInBuf[1] = 0x2222;    // System status 2
//     usSRegInBuf[2] = 0x3333;    // System status 3
//     usSRegInBuf[3] = 0x0000;    // Live counter x2 (will be updated)
//     usSRegInBuf[4] = (uint16_t)(SystemCoreClock / 1000000); // System clock in MHz
//     usSRegInBuf[5] = 0x0030;    // MCU type identifier (STM32F030)
    
//     // Initialize coils (all off by default)
//     ucSCoilBuf[0] = 0x00;       // LED control and other coils
// }

// void App_ModbusTask(void *pvParameters)
// {
//     // Suppress unused parameter warning
//     (void)pvParameters;
    
//     // Poll Modbus stack - this must be called regularly
//     (void)eMBPoll();
    
//     // Update live data registers
//     static uint16_t counter = 0;
//     static uint32_t uptime_seconds = 0;
//     static uint16_t task_call_count = 0;
    
//     counter++;
//     task_call_count++;
    
//     // Update every second (200 calls at 5ms = 1 second)
//     if (task_call_count >= 200)
//     {
//         task_call_count = 0;
//         uptime_seconds++;
//     }
    
//     // Update holding register with a counter (address 3)
//     usSRegHoldBuf[3] = counter;
    
//     // Update input register with live data
//     usSRegInBuf[3] = counter * 2;
    
//     // Update uptime in input registers (addresses 6-7 for 32-bit value)
//     if (S_REG_INPUT_NREGS > 7)
//     {
//         usSRegInBuf[6] = (uint16_t)(uptime_seconds & 0xFFFF);        // Low word
//         usSRegInBuf[7] = (uint16_t)((uptime_seconds >> 16) & 0xFFFF); // High word
//     }
    
//     // Reset counter to prevent overflow
//     if (counter >= 65535)
//     {
//         counter = 0;
//     }
// }

// static void Status_Led(void *pvParameters)
// {
//     // Suppress unused parameter warning
//     (void)pvParameters;
    
//     // Check if coil 1 is set for manual LED control
//     if (ucSCoilBuf[1] & 0x01) // Use coil 1 for LED blink control
//     {
//         LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_4);
//     }
//     // If coil 0 is set, LED is controlled by Modbus (static on/off)
//     else if (ucSCoilBuf[0] & 0x01) 
//     {
//         LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_4);  // Turn on LED
//     }
//     else
//     {
//         // Default: blink LED to show system is alive
//         LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_4);
//     }
// }

static void Status_Led(void *pvParameters)
{
    // Default: blink LED to show system is alive
    LL_GPIO_TogglePin(GPIOB, LL_GPIO_PIN_10);
}


