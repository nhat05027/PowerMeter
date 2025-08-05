// APP HEADER //
#include "app.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_usart.h"

TG12864_Handle LCD = {
    .cs1 = {LCD_CS1_PORT, LCD_CS1_PIN},
    .cs2 = {LCD_CS2_PORT, LCD_CS2_PIN},
    .e   = {LCD_EN_PORT, LCD_EN_PIN},
    .rw  = {NULL, NULL},
    .di  = {LCD_DI_PORT, LCD_DI_PIN},
    .rst = {LCD_NRST_PORT, LCD_NRST_PIN},
    .db = {
        {LCD_D0_PORT, LCD_D0_PIN},
        {LCD_D1_PORT, LCD_D1_PIN},
        {LCD_D2_PORT, LCD_D2_PIN},
        {LCD_D3_PORT, LCD_D3_PIN},
        {LCD_D4_PORT, LCD_D4_PIN},
        {LCD_D5_PORT, LCD_D5_PIN},
        {LCD_D6_PORT, LCD_D6_PIN},
        {LCD_D7_PORT, LCD_D7_PIN},
    },
};

static void App_ModbusTask(void *pvParameters);
static void App_UITask(void *pvParameters);
static void App_ButtonTask(void *pvParameters);


#define         SCHEDULER_TASK_COUNT  4
uint32_t        g_ui32SchedulerNumTasks = SCHEDULER_TASK_COUNT;
tSchedulerTask 	g_psSchedulerTable[SCHEDULER_TASK_COUNT] =
                {
                    {
                        &ADC_Task,
                        (void *) 0,
                        10,                          //call every 500us
                        0,			                //count from start
                        true		                //is active
                    },
                    {
                        &App_ModbusTask,
                        (void *) 0,
                        5000,                          //call every 500ms (2Hz)
                        0,                          //count from start
                        true                        //is active
                    },
                    {
                        &App_UITask,
                        (void *) 0,
                        5000,                        //call every 500ms (2Hz) - LCD update
                        0,                          //count from start
                        true                        //is active
                    },
                    {
                        &App_ButtonTask,
                        (void *) 0,
                        1000,                         //call every 100ms (10Hz) - Button scan
                        0,                          //count from start
                        true                        //is active
                    },
                };

void App_Main(void)
{   
    // Initialize Modbus slave
    App_ModbusInit();
    Modbus_RS485_Init();
    // Initialize ADC task
    // Use 7.5 cycles sampling time for faster response
    ADC_Task_Init(LL_ADC_SAMPLINGTIME_7CYCLES_5);
    Calculate_Task_Init();

    // Initialize scheduler 
    SchedulerInit(10000);  // 10000 ticks per second = 0.1ms per tick
    // Initialize LCD
    TG12864_Init(&LCD);
    
    // Enable rotation for upside-down mounted LCD
    TG12864_SetRotate180(1);
    
    // Initialize UI system
    UI_Init(&LCD);
    
    // Create sample data for testing
    ui_power_data_t test_data = {
        .voltage_rms = {220.15f, 219.87f, 221.32f},
        .current_rms = {12.45f, 11.98f, 12.67f},
        .power_active = {2743.3f, 2635.1f, 2801.8f},
        .power_reactive = {456.7f, 432.1f, 478.9f},
        .frequency = 50.0f,
        .status = 0x01  // System OK
    };
    
    // Update UI with test data
    UI_UpdateData(&test_data);

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


static void App_UITask(void *pvParameters)
{
    // Suppress unused parameter warning
    (void)pvParameters;
    
    // Simulate changing power data
    static uint16_t update_counter = 0;
    static uint8_t page_counter = 0;
    
    ui_power_data_t live_data;
    
    // Generate simulated power data with some variation
    float base_voltage = 220.0f + (update_counter % 20) * 0.1f - 1.0f; // 219-221V
    float base_current = 12.0f + (update_counter % 15) * 0.05f - 0.375f; // 11.625-12.375A
    
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        live_data.voltage_rms[phase] = base_voltage + (phase * 0.5f) + ((update_counter + phase * 7) % 10) * 0.05f;
        live_data.current_rms[phase] = base_current + (phase * 0.3f) + ((update_counter + phase * 5) % 8) * 0.02f;
        live_data.power_active[phase] = live_data.voltage_rms[phase] * live_data.current_rms[phase] * 0.95f; // PF = 0.95
        live_data.power_reactive[phase] = live_data.power_active[phase] * 0.33f; // tan(phi) = 0.33
    }
    
    live_data.frequency = 50.0f + ((update_counter % 100) * 0.01f) - 0.5f; // 49.5-50.5 Hz
    live_data.status = (update_counter % 50 < 45) ? 0x01 : 0x00; // Occasional error simulation
    
    // Update UI data
    UI_UpdateData(&live_data);
    
    // Process button events (read flags and handle UI logic)
    UI_ProcessButtonFlags();
    
    // Auto page switching only if auto mode is enabled
    update_counter++;
    if (UI_IsAutoPageMode() && (update_counter % 25 == 0)) // Every 5 seconds (25 calls at 200ms)
    {
        page_counter = (page_counter + 1) % UI_PAGE_COUNT;
        UI_SetCurrentPage(page_counter);
    }
}

static void App_ButtonTask(void *pvParameters)
{
    // Suppress unused parameter warning
    (void)pvParameters;
    
    // Button task only scans hardware and sets event flags
    Button_Scan();
    Button_Process();
}


