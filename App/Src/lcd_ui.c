#include "lcd_ui.h"
#include <stddef.h>
#include "stm32f0xx_ll_gpio.h"

// Forward declarations
static void UI_HandleShortPress(uint8_t button);
static void UI_HandleLongPress(uint8_t button);

// Static variables
static TG12864_Handle *g_lcd = NULL;
static uint8_t g_current_page = UI_VOLTAGE_PAGE;
static ui_power_data_t g_power_data = {0};
static uint8_t g_auto_page_mode = 0;  // Auto page switching enabled by default
static uint8_t g_backlight_enabled = 1;  // Backlight enabled by default

// Initialize UI system
void UI_Init(TG12864_Handle *lcd)
{
    g_lcd = lcd;
    g_current_page = UI_VOLTAGE_PAGE;
    g_auto_page_mode = 1;
    
    LL_GPIO_SetOutputPin(LCD_BL_PORT, LCD_BL_PIN);  // Turn on backlight by default
    // Initialize button handler (no callback needed for flag-based approach)
    Button_Init();
    
    // Clear display and show initial page
    TG12864_Clear(g_lcd);
    UI_Refresh();
}

// Set current page
void UI_SetCurrentPage(uint8_t page)
{
    if (page < UI_PAGE_COUNT)
    {
        g_current_page = page;
        TG12864_Clear(g_lcd);
        UI_Refresh();
    }
}

// Get current page
uint8_t UI_GetCurrentPage(void)
{
    return g_current_page;
}

// Update power data
void UI_UpdateData(const ui_power_data_t *data)
{
    if (data != NULL)
    {
        g_power_data = *data;
        UI_Refresh();
    }
}

// Refresh current page
void UI_Refresh(void)
{
    if (g_lcd == NULL) return;
    
    switch (g_current_page)
    {
        case UI_VOLTAGE_PAGE:
            UI_ShowVoltagePage();
            break;
        case UI_CURRENT_PAGE:
            UI_ShowCurrentPage();
            break;
        case UI_POWER_PAGE:
            UI_ShowPowerPage();
            break;
        case UI_REACTIVE_PAGE:
            UI_ShowReactivePage();
            break;
        case UI_APPARENT_PAGE:
            UI_ShowApparentPage();
            break;
        case UI_STATUS_PAGE:
            UI_ShowStatusPage();
            break;
        default:
            break;
    }
}

// Show voltage RMS page
void UI_ShowVoltagePage(void)
{
    char buffer[16];
    
    // Title
    TG12864_DrawString_Font(g_lcd, 30, UI_TITLE_ROW, "VOLTAGE RMS", &Font5x7);
    
    // Draw phase boxes and data
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        uint8_t row = UI_DATA_START_ROW + (phase * UI_PHASE_HEIGHT);
        
        // Phase label
        buffer[0] = 'V';
        buffer[1] = '1' + phase;
        buffer[2] = ':';
        buffer[3] = '\0';
        TG12864_DrawString_Font(g_lcd, 0, row, buffer, &Font5x7);
        
        // Voltage value
        UI_FloatToString(g_power_data.voltage_rms[phase], buffer, 2);
        TG12864_DrawString_Font(g_lcd, 25, row, buffer, &Font5x7);
        
        // Unit
        TG12864_DrawString_Font(g_lcd, 80, row, "V", &Font5x7);
        
        // Simple separator using dash characters
        if (phase < 2)
        {
            TG12864_DrawString_Font(g_lcd, 0, row + 1, "---------------", &Font5x7);
        }
    }
    
    // Page indicator with auto mode and backlight status
    if (g_auto_page_mode)
    {
        if (g_backlight_enabled)
        {
            TG12864_DrawString_Font(g_lcd, 95, 7, "A1/4", &Font5x7);  // Auto mode + backlight
        }
        else
        {
            TG12864_DrawString_Font(g_lcd, 95, 7, "A1/4*", &Font5x7); // Auto mode + no backlight
        }
    }
    else
    {
        if (g_backlight_enabled)
        {
            TG12864_DrawString_Font(g_lcd, 105, 7, "1/4", &Font5x7);   // Manual mode + backlight
        }
        else
        {
            TG12864_DrawString_Font(g_lcd, 105, 7, "1/4*", &Font5x7);  // Manual mode + no backlight
        }
    }
}

