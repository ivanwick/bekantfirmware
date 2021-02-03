/**
 * BEKANT Control
 */

#include <stdlib.h>
#include <stdbool.h>
#include "bctrl.h"
#include "../lin/lin_d.h"
#include <pic.h>

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
#define SCHEDULE_COMMAND_SLOT 10

BCTRL_state_t current_state = BCTRL_AFTER_SCAN;
BCTRL_state_t target_state = BCTRL_STOP;

#define PRE_STOP_RAMPDOWN 3

BCTRL_state_t last_move = BCTRL_DOWN;

uint8_t pre_stop_counter = 0;
uint16_t overshoot_target = 0;

void bctrl_next_state(void) {
    switch (current_state) {
        case BCTRL_AFTER_SCAN:
            current_state = BCTRL_STOP;
            break;

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
            last_move = current_state;
            if (target_state != current_state || target_state == BCTRL_STOP) {
                current_state = BCTRL_PRE_STOP_A;
                pre_stop_counter = 0;
            }
            break;

        case BCTRL_PRE_STOP_A:
            pre_stop_counter += 1;
            if (pre_stop_counter >= PRE_STOP_RAMPDOWN) {
                current_state = BCTRL_PRE_STOP_B;
            }
            break;

        case BCTRL_PRE_STOP_B:
            current_state = BCTRL_STOP;
            break;
    }

    // Set up next transmitted frame for ID 0x12
    data_space_12.encoder = bctrl_pos;
    data_space_12.status = current_state;
}

void bctrl_timer(void) {
    static uint8_t schedule_pos = 0;
    if (schedule_pos < SCHEDULE_OCCUPIED_COUNT) {
        LIN_bus_message_t msg = bus_schedule[schedule_pos];

        // Check if last frame ever finished
        if (lin_flags.L_STATUS_BUSY) {
            // If still busy, reinit
            msg.status = 0;
            lin_reset_frame();
            // schedule_pos = 0;
            // return;
        } else {
            msg.status = 1;
        }

        if (schedule_pos == SCHEDULE_COMMAND_SLOT) {
            // populate command frame based on the most recent
            // 0x08 and 0x09 frames
            switch (current_state) {
                case BCTRL_PRE_STOP_A:
                    if (pre_stop_counter == 0) {
                        // calculate overshoot value based on last_move
                        if (last_move == BCTRL_UP) {
                            // add overshoot
                            if (data_space_08.encoder < data_space_09.encoder) {
                                overshoot_target = data_space_09.encoder + 0x6c;
                            } else {
                                overshoot_target = data_space_08.encoder + 0x6c;
                            }

                        } else {
                            // subtract overshoot
                            if (data_space_08.encoder < data_space_09.encoder) {
                                overshoot_target = data_space_08.encoder - 0x6c;
                            } else {
                                overshoot_target = data_space_09.encoder - 0x6c;
                            }
                        }
                    }
                    msg.data->encoder = overshoot_target;
                    break;
                    
                case BCTRL_UP:
                    if ((data_space_08.encoder & 0x8000) != 0 ||
                            (data_space_09.encoder & 0x8000) != 0 ) {
                        // error condition?
                        // take greater encoder value
                        if (data_space_08.encoder < data_space_09.encoder) {
                            msg.data->encoder = data_space_09.encoder;
                        } else {
                            msg.data->encoder = data_space_08.encoder;
                        }
                        msg.data->status = 0xbc;
                        break;
                    }
                    
                    // take lesser encoder value
                    if (data_space_08.encoder < data_space_09.encoder) {
                        msg.data->encoder = data_space_08.encoder;
                    } else {
                        msg.data->encoder = data_space_09.encoder;
                    }
                    break;
                    
                case BCTRL_AFTER_SCAN:
                    msg.data->encoder = 0xfff6;
                    msg.data->status = BCTRL_AFTER_SCAN;
                    break;
                    
                case BCTRL_DOWN:
                    if ((data_space_08.encoder & 0x8000) != 0 ||
                            (data_space_09.encoder & 0x8000) != 0 ) {
                        // error condition?
                        // encoder zero when OEM_DOWN_SLOW
                        msg.data->encoder = 0;
                        msg.data->status = 0xbd;
                    } else {
                        // take greater encoder value
                        if (data_space_08.encoder < data_space_09.encoder) {
                            msg.data->encoder = data_space_09.encoder;
                        } else {
                            msg.data->encoder = data_space_08.encoder;
                        }
                    }
                    break;
                default:
                    // take greater encoder value
                    if (data_space_08.encoder < data_space_09.encoder) {
                        msg.data->encoder = data_space_09.encoder;
                    } else {
                        msg.data->encoder = data_space_08.encoder;
                    }
                    break;
            }
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

void bctrl_init(void) {
    // LIN Frame timeout timer
    // Slave must respond within 10ms after BREAK+SYNC+ID
    // or at least start responding
    // typical response starts after 225 us
    // full LIN frame takes ~4.225 ms
    // Each LIN frame starts ~5ms
    //

    // 4 Mhz / 16 prescaler / 125 period / 10 postscaler = 200 Hz
    //   0.005 sec
    //   5 msec
    T4CONbits.T4CKPS = 0b10; // Prescaler is 16
    PR4bits.PR4 = 0x7d; // Period: 125
    T4CONbits.T4OUTPS = 0b1001; // 1:10 Postscaler

    T4CONbits.TMR4ON = 1; // Timer is on
    PIE3bits.TMR4IE = 1; // Enable Timer4 interrupt
}
