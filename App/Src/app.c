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


#define         SCHEDULER_TASK_COUNT  5
uint32_t        g_ui32SchedulerNumTasks = SCHEDULER_TASK_COUNT;
tSchedulerTask 	g_psSchedulerTable[SCHEDULER_TASK_COUNT] =
                {
                    {
                        &ADC_Task,
                        (void *) 0,
                        5,                          //call every 1000us
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
                    {
                        &Check_Phase_Timeouts,
                        (void *) 0,
                        10000,                         //call every 1000ms (1Hz) - Phase timeout check
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
    

    while (1)
    {
        SchedulerRun();
    }
}


static inline void MB_WriteFloatInput(uint16_t addr, float value)
{
    union { float f; uint16_t w[2]; } u;
    u.f = value;
    if ((addr + 1) < S_REG_INPUT_NREGS) {
        usSRegInBuf[addr]     = u.w[0]; // low word
        usSRegInBuf[addr + 1] = u.w[1]; // high word
    }
}

void App_ModbusTask(void *pvParameters)
{
    // Suppress unused parameter warning
    (void)pvParameters;
    
    // Poll Modbus stack - this must be called regularly
    (void)eMBPoll();

    // ---------------- Publish 3-phase measurements over Modbus Input Registers ----------------
    // Mapping (all as IEEE-754 float, 2 registers each, low word first):
    // 10-11: L1 Vrms, 12-13: L2 Vrms, 14-15: L3 Vrms
    // 16-17: L1 Irms, 18-19: L2 Irms, 20-21: L3 Irms
    // 22-23: L1 P (W), 24-25: L2 P, 26-27: L3 P
    // 28-29: L1 Q (VAR), 30-31: L2 Q, 32-33: L3 Q
    // 34-35: L1 S (VA), 36-37: L2 S, 38-39: L3 S
    // 40-41: L1 PF, 42-43: L2 PF, 44-45: L3 PF
    // 46-47: Frequency (Hz)
    // 48   : Phase Active bitmap (bit0=L1, bit1=L2, bit2=L3)
    // 49   : Phase Leading bitmap (bit0=L1, bit1=L2, bit2=L3)

    // Voltages (channels mapped in calculate_task.c)
    MB_WriteFloatInput(10, g_RMS_Value[3]); // L1 V
    MB_WriteFloatInput(12, g_RMS_Value[4]); // L2 V
    MB_WriteFloatInput(14, g_RMS_Value[5]); // L3 V

    // Currents
    MB_WriteFloatInput(16, g_RMS_Value[2]); // L1 I
    MB_WriteFloatInput(18, g_RMS_Value[1]); // L2 I
    MB_WriteFloatInput(20, g_RMS_Value[0]); // L3 I

    // Active Power P
    MB_WriteFloatInput(22, g_Active_Power[0]);
    MB_WriteFloatInput(24, g_Active_Power[1]);
    MB_WriteFloatInput(26, g_Active_Power[2]);

    // Reactive Power Q
    MB_WriteFloatInput(28, g_Reactive_Power[0]);
    MB_WriteFloatInput(30, g_Reactive_Power[1]);
    MB_WriteFloatInput(32, g_Reactive_Power[2]);

    // Apparent Power S
    MB_WriteFloatInput(34, g_Apparent_Power[0]);
    MB_WriteFloatInput(36, g_Apparent_Power[1]);
    MB_WriteFloatInput(38, g_Apparent_Power[2]);

    // Power Factor PF
    MB_WriteFloatInput(40, g_Power_Factor[0]);
    MB_WriteFloatInput(42, g_Power_Factor[1]);
    MB_WriteFloatInput(44, g_Power_Factor[2]);

    // Frequency
    MB_WriteFloatInput(46, g_Signal_Frequency);

    // Phase status bitmaps
    uint16_t phase_active_bits = (g_Phase_Active[0] ? 0x0001 : 0) |
                                 (g_Phase_Active[1] ? 0x0002 : 0) |
                                 (g_Phase_Active[2] ? 0x0004 : 0);
    uint16_t phase_leading_bits = (g_Phase_Leading[0] ? 0x0001 : 0) |
                                  (g_Phase_Leading[1] ? 0x0002 : 0) |
                                  (g_Phase_Leading[2] ? 0x0004 : 0);
    if (48 < S_REG_INPUT_NREGS) usSRegInBuf[48] = phase_active_bits;
    if (49 < S_REG_INPUT_NREGS) usSRegInBuf[49] = phase_leading_bits;

    // ---------------- Modbus coil to control relay ----------------
    // Coil 0 controls RELAY: 1=ON, 0=OFF
    bool relay_on = (ucSCoilBuf[0] & 0x01) != 0;
    if (relay_on) {
        LL_GPIO_SetOutputPin(RELAY_PORT, RELAY_PIN);
    } else {
        LL_GPIO_ResetOutputPin(RELAY_PORT, RELAY_PIN);
    }
}



static void App_UITask(void *pvParameters)
{
    // Suppress unused parameter warning
    (void)pvParameters;
    
    static uint32_t update_counter = 0;
    static uint8_t page_counter = 0;
    
    // Create real data structure and populate with calculate_task data
    ui_power_data_t real_data;
    
    // Copy RMS values (mapping channels to phases)
    real_data.voltage_rms[0] = g_RMS_Value[3]; // L1 voltage
    real_data.voltage_rms[1] = g_RMS_Value[4]; // L2 voltage
    real_data.voltage_rms[2] = g_RMS_Value[5]; // L3 voltage

    real_data.current_rms[0] = g_RMS_Value[0]; // L1 current
    real_data.current_rms[1] = g_RMS_Value[1]; // L2 current
    real_data.current_rms[2] = g_RMS_Value[2]; // L3 current

    // Copy power values
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        real_data.power_active[phase] = g_Active_Power[phase];
        real_data.power_reactive[phase] = g_Reactive_Power[phase];
        real_data.power_apparent[phase] = g_Apparent_Power[phase];
        real_data.power_factor[phase] = g_Power_Factor[phase];
    }
    
    real_data.frequency = g_Signal_Frequency;
    real_data.status = 0x01; // System OK
    
    // Update UI with real data
    UI_UpdateData(&real_data);
    
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