// Show current RMS page
void UI_ShowCurrentPage(void)
{
    char buffer[16];
    
    // Title
    TG12864_DrawString_Font(g_lcd, 30, UI_TITLE_ROW, "CURRENT RMS", &Font5x7);
    
    // Draw phase data
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        uint8_t row = UI_DATA_START_ROW + (phase * UI_PHASE_HEIGHT);
        
        // Phase label
        buffer[0] = 'I';
        buffer[1] = '1' + phase;
        buffer[2] = ':';
        buffer[3] = '\0';
        TG12864_DrawString_Font(g_lcd, 0, row, buffer, &Font5x7);
        
        // Current value
        UI_FloatToString(g_power_data.current_rms[phase], buffer, 2);
        TG12864_DrawString_Font(g_lcd, 25, row, buffer, &Font5x7);
        
        // Unit
        TG12864_DrawString_Font(g_lcd, 80, row, "A", &Font5x7);
        
        // Simple separator using dash characters
        if (phase < 2)
        {
            TG12864_DrawString_Font(g_lcd, 0, row + 1, "---------------", &Font5x7);
        }
    }
    
    // Page indicator with auto mode and backlight status
    if (g_auto_page_mode)
    {
        if (g_backlight_enabled)
        {
            TG12864_DrawString_Font(g_lcd, 95, 7, "A2/4", &Font5x7);  // Auto mode + backlight
        }
        else
        {
            TG12864_DrawString_Font(g_lcd, 95, 7, "A2/4*", &Font5x7); // Auto mode + no backlight
        }
    }
    else
    {
        if (g_backlight_enabled)
        {
            TG12864_DrawString_Font(g_lcd, 105, 7, "2/4", &Font5x7);   // Manual mode + backlight
        }
        else
        {
            TG12864_DrawString_Font(g_lcd, 105, 7, "2/4*", &Font5x7);  // Manual mode + no backlight
        }
    }
}

// Show power page
void UI_ShowPowerPage(void)
{
    char buffer[16];
    
    // Title
    TG12864_DrawString_Font(g_lcd, 35, UI_TITLE_ROW, "POWER", &Font5x7);
    
    // Draw phase data
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        uint8_t row = UI_DATA_START_ROW + (phase * UI_PHASE_HEIGHT);
        
        // Phase label
        buffer[0] = 'P';
        buffer[1] = '1' + phase;
        buffer[2] = ':';
        buffer[3] = '\0';
        TG12864_DrawString_Font(g_lcd, 0, row, buffer, &Font5x7);
        
        // Power value
        UI_FloatToString(g_power_data.power_active[phase], buffer, 1);
        TG12864_DrawString_Font(g_lcd, 25, row, buffer, &Font5x7);
        
        // Unit
        TG12864_DrawString_Font(g_lcd, 80, row, "W", &Font5x7);
        
        // Simple separator using dash characters
        if (phase < 2)
        {
            TG12864_DrawString_Font(g_lcd, 0, row + 1, "---------------", &Font5x7);
        }
    }
    
    // Page indicator with auto mode and backlight status
    if (g_auto_page_mode)
    {
        if (g_backlight_enabled)
        {
            TG12864_DrawString_Font(g_lcd, 95, 7, "A3/4", &Font5x7);  // Auto mode + backlight
        }
        else
        {
            TG12864_DrawString_Font(g_lcd, 95, 7, "A3/4*", &Font5x7); // Auto mode + no backlight
        }
    }
    else
    {
        if (g_backlight_enabled)
        {
            TG12864_DrawString_Font(g_lcd, 105, 7, "3/4", &Font5x7);   // Manual mode + backlight
        }
        else
        {
            TG12864_DrawString_Font(g_lcd, 105, 7, "3/4*", &Font5x7);  // Manual mode + no backlight
        }
    }
}

// Show status page
void UI_ShowStatusPage(void)
{
    char buffer[16];
    
    // Title
    TG12864_DrawString_Font(g_lcd, 35, UI_TITLE_ROW, "STATUS", &Font5x7);
    
    // Frequency
    TG12864_DrawString_Font(g_lcd, 0, 2, "Freq:", &Font5x7);
    UI_FloatToString(g_power_data.frequency, buffer, 1);
    TG12864_DrawString_Font(g_lcd, 30, 2, buffer, &Font5x7);
    TG12864_DrawString_Font(g_lcd, 70, 2, "Hz", &Font5x7);
    
    // Status
    TG12864_DrawString_Font(g_lcd, 0, 4, "Status:", &Font5x7);
    if (g_power_data.status & 0x01)
    {
        TG12864_DrawString_Font(g_lcd, 42, 4, "OK", &Font5x7);
    }
    else
    {
        TG12864_DrawString_Font(g_lcd, 42, 4, "ERROR", &Font5x7);
    }
    
    // Page indicator with auto mode and backlight status
    if (g_auto_page_mode)
    {
        if (g_backlight_enabled)
        {
            TG12864_DrawString_Font(g_lcd, 95, 7, "A4/4", &Font5x7);  // Auto mode + backlight
        }
        else
        {
            TG12864_DrawString_Font(g_lcd, 95, 7, "A4/4*", &Font5x7); // Auto mode + no backlight
        }
    }
    else
    {
        if (g_backlight_enabled)
        {
            TG12864_DrawString_Font(g_lcd, 105, 7, "4/4", &Font5x7);   // Manual mode + backlight
        }
        else
        {
            TG12864_DrawString_Font(g_lcd, 105, 7, "4/4*", &Font5x7);  // Manual mode + no backlight
        }
    }
}

