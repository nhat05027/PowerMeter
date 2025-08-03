#include "button_handler.h"
#include "stm32f0xx_ll_gpio.h"
#include <stddef.h>

// Button state variables
static uint8_t g_button_state[BUTTON_COUNT] = {0};
static uint8_t g_button_last_state[BUTTON_COUNT] = {0};
static uint32_t g_button_press_time[BUTTON_COUNT] = {0};
static uint8_t g_button_press_flag[BUTTON_COUNT] = {0};  // Flag to track if button press was recorded
static uint32_t g_button_tick_counter = 0;

// Button event queue
static button_event_queue_item_t g_button_queue[BUTTON_QUEUE_SIZE];
static uint8_t g_queue_head = 0;
static uint8_t g_queue_tail = 0;
static uint8_t g_queue_count = 0;

// Long press threshold (in ticks) - 10 ticks = 1000ms at 10ms per tick
#define BUTTON_LONG_PRESS_THRESHOLD    10

// Queue helper functions
static uint8_t Button_QueuePut(uint8_t button, button_event_t event)
{
    if (g_queue_count >= BUTTON_QUEUE_SIZE)
    {
        return 0; // Queue full
    }
    
    g_button_queue[g_queue_tail].button = button;
    g_button_queue[g_queue_tail].event = event;
    
    g_queue_tail = (g_queue_tail + 1) % BUTTON_QUEUE_SIZE;
    g_queue_count++;
    
    return 1; // Success
}

static uint8_t Button_QueueGet(button_event_queue_item_t *event)
{
    if (g_queue_count == 0)
    {
        return 0; // Queue empty
    }
    
    if (event != NULL)
    {
        *event = g_button_queue[g_queue_head];
    }
    
    g_queue_head = (g_queue_head + 1) % BUTTON_QUEUE_SIZE;
    g_queue_count--;
    
    return 1; // Success
}

// Initialize button system
void Button_Init(void)
{
    // Reset all button states
    for (uint8_t i = 0; i < BUTTON_COUNT; i++)
    {
        g_button_state[i] = 0;
        g_button_last_state[i] = 0;
        g_button_press_time[i] = 0;
        g_button_press_flag[i] = 0;
    }
    g_button_tick_counter = 0;
    
    // Initialize queue
    g_queue_head = 0;
    g_queue_tail = 0;
    g_queue_count = 0;
}

// Set button event callback
void Button_SetCallback(button_callback_t callback)
{
    // Not used in simplified version
}

// Read individual button state (hardware inverted - buttons are active low)
static uint8_t Button_Read(uint8_t button)
{
    uint8_t state = 0;
    
    switch (button)
    {
        case BUTTON_UP:    // Button 0 - PC13
            state = !LL_GPIO_IsInputPinSet(BUTTON_0_PORT, BUTTON_0_PIN);
            break;
        case BUTTON_DOWN:  // Button 1 - PC14
            state = !LL_GPIO_IsInputPinSet(BUTTON_1_PORT, BUTTON_1_PIN);
            break;
        case BUTTON_SELECT: // Button 2 - PC15
            state = !LL_GPIO_IsInputPinSet(BUTTON_2_PORT, BUTTON_2_PIN);
            break;
        case BUTTON_BACK:  // Button 3 - PF0
            state = !LL_GPIO_IsInputPinSet(BUTTON_3_PORT, BUTTON_3_PIN);
            break;
        default:
            state = 0;
            break;
    }
    
    return state;
}

// ============ QUEUE FUNCTIONS ============

// Get button event from queue
uint8_t Button_GetEvent(button_event_queue_item_t *event)
{
    return Button_QueueGet(event);
}

// Check if queue is empty
uint8_t Button_IsQueueEmpty(void)
{
    return (g_queue_count == 0);
}

// Scan buttons and update states
void Button_Scan(void)
{
    // Increment tick counter (called every 10ms)
    g_button_tick_counter++;
    
    for (uint8_t i = 0; i < BUTTON_COUNT; i++)
    {
        uint8_t current_state = Button_Read(i);
        
        // Update state
        g_button_last_state[i] = g_button_state[i];
        g_button_state[i] = current_state;
    }
}

// Process button events and set flags
void Button_Process(void)
{
    for (uint8_t i = 0; i < BUTTON_COUNT; i++)
    {
        // Check if button just pressed (transition from LOW to HIGH after inversion)
        if (g_button_state[i] && !g_button_last_state[i] && !g_button_press_flag[i])
        {
            // Button just pressed - record time and set press flag
            g_button_press_time[i] = g_button_tick_counter;
            g_button_press_flag[i] = 1;
        }
        // Check if button is still pressed and enough time has passed for long press
        else if (g_button_state[i] && g_button_press_flag[i])
        {
            uint32_t delta_time = g_button_tick_counter - g_button_press_time[i];
            
            // If long press threshold reached, trigger long press immediately
            if (delta_time >= BUTTON_LONG_PRESS_THRESHOLD)
            {
                Button_QueuePut(i, BUTTON_EVENT_LONG_PRESS);
                // Reset press flag to prevent multiple triggers
                g_button_press_flag[i] = 0;
                g_button_press_time[i] = 0;
            }
        }
        // Check if button was released and we had recorded a press
        else if (!g_button_state[i] && g_button_last_state[i] && g_button_press_flag[i])
        {
            // Button was released - this is a short press
            Button_QueuePut(i, BUTTON_EVENT_SHORT_PRESS);
            
            // Reset press flag to start next cycle
            g_button_press_flag[i] = 0;
            g_button_press_time[i] = 0;
        }
    }
}
