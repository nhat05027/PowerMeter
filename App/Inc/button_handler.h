#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <stdint.h>
#include "board.h"

// Button definitions
#define BUTTON_COUNT    4

// Button indices
#define BUTTON_UP       0   // PC13 - Button 0
#define BUTTON_DOWN     1   // PC14 - Button 1  
#define BUTTON_SELECT   2   // PC15 - Button 2
#define BUTTON_BACK     3   // PF0  - Button 3

// Button events
typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_SHORT_PRESS,
    BUTTON_EVENT_LONG_PRESS
} button_event_t;

// Button event structure for queue
typedef struct {
    uint8_t button;
    button_event_t event;
} button_event_queue_item_t;

// Queue configuration
#define BUTTON_QUEUE_SIZE   8

// Button event callback function type
typedef void (*button_callback_t)(uint8_t button, button_event_t event);

// Button handler functions
void Button_Init(void);
void Button_Scan(void);
void Button_Process(void);

// Queue functions
uint8_t Button_GetEvent(button_event_queue_item_t *event);
uint8_t Button_IsQueueEmpty(void);


#endif // BUTTON_HANDLER_H
