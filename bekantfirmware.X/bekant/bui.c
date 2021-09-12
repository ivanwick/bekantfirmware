/**
 * BEKANT UI
 */

#include "bui.h"
#include "bctrl.h"

typedef enum {
    BUI_STOP,
    BUI_UP,
    BUI_MEM_UP,
    BUI_DOWN,
    BUI_MEM_DOWN
} BUI_state_t;

BUI_state_t bui_state = BUI_STOP;
int16_t high_pos;
int16_t low_pos;
int16_t cur_pos; // accessed by bui_input() and bui_set_pos()

typedef struct {
    int16_t low_pos;
    int16_t high_pos;
} BUI_saved_t;

//__EEPROM_DATA(0x36, 0x06, 0x00, 0x16, 0, 0, 0, 0);
__eeprom BUI_saved_t saved = {
    .high_pos = 0x1600,
    .low_pos = 0x0636
};

void bui_save_pos(int16_t save_pos);

/**
 * Initialize from stored position values.
 */
void bui_init(void) {
    low_pos = saved.low_pos;
    high_pos = saved.high_pos;
}

/**
 * 
 * @param input Input gesture
 */
void bui_input(INPUT_t input) {
    switch (bui_state) {
        case BUI_STOP:
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
            } else if (input == INPUT_SAVE) {
                bui_save_pos(cur_pos);
            }
            break;

        case BUI_UP:
            if (input == INPUT_MEM_UP && cur_pos < high_pos) {
                bui_state = BUI_MEM_UP;
            } else if (input == INPUT_IDLE) {
                bui_state = BUI_STOP;
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
                bui_state = BUI_STOP;
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
}

/**
 * Update this module's copy of the current position.
 * Expect to be called by BEKANT Control module after the LIN bus transaction
 * is completed and position is reported.
 * 
 * @param pos Current position
 */
void bui_set_pos(int16_t pos) {
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

/**
 * Store the given save_pos encoder value into persistent memory.
 * Overwrite stored low_pos or high_pos, whichever is closer to save_pos.
 * 
 * If the given save_pos looks like an encoder error state (negative),
 * then don't save.
 * 
 * @param save_pos new encoder value to store into persistent memory
 */
void bui_save_pos(int16_t save_pos) {
    if (save_pos < 0) {
        // Encoder value indicates error state
        return;
    }

    int16_t diff_high = abs(high_pos - save_pos);
    int16_t diff_low = abs(low_pos - save_pos);

    // Next time during a MEM move, BUI will send the STOP signal to BCTRL
    // before it reaches the target position, to allow for deceleration.
    if (diff_low < diff_high) {
        low_pos = save_pos + BCTRL_DECEL_MARGIN;
        saved.low_pos = low_pos;
    } else {
        high_pos = save_pos - BCTRL_DECEL_MARGIN;
        saved.high_pos = high_pos;
    }

    // Click feedback
    bctrl_set_target(BCTRL_CLICK);
}