// APP HEADER //
#include "app.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_usart.h"

static void Status_Led(void *pvParameters);
static void App_ModbusTask(void *pvParameters);


#define         SCHEDULER_TASK_COUNT  2
uint32_t        g_ui32SchedulerNumTasks = SCHEDULER_TASK_COUNT;
tSchedulerTask 	g_psSchedulerTable[SCHEDULER_TASK_COUNT] =
                {
                    {
                        &Status_Led,
                        (void *) 0,
                        10000,                        //call every 1000ms (1Hz)
                        0,                          //count from start
                        true                        //is active
                    },
                    {
                        &App_ModbusTask,
                        (void *) 0,
                        50,                          //call every 5ms (200Hz)
                        0,                          //count from start
                        true                        //is active
                    },
                };

void App_Main(void)
{   
    // Initialize Modbus slave
    App_ModbusInit();
    Modbus_RS485_Init();
    // Initialize scheduler 
    SchedulerInit(10000);  // 10000 ticks per second = 0.1ms per tick

    while (1)
    {
        SchedulerRun();
    }
}


void App_ModbusTask(void *pvParameters)
{
    // Suppress unused parameter warning
    (void)pvParameters;
    
    // Poll Modbus stack - this must be called regularly
    (void)eMBPoll();
    
    // Update live data registers
    static uint16_t counter = 0;
    static uint32_t uptime_seconds = 0;
    static uint16_t task_call_count = 0;
    
    counter++;
    task_call_count++;
    
    // Update every second (200 calls at 5ms = 1 second)
    if (task_call_count >= 200)
    {
        task_call_count = 0;
        uptime_seconds++;
    }
    
    // Update holding register with a counter (address 3)
    usSRegHoldBuf[3] = counter;
    
    // Update input register with live data
    usSRegInBuf[3] = counter * 2;
    
    // Update uptime in input registers (addresses 6-7 for 32-bit value)
    if (S_REG_INPUT_NREGS > 7)
    {
        usSRegInBuf[6] = (uint16_t)(uptime_seconds & 0xFFFF);        // Low word
        usSRegInBuf[7] = (uint16_t)((uptime_seconds >> 16) & 0xFFFF); // High word
    }
    
    // Reset counter to prevent overflow
    if (counter >= 65535)
    {
        counter = 0;
    }
}

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
    LL_GPIO_TogglePin(LED_PORT, LED_PIN);
}


