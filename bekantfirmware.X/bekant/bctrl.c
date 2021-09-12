/**
 * BEKANT Control
 */

#include <stdlib.h>
#include <stdbool.h>
#include "bctrl.h"
#include "../lin/lin_d.h"
#include <pic.h>

void (*bctrl_report_pos)(int16_t pos);
void bctrl_populate_cmd(void);
void bctrl_next_state(void);

int16_t bctrl_pos;
uint8_t bctrl_status_08;
uint8_t bctrl_status_09;
BCTRL_state_t current_state = BCTRL_AFTER_SCAN;
BCTRL_state_t target_state = BCTRL_STOP;

uint8_t decel_counter = 0;
int16_t decel_target = 0;

typedef union {
    uint8_t data[3];
    struct {
        int16_t encoder;
        uint8_t status;
    };
} BCTRL_bus_data_t;

BCTRL_bus_data_t zeroes = {0, 0, 0};
BCTRL_bus_data_t data_space_08;
BCTRL_bus_data_t data_space_09;
BCTRL_bus_data_t cmd_data;

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
    { .tx = true, .id = 0x12, .data_count = 3, .data = &cmd_data },

    // Nothing on the bus responds to ID 0x10, so the repeated RX frames
    // for 0x10 might be some kind of retry logic.
    // TODO
    // See if it works after removing RX frames from 0x10
};

// Number of 5 ms slots in the bus schedule which are occupied by a frame
#define SCHEDULE_OCCUPIED_COUNT 11
#define SCHEDULE_TOTAL_COUNT 20
#define SCHEDULE_COMMAND_SLOT 10

#define DECEL_COUNT_MAX 3 // How many decel frames to send

enum {
    BCMD_AFTER_SCAN_INIT = 0xbf,
    BCMD_STOP_IDLE = 0xfc,
    BCMD_PRE_MOVE = 0xc4,
    BCMD_UP = 0x86,
    BCMD_DOWN = 0x85,
    BCMD_DECEL = 0x87,
    BCMD_PRE_STOP = 0x84,
    BCMD_ERR_DOWN = 0xbd,
    BCMD_ERR_UP = 0xbc,
};

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
            if (target_state != current_state) {
                current_state = BCTRL_UP_DECEL;
                decel_counter = 0;
            }
            break;
        case BCTRL_DOWN:
            if (target_state != current_state) {
                current_state = BCTRL_DOWN_DECEL;
                decel_counter = 0;
            }
            break;

        case BCTRL_UP_DECEL:
        case BCTRL_DOWN_DECEL:
            decel_counter += 1;
            if (decel_counter >= DECEL_COUNT_MAX) {
                current_state = BCTRL_PRE_STOP;
            }
            break;

        case BCTRL_PRE_STOP:
            current_state = BCTRL_STOP;
            break;
    }
}

void bctrl_timer(void) {
    static uint8_t schedule_pos = 0;

    if (schedule_pos > 0) {
        // Set status of prior frame
        // At schedule_pos == 0, there is no prior frame
        uint8_t prior_frame_pos = schedule_pos - 1;
        if (prior_frame_pos < SCHEDULE_OCCUPIED_COUNT) {
            LIN_bus_message_t* prior_msg = &bus_schedule[prior_frame_pos];

            // Check if last frame ever finished
            if (lin_flags.L_STATUS_BUSY) {
                // If still busy, reinit
                prior_msg->status = 0;
                lin_reset_frame();
            } else {
                prior_msg->status = 1;
            }
        }
    }

    if (schedule_pos < SCHEDULE_OCCUPIED_COUNT) {
        LIN_bus_message_t* msg = &bus_schedule[schedule_pos];
        if (schedule_pos == SCHEDULE_COMMAND_SLOT) {
            // Collect data from bus transaction so far
            bctrl_status_08 = data_space_08.status;
            bctrl_status_09 = data_space_09.status;
            bctrl_populate_cmd();
        }

        lin_id = msg->id;
        lin_data_count = msg->data_count;
        lin_data = (uint8_t*)msg->data;
        if (msg->tx) {
            lin_tx_frame();
        } else {
            lin_rx_frame();
        }
    }

    schedule_pos++;
    if (schedule_pos >= SCHEDULE_TOTAL_COUNT) {
        // BUI might call back bctrl_set_target to change target state
        // BCTRL_STOP based on the position
        bctrl_pos = cmd_data.encoder; // encoder val from whatever cmd sent
        bctrl_report_pos(bctrl_pos);

        // Finish bus transaction
        schedule_pos = 0;
        bctrl_next_state();
        // TODO if settled at stop state (current/target) then disable
        // timer to prevent continuous non-moving transactions
        // Possibly go into sleep mode?
    }
}

inline int16_t min(int16_t a, int16_t b) {
    return a < b ? a : b;
}

inline int16_t max(int16_t a, int16_t b) {
    return a > b ? a : b;
}

/**
 * Populate command frame based on the most recent 0x08 and 0x09 frames
 */
void bctrl_populate_cmd() {
    int16_t encoder_min = min(data_space_08.encoder, data_space_09.encoder);
    int16_t encoder_max = max(data_space_08.encoder, data_space_09.encoder);
    bool error_cond = (
            data_space_08.encoder < 0 ||
            data_space_09.encoder < 0
            );

    switch (current_state) {
        case BCTRL_AFTER_SCAN:
            cmd_data.encoder = (int16_t)0xfff6;
            cmd_data.status = BCMD_AFTER_SCAN_INIT;
            break;

        case BCTRL_UP_DECEL:
            if (decel_counter == 0) {
                // add overshoot to max
                decel_target = encoder_max + BCTRL_DECEL_MARGIN;
            }

            cmd_data.encoder = decel_target;
            cmd_data.status = BCMD_DECEL;
            break;

        case BCTRL_DOWN_DECEL:
            if (decel_counter == 0) {
                // subtract overshoot from min
                decel_target = encoder_min - BCTRL_DECEL_MARGIN;
            }

            cmd_data.encoder = decel_target;
            cmd_data.status = BCMD_DECEL;
            break;

        case BCTRL_UP:
            if (error_cond) {
                // take greater encoder value
                cmd_data.encoder = encoder_max;
                cmd_data.status = BCMD_ERR_UP;
            } else {
                // take lesser encoder value
                cmd_data.encoder = encoder_min;
                cmd_data.status = BCMD_UP;
            }
            break;

        case BCTRL_DOWN:
            if (error_cond) {
                // encoder zero when OEM_DOWN_SLOW
                cmd_data.encoder = 0;
                cmd_data.status = BCMD_ERR_DOWN;
            } else {
                // take greater encoder value
                cmd_data.encoder = encoder_max;
                cmd_data.status = BCMD_DOWN;
            }
            break;

        case BCTRL_STOP:
            cmd_data.encoder = encoder_max;
            cmd_data.status = BCMD_STOP_IDLE;
            break;
        case BCTRL_PRE_MOVE:
            cmd_data.encoder = encoder_max;
            cmd_data.status = BCMD_PRE_MOVE;
            break;
        case BCTRL_PRE_STOP:
            cmd_data.encoder = encoder_max;
            cmd_data.status = BCMD_PRE_STOP;
            break;
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
