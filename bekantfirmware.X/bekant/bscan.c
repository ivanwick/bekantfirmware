/**
 * BEKANT Startup bus scan
 */

#include <stdint.h>
#include <stdbool.h>
#include <pic.h>
#include "bscan.h"
#include "../lin/lin_d.h"

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
    uint8_t *data;
} LIN_scan_message_t;

LIN_scan_message_t *scan_msg;

void lin_txrx_blocking(LIN_scan_message_t *msg);

LIN_scan_message_t scan_tx_msg = {
    .id = 0x3c,
    .data_count = 8,
    .tx = true
};
uint8_t rx_data[8];
LIN_scan_message_t scan_rx_msg = {
    .id = 0x3d,
    .data = rx_data,
    .data_count = 8,
    .tx = false
};


void bscan_init() {
    // LIN Frame timeout timer
    // for scan
    //
    // 4 Mhz / 16 prescaler / 250 period / 10 postscaler = 100 Hz
    //   0.01 sec
    //   10 msec
    T4CONbits.T4CKPS = 0b10; // Prescaler is 16
    PR4 = 250; // Period: 250
    T4CONbits.T4OUTPS = 0b1001; // 1:10 Postscaler

    PIE3bits.TMR4IE = 0; // Disable Timer4 interrupt
    T4CONbits.TMR4ON = 1; // Timer is on
}

uint8_t scan_data[][8] = {
    // first
    {0xff, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0xff, 0x01, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff},

    // slot: repeat with 0xd0, 0x00-0x07 in first byte
    {0x00, 0x02, 0x07, 0x00, 0xff, 0xff, 0xff, 0xff},
    {0x00, 0x06, 0x09, 0x00, 0xff, 0xff, 0xff, 0xff},
    {0x00, 0x06, 0x0c, 0x00, 0xff, 0xff, 0xff, 0xff},
    {0x00, 0x06, 0x0d, 0x00, 0xff, 0xff, 0xff, 0xff},
    {0x00, 0x06, 0x0a, 0x00, 0xff, 0xff, 0xff, 0xff},
    {0x00, 0x06, 0x0b, 0x00, 0xff, 0xff, 0xff, 0xff},
    {0x00, 0x04, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff},
    
    // last
    {0xd0, 0x01, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0xd0, 0x02, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff},
};

#define SLOT_MIN 2
#define SLOT_MAX 8

// Just ints from 0-7 except for prepending 0xd0
uint8_t slot_sequence[] = {0xd0, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

bool bscan_scan_slot_data(uint8_t slot_num, uint8_t data_index) {
    scan_tx_msg.data = scan_data[data_index];
    scan_tx_msg.data[0] = slot_num;
    lin_txrx_blocking(&scan_tx_msg);
    lin_txrx_blocking(&scan_rx_msg);

    // check
    return scan_rx_msg.status == 1;
}

bool bscan_scan_slot_first(uint8_t slot_num) {
    return bscan_scan_slot_data(slot_num, SLOT_MIN);
}

bool bscan_scan_slot_rest(uint8_t slot_num, uint8_t assign_maybe) {
    // 3-8 inclusive
    for (uint8_t index = SLOT_MIN+1; index <= SLOT_MAX; index++) {
        if (index == SLOT_MAX) {
            scan_data[index][2] = assign_maybe;
        }
        bool ok = bscan_scan_slot_data(slot_num, index);
        if (!ok) {
            return false;
        }
    }
    
    return true;
}

void bscan_scan(void) {
    bool scan_ok = false;

    scan_tx_msg.data = scan_data[0];
    lin_txrx_blocking(&scan_tx_msg);
    lin_txrx_blocking(&scan_rx_msg);

    while (!scan_ok) {
        scan_tx_msg.data = scan_data[0];
        lin_txrx_blocking(&scan_tx_msg);
        lin_txrx_blocking(&scan_rx_msg);

        scan_tx_msg.data = scan_data[1];
        lin_txrx_blocking(&scan_tx_msg);
        lin_txrx_blocking(&scan_rx_msg);

        uint8_t slot_ok_index = 0;
        for (uint8_t index = 0; index <= 8; index++) {
            bool slot_started = false;
            uint8_t slot_num = slot_sequence[index];
            slot_started = bscan_scan_slot_first(slot_num);
            if (!slot_started) {
                continue;
            }
            bool slot_ok = bscan_scan_slot_rest(slot_num, slot_ok_index);
            if (!slot_ok) {
                scan_ok = false;
                break;
            } else {
                if (rx_data[1] == slot_ok_index)
                slot_ok_index += 1;
            }
        }
        
        if (slot_ok_index == 2) {
            scan_ok = true;
        }
    }

    // last
    scan_tx_msg.data = scan_data[9];
    lin_txrx_blocking(&scan_tx_msg);
    lin_txrx_blocking(&scan_rx_msg);

    scan_tx_msg.data = scan_data[10];
    lin_txrx_blocking(&scan_tx_msg);
    lin_txrx_blocking(&scan_rx_msg);
}

void lin_txrx_blocking(LIN_scan_message_t *msg) {
    lin_id = msg->id;
    lin_data_count = msg->data_count;
    lin_data = (uint8_t*)msg->data;
    if (msg->tx) {
        lin_tx_frame();
    } else {
        lin_rx_frame();
    }
    TMR4 = 0; // restart timer
    PIR3bits.TMR4IF = false;
    CLRWDT();
    while (!PIR3bits.TMR4IF) {
        // busy wait
    }
    PIR3bits.TMR4IF = false;

    if (lin_flags.L_STATUS_BUSY) {
        // If still busy, reinit
        msg->status = 0;
        lin_reset_frame();
    } else {
        msg->status = 1;
    }
}