// Convert float to string with specified decimal places
void UI_FloatToString(float value, char *buffer, uint8_t decimal_places)
{
    if (buffer == NULL) return;
    
    // Handle negative values
    uint8_t index = 0;
    if (value < 0)
    {
        buffer[index++] = '-';
        value = -value;
    }
    
    // Handle overflow
    if (value >= 9999.0f)
    {
        buffer[0] = 'O';
        buffer[1] = 'V';
        buffer[2] = 'F';
        buffer[3] = '\0';
        return;
    }
    
    // Calculate multiplier for decimal places
    uint32_t multiplier = 1;
    for (uint8_t i = 0; i < decimal_places; i++)
    {
        multiplier *= 10;
    }
    
    // Convert to integer (with scaling for decimals)
    uint32_t int_value = (uint32_t)(value * multiplier + 0.5f); // Add 0.5 for rounding
    
    // Extract integer part
    uint32_t integer_part = int_value / multiplier;
    uint32_t decimal_part = int_value % multiplier;
    
    // Convert integer part
    if (integer_part == 0)
    {
        buffer[index++] = '0';
    }
    else
    {
        // Find number of digits
        uint32_t temp = integer_part;
        uint8_t digit_count = 0;
        while (temp > 0)
        {
            temp /= 10;
            digit_count++;
        }
        
        // Convert digits (reverse order)
        uint8_t start_pos = index;
        temp = integer_part;
        for (uint8_t i = 0; i < digit_count; i++)
        {
            buffer[start_pos + digit_count - 1 - i] = '0' + (temp % 10);
            temp /= 10;
        }
        index += digit_count;
    }
    
    // Add decimal point and decimal part if needed
    if (decimal_places > 0)
    {
        buffer[index++] = '.';
        
        // Convert decimal part
        uint32_t temp = decimal_part;
        for (uint8_t i = 0; i < decimal_places; i++)
        {
            uint32_t divisor = 1;
            for (uint8_t j = 0; j < (decimal_places - 1 - i); j++)
            {
                divisor *= 10;
            }
            buffer[index++] = '0' + ((temp / divisor) % 10);
        }
    }
    
    // Null terminate
    buffer[index] = '\0';
}

// Draw a simple separator line using characters
void UI_DrawBox(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    // Simplified - just draw separator characters
    if (g_lcd == NULL) return;
    
    // Draw simple line using minus characters
    for (uint8_t i = 0; i < (width / 6); i++)
    {
        TG12864_DrawString_Font(g_lcd, x + (i * 6), y, "-", &Font5x7);
    }
}

// Draw phase data in a consistent format
void UI_DrawPhaseData(uint8_t phase, const char *label, float value, uint8_t row)
{
    char buffer[16];
    
    if (g_lcd == NULL || label == NULL) return;
    
    // Phase label
    TG12864_DrawString_Font(g_lcd, 0, row, label, &Font5x7);
    
    // Value
    UI_FloatToString(value, buffer, 2);
    TG12864_DrawString_Font(g_lcd, 25, row, buffer, &Font5x7);
}

// ============ BUTTON EVENT QUEUE PROCESSING ============

// Process button events from queue
void UI_ProcessButtonFlags(void)
{
    button_event_queue_item_t event;
    
    // Process all events in queue
    while (Button_GetEvent(&event))
    {
        switch (event.event)
        {
            case BUTTON_EVENT_SHORT_PRESS:
                UI_HandleShortPress(event.button);
                break;
                
            case BUTTON_EVENT_LONG_PRESS:
                UI_HandleLongPress(event.button);
                break;
                
            default:
                break;
        }
    }
}

// Handle short press events
static void UI_HandleShortPress(uint8_t button)
{
    switch (button)
    {
        case BUTTON_UP:
            // Previous page
            g_auto_page_mode = 0;  // Exit auto mode when navigating
            if (g_current_page > 0)
            {
                UI_SetCurrentPage(g_current_page - 1);
            }
            else
            {
                UI_SetCurrentPage(UI_PAGE_COUNT - 1); // Wrap to last page
            }
            break;
            
        case BUTTON_DOWN:
            // Next page
            g_auto_page_mode = 0;  // Exit auto mode when navigating
            UI_SetCurrentPage((g_current_page + 1) % UI_PAGE_COUNT);
            break;
            
        case BUTTON_SELECT:
            // Toggle auto page mode
            g_auto_page_mode = !g_auto_page_mode;
            UI_Refresh();  // Refresh to show mode change
            break;
            
        case BUTTON_BACK:
            // Re-enable auto page mode (short press)
            g_auto_page_mode = 1;
            UI_Refresh();  // Refresh to show mode change
            break;
            
        default:
            break;
    }
}

