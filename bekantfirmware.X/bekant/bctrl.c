/**
 * BEKANT Control
 */

#include <stdlib.h>
#include <stdbool.h>
#include "bctrl.h"
#include "../lin/lin_d.h"

void (*bctrl_report_pos)(uint16_t pos);

uint16_t bctrl_pos;
uint8_t bctrl_status_08;
uint8_t bctrl_status_09;

typedef union {
    uint8_t data[3];
    struct {
        uint16_t encoder;
        uint8_t status;
    };
} BCTRL_bus_data_t;

BCTRL_bus_data_t zeroes = {0, 0, 0};
BCTRL_bus_data_t data_space_08;
BCTRL_bus_data_t data_space_09;
BCTRL_bus_data_t data_space_12;

typedef struct {
    union {
        uint8_t flags_and_id;
        struct {
            unsigned tx:1;
            unsigned status:1; // whether frame was received OK
            unsigned id:6;
        };
    };
    uint8_t data_count;
    BCTRL_bus_data_t *data;
} LIN_bus_message_t;

LIN_bus_message_t bus_schedule[] = {
    { .tx = true, .id = 0x11, .data_count = 3, .data = &zeroes },
    { .tx = false, .id = 0x08, .data_count = 3, .data = &data_space_08 },
    { .tx = false, .id = 0x09, .data_count = 3, .data = &data_space_09 },
    { .tx = false, .id = 0x10, .data_count = 0, .data = NULL },
    { .tx = false, .id = 0x10, .data_count = 0, .data = NULL },
    { .tx = false, .id = 0x10, .data_count = 0, .data = NULL },
    { .tx = false, .id = 0x10, .data_count = 0, .data = NULL },
    { .tx = false, .id = 0x10, .data_count = 0, .data = NULL },
    { .tx = false, .id = 0x10, .data_count = 0, .data = NULL },
    { .tx = false, .id = 0x01, .data_count = 0, .data = NULL },
    { .tx = true, .id = 0x12, .data_count = 3, .data = &data_space_12 },

    // Nothing on the bus responds to ID 0x10, so the repeated RX frames
    // for 0x10 might be some kind of retry logic.
    // TODO
    // See if it works after removing RX frames from 0x10
};

// Number of 5 ms slots in the bus schedule which are occupied by a frame
#define SCHEDULE_OCCUPIED_COUNT 11
#define SCHEDULE_TOTAL_COUNT 20

BCTRL_state_t current_state = BCTRL_STOP;
BCTRL_state_t target_state = BCTRL_STOP;

void bctrl_next_state(void) {
    switch (current_state) {
        case BCTRL_STOP:
            if (target_state == BCTRL_UP || target_state == BCTRL_DOWN) {
                current_state = BCTRL_PRE_MOVE;
            }
            break;

        case BCTRL_PRE_MOVE:
            if (target_state == BCTRL_UP || target_state == BCTRL_DOWN) {
                current_state = target_state;
            }
            break;

        case BCTRL_UP:
        case BCTRL_DOWN:
            if (target_state != current_state || target_state == BCTRL_STOP) {
                current_state = BCTRL_PRE_STOP_A;
            }
            break;

        case BCTRL_PRE_STOP_A:
            current_state = BCTRL_PRE_STOP_B;
            break;

        case BCTRL_PRE_STOP_B:
            current_state = BCTRL_STOP;
            break;
    }
}

void bctrl_timer(void) {
    static uint8_t schedule_pos = 0;
    if (schedule_pos < SCHEDULE_OCCUPIED_COUNT) {
        LIN_bus_message_t msg = bus_schedule[schedule_pos];

        // Check if last frame ever finished
        if (lin_flags.L_STATUS_BUSY) {
            // If still busy, reinit
            msg.status = 0;
            lin_init_hw(); // TODO rename to lin_reset_frame ?
            // schedule_pos = 0;
            // return;
        } else {
            msg.status = 1;
        }

        lin_id = msg.id;
        lin_data_count = msg.data_count;
        lin_data = (uint8_t*)msg.data;
        if (msg.tx) {
            lin_tx_frame();
        } else {
            lin_rx_frame();
        }
    }

    schedule_pos++;
    if (schedule_pos >= SCHEDULE_TOTAL_COUNT) {
        // Collect data from bus transaction
        // TODO take from 0x08 or 0x09?
        bctrl_status_08 = data_space_08.status;
        bctrl_status_09 = data_space_09.status;

        // BUI might call back bctrl_set_target to change target state
        // BCTRL_STOP based on the position
        bctrl_pos = data_space_08.encoder; // take from 0x08 for now
        bctrl_report_pos(bctrl_pos);

        // Finish bus transaction
        schedule_pos = 0;
        bctrl_next_state();
        // TODO if settled at stop state (current/target) then disable
        // timer to prevent continuous non-moving transactions
        // Possibly go into sleep mode?
    }
}

void bctrl_set_target(BCTRL_state_t state) {

    // Note this is switching on the NEW target state, not the current state
    switch (state) {
        case BCTRL_STOP:
            target_state = state;
            break;
        case BCTRL_UP:
        case BCTRL_DOWN:
            if (current_state == BCTRL_STOP) {
                // enable timer because we are coming out of BCTRL_STOP state
                // and will need to start doing a bus transaction

                // TODO enable timer
            }
            target_state = state;
            break;

        default:
            // Don't allow intermediate states (PRE_STOP/PRE_MOVE) as target
            break;
    }
}

void bctrl_rx_lin(void) {
    // ??
}
