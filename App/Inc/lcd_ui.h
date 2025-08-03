#ifndef LCD_UI_H
#define LCD_UI_H

#include "tg12864.h"
#include "font5x7.h"
#include "board.h"
#include "button_handler.h"

// UI Constants
#define UI_PAGE_COUNT           4
#define UI_VOLTAGE_PAGE         0
#define UI_CURRENT_PAGE         1
#define UI_POWER_PAGE           2
#define UI_STATUS_PAGE          3

// Display Areas
#define UI_TITLE_ROW            0
#define UI_DATA_START_ROW       2
#define UI_PHASE_HEIGHT         2  // Each phase takes 2 rows

// Phase data structure
typedef struct {
    float voltage_rms[3];    // V1, V2, V3 RMS voltages
    float current_rms[3];    // I1, I2, I3 RMS currents  
    float power_active[3];   // P1, P2, P3 Active power
    float power_reactive[3]; // Q1, Q2, Q3 Reactive power
    float frequency;         // System frequency
    uint8_t status;          // System status flags
} ui_power_data_t;

// UI Functions
void UI_Init(TG12864_Handle *lcd);
void UI_SetCurrentPage(uint8_t page);
uint8_t UI_GetCurrentPage(void);
void UI_UpdateData(const ui_power_data_t *data);
void UI_Refresh(void);

// Button processing (flag-based approach)
void UI_ProcessButtonFlags(void);

// UI state functions
uint8_t UI_IsAutoPageMode(void);
void UI_SetBacklight(uint8_t enable);
uint8_t UI_GetBacklight(void);

// Page display functions
void UI_ShowVoltagePage(void);
void UI_ShowCurrentPage(void);
void UI_ShowPowerPage(void);
void UI_ShowStatusPage(void);

// Utility functions
void UI_FloatToString(float value, char *buffer, uint8_t decimal_places);
void UI_DrawBox(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void UI_DrawPhaseData(uint8_t phase, const char *label, float value, uint8_t row);

#endif // LCD_UI_H
