
#include <pic.h>
#include <stdbool.h>       /* For true/false definition */

#ifndef BTN_H
#define BTN_H

// A mapping to PORTB
// Determined by PCB traces
typedef union {
    PORTBbits_t PORTB;
    struct {
        unsigned DOWN : 1;
        unsigned UP : 1;
        unsigned : 6;
    };
} ButtonState_t;

bool btn_debounce(ButtonState_t now_btn);

typedef enum {
    INPUT_IDLE,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_MEM_UP,
    INPUT_MEM_DOWN,
    INPUT_SAVE,
} INPUT_t;

INPUT_t btn_gesture(ButtonState_t btn);

#endif /* BTN_H */
