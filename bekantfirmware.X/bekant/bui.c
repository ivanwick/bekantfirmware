/**
 * BEKANT UI
 */

#include "bui.h"
#include "bctrl.h"
#include <pic.h>

typedef enum {
    BUI_STOP,
    BUI_UP,
    BUI_MEM_UP,
    BUI_DOWN,
    BUI_MEM_DOWN
} BUI_state_t;

BUI_state_t bui_state = BUI_STOP;
uint16_t high_pos;
uint16_t low_pos;
uint16_t cur_pos; // accessed by bui_input() and bui_set_pos()


/**
 * Set this module's stored position values.
 * Expect to be called during initialization with values from EEPROM.
 * 
 * @param init_low_pos
 * @param init_high_pos
 */
void bui_init_pos(uint16_t init_low_pos, uint16_t init_high_pos) {
    low_pos = init_low_pos;
    high_pos = init_high_pos;
}

/**
 * 
 * @param input Input gesture
 */
void bui_input(INPUT_t input) {
    LATBbits.LATB6 = 1; // LATB |= (1 << 6);

    switch (bui_state) {
        case BUI_STOP:
            LATBbits.LATB7 = 1; // LATB |= (1 << 7);

            if (input == INPUT_UP) {
                bui_state = BUI_UP;
                bctrl_set_target(BCTRL_UP);
            } else if (input == INPUT_DOWN) {
                bui_state = BUI_DOWN;
                bctrl_set_target(BCTRL_DOWN);
            } else if (input == INPUT_MEM_UP && cur_pos < high_pos) {
                bui_state = BUI_MEM_UP;
                bctrl_set_target(BCTRL_UP);
            } else if (input == INPUT_MEM_DOWN && cur_pos > low_pos) {
                bui_state = BUI_MEM_DOWN;
                bctrl_set_target(BCTRL_DOWN);
            }

            LATBbits.LATB7 = 0; // LATB &= ~(1 << 7);
            break;

        case BUI_UP:
            if (input == INPUT_MEM_UP && cur_pos < high_pos) {
                bui_state = BUI_MEM_UP;
            } else if (input == INPUT_IDLE) {
                bctrl_set_target(BCTRL_STOP);
            } else if (input == INPUT_DOWN) {
                bui_state = BUI_DOWN;
                bctrl_set_target(BCTRL_DOWN);
            }
            break;

        case BUI_DOWN:
            if (input == INPUT_MEM_DOWN && cur_pos > low_pos) {
                bui_state = BUI_MEM_DOWN;
            } else if (input == INPUT_IDLE) {
                bctrl_set_target(BCTRL_STOP);
            } else if (input == INPUT_UP) {
                bui_state = BUI_UP;
                bctrl_set_target(BCTRL_UP);
            }
            break;

        case BUI_MEM_UP:
            if (input == INPUT_UP) {
                bui_state = BUI_UP;
            } else if (input == INPUT_DOWN) {
                bui_state = BUI_DOWN;
                bctrl_set_target(BCTRL_DOWN); // switch directions
            }
            break;

        case BUI_MEM_DOWN:
            if (input == INPUT_UP) {
                bui_state = BUI_UP;
                bctrl_set_target(BCTRL_UP); // switch directions
            } else if (input == INPUT_DOWN) {
                bui_state = BUI_DOWN;
            }
            break;
    }
    LATBbits.LATB6 = 0; // LATB &= ~(1 << 6);
}

/**
 * Update this module's copy of the current position.
 * Expect to be called by BEKANT Control module after the LIN bus transaction
 * is completed and position is reported.
 * 
 * @param pos Current position
 */
void bui_set_pos(uint16_t pos) {
    cur_pos = pos;
    switch (bui_state) {
        case BUI_MEM_UP:
            if (cur_pos >= high_pos) {
                bui_state = BUI_STOP;
                bctrl_set_target(BCTRL_STOP);
            }
            break;
        case BUI_MEM_DOWN:
            if (cur_pos <= low_pos) {
                bui_state = BUI_STOP;
                bctrl_set_target(BCTRL_STOP);
            }
            break;
        default:
            break;
    }
}