// Handle long press events
static void UI_HandleLongPress(uint8_t button)
{
    switch (button)
    {
        case BUTTON_BACK:
            // Toggle backlight (long press)
            UI_SetBacklight(!g_backlight_enabled);
            UI_Refresh();  // Force refresh to show backlight status change
            break;
            
        case BUTTON_UP:
        case BUTTON_DOWN:
        case BUTTON_SELECT:
            LL_GPIO_TogglePin(RELAY_PORT, RELAY_PIN); // Blink LED to indicate long press
            break;
            
        default:
            break;
    }
}

// Check if auto page mode is enabled
uint8_t UI_IsAutoPageMode(void)
{
    return g_auto_page_mode;
}

// Set backlight state
void UI_SetBacklight(uint8_t enable)
{
    g_backlight_enabled = enable;
    
    // Control backlight pin if available
    // Assuming backlight is controlled by a GPIO pin
    // You may need to adjust this based on your hardware
    if (enable)
    {
        // Turn on backlight - implement based on your hardware
        LL_GPIO_SetOutputPin(LCD_BL_PORT, LCD_BL_PIN);
    }
    else
    {
        // Turn off backlight - implement based on your hardware
        LL_GPIO_ResetOutputPin(LCD_BL_PORT, LCD_BL_PIN);
    }
}

// Get backlight state
uint8_t UI_GetBacklight(void)
{
    return g_backlight_enabled;
}

// Show reactive power page
void UI_ShowReactivePage(void)
{
    char buffer[32];
    
    // Title
    TG12864_DrawString_Font(g_lcd, 0, UI_TITLE_ROW, "Reactive Power (VAR)", &Font5x7);
    
    // Phase data
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        uint8_t row = UI_DATA_START_ROW + (phase * UI_PHASE_HEIGHT);
        
        // Phase label (L1, L2, L3)
        TG12864_DrawString_Font(g_lcd, 0, row, (phase == 0) ? "L1:" : (phase == 1) ? "L2:" : "L3:", &Font5x7);
        
        // Reactive power value
        UI_FloatToString(g_power_data.power_reactive[phase], buffer, 2);
        TG12864_DrawString_Font(g_lcd, 25, row, buffer, &Font5x7);
        TG12864_DrawString_Font(g_lcd, 80, row, "VAR", &Font5x7);
    }
    
    // Auto/Manual mode indicator
    if (g_auto_page_mode)
    {
        TG12864_DrawString_Font(g_lcd, 100, 7, "AUTO", &Font5x7);
    }
    else
    {
        TG12864_DrawString_Font(g_lcd, 100, 7, "MAN", &Font5x7);
    }
}

// Show apparent power page
void UI_ShowApparentPage(void)
{
    char buffer[32];
    
    // Title
    TG12864_DrawString_Font(g_lcd, 0, UI_TITLE_ROW, "Apparent Power (VA)", &Font5x7);

    // Phase data
    for (uint8_t phase = 0; phase < 3; phase++)
    {
        uint8_t row = UI_DATA_START_ROW + (phase * UI_PHASE_HEIGHT);
        
        // Phase label (L1, L2, L3)
        TG12864_DrawString_Font(g_lcd, 0, row, (phase == 0) ? "L1:" : (phase == 1) ? "L2:" : "L3:", &Font5x7);

        // Apparent power value
        UI_FloatToString(g_power_data.power_apparent[phase], buffer, 2);
        TG12864_DrawString_Font(g_lcd, 25, row, buffer, &Font5x7);
        TG12864_DrawString_Font(g_lcd, 80, row, "VA", &Font5x7);

        // Power factor on same line
        UI_FloatToString(g_power_data.power_factor[phase], buffer, 3);
        TG12864_DrawString_Font(g_lcd, 0, row + 1, "PF:", &Font5x7);
        TG12864_DrawString_Font(g_lcd, 25, row + 1, buffer, &Font5x7);
    }
    
    // Auto/Manual mode indicator
    if (g_auto_page_mode)
    {
        TG12864_DrawString_Font(g_lcd, 100, 7, "AUTO", &Font5x7);
    }
    else
    {
        TG12864_DrawString_Font(g_lcd, 100, 7, "MAN", &Font5x7);
    }
}
