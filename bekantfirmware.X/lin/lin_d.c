#include <pic16lf1938.h>
#include <pic.h>

#include "lin_d.h"

uint8_t lin_id;          // Identifier byte
uint8_t lin_data_count;  // Data count (bytes in the message)
uint8_t *lin_data;       // Pointer to data
uint8_t lin_protected_id;// Computed ID with parity bits
uint8_t lin_checksum;    // Computed checksum
void (*lin_frame_finish)(void);
uint8_t data_index;

LIN_FLAGS_t lin_flags;
LIN_ERROR_FLAGS_t lin_error_flags;

#define LIN_SYNC_BYTE 0x55

uint8_t lin_compute_protected_id(uint8_t id);
uint8_t lin_compute_checksum();

/**
 * The background LIN transmit/receive handler.
 * Expect to be called from receive interrupt.
 */
void lin_txrx_daemon() {
    // Reading the byte updates/clears flags like FERR
    uint8_t rxbyte = RCREG;

    switch (lin_frame_state) {
        case L_FRAME_BREAK:
            // send sync
            lin_frame_state = L_FRAME_SYNC;
            TXREG = LIN_SYNC_BYTE;
            break;

        case L_FRAME_SYNC:
            // send ID with parity (protected ID)
            lin_frame_state = L_FRAME_ID;
            TXREG = lin_protected_id;
            break;

        case L_FRAME_ID:
            // start tx/rx data
            lin_frame_state = L_FRAME_DATA;

            data_index = 0;
            // FALL THROUGH
        case L_FRAME_DATA:
            if (data_index < lin_data_count) {
                if (lin_flags.L_STATE_TX) { // true:TX false:RX
                    TXREG = lin_data[data_index];
                } else {
                    lin_data[data_index] = RCREG;
                }
                data_index += 1;
            } else {
                if (lin_flags.L_STATE_TX) { // true:TX false:RX
                    TXREG = lin_checksum;
                }

                lin_frame_state = L_FRAME_CHECKSUM;
            }
            break;

        case L_FRAME_CHECKSUM:
            if (!lin_flags.L_STATE_TX) { // RX
                lin_checksum = RCREG;
                // TODO validate checksum
            }

            lin_frame_state = L_FRAME_INTERFRAME;
            lin_flags.L_STATUS_BUSY = 0;
            lin_frame_finish();

            break;

        case L_FRAME_INTERFRAME:
            // some kind of error out-of-frame byte
            // LE_FTO ??
            break;
    }
    CLRWDT();
}

/**
 * Compute the checksum based on the current ID + parity and data.
 * lin_checksum populated with result.
 *
 * TODO
 * Only supports LIN 2.x enhanced checksums.
 * Support classic or enhanced checksums based on ID.
 *
 * @return checksum byte
 */
uint8_t lin_compute_checksum(void) {
    // TODO inline assembly
    uint8_t result = 0;

    result += lin_protected_id;

    for (uint8_t i = 0; i < lin_data_count; i++) {
        uint16_t sum = result + lin_data[i];
        result += lin_data[i];
        if (sum > 0xff) {
            result++;
        }
    }

    result = ~result;

    return result;
}


/**
 * Compute ID parity bits
 *
 * P0 = ID0 ^ ID1 ^ ID2 ^ ID4
 * P1 = !(ID1 ^ ID3 ^ ID4 ^ ID5)
 *
 *   P1 P0
 *   |  |
 * 76543210
 *   |  |     << 1
 * 6543210z 
 *   |  |     << 1
 * 543210zz
 *   |  |     Copy bit[6] to bit[0], which was originally ID4
 *   |  |     << 2
 * 3210z4zz
 * 
 * @return Top two MSB are parity bits P0 and P1
 */
uint8_t lin_compute_protected_id(uint8_t id) {
    // TODO inline assembly
    uint8_t temp = id;
    uint8_t parity = id; // space for parity xor, will use bits 2 and 5
    
    temp <<= 1; parity ^= temp;
    temp <<= 1; parity ^= temp;
    
    if (temp & 0b01000000) {
        temp |= 1 << 0;
    }
    // else assume shifted-in bits are already zero (logical shift)

    temp <<= 2; parity ^= temp;
    
    uint8_t result = (id & 0b00111111);
    if (parity & 0b00000100) {
        result |= 1 << 6;
    }
    
    // negated
    if (!(parity & 0b00100000)) {
        result |= 1 << 7;
    }
    
    return result;
}

/**
 * Shared routine for TX and RX
 *
 * Static parameters:
 * lin_id
 * lin_data_count
 * lin_data
 * 
 * TODO
 * Deal with LIN Sleep mode
 */
void lin_txrx() {
    lin_flags.L_STATUS_BUSY = 1;

    lin_protected_id = lin_compute_protected_id(lin_id);

    if (lin_flags.L_STATE_TX) {
        // Has to be done after the protected_id is computed,
        // otherwise it would have been part of lin_tx_frame()
        lin_checksum = lin_compute_checksum();
    }

    // Send break
    TXSTAbits.SENDB = 1;
    TXREG = 0;
    lin_frame_state = L_FRAME_BREAK;
}

/**
 * Receive LIN frame
 * 
 * Static parameters:
 * lin_id
 * lin_data_count
 * lin_data
 */
void lin_rx_frame() {
    if (!lin_flags.L_STATUS_BUSY) {
        lin_flags.L_STATE_TX = 0; // 0 means RX
        lin_txrx();
    }
}

/**
 * Send LIN frame
 * 
 * Static parameters:
 * lin_id
 * lin_data_count
 * lin_data
 */
void lin_tx_frame() {
    // TODO calculate ID parity and checksum before starting?
    
    if (!lin_flags.L_STATUS_BUSY) {
        lin_flags.L_STATE_TX = 1; // 1 means TX
        lin_txrx();
    }
}

void lin_reset_usart(void) {
    RCSTAbits.SPEN = 0; // Serial port disable (enabled below after everything configured)
    RCSTAbits.SPEN = 1; // Serial port enable
    
    TXSTAbits.TXEN = 0;
    TXSTAbits.TXEN = 1;
}

/**
 * Initializes all flags and resets the hardware used by LIN.
 */
void lin_init_hw() {
    lin_reset_usart();
    
    lin_data_count = 0;
    lin_flags = (LIN_FLAGS_t)(uint8_t)0;
    lin_error_flags = (LIN_ERROR_FLAGS_t)(uint8_t)0;
}
