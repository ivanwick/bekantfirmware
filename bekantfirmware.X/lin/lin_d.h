#include <stdint.h>

extern uint8_t lin_id;          // Identifier byte
extern uint8_t lin_data_count;  // Data count (bytes in the message)
extern uint8_t *lin_data;       // Pointer to data
extern uint8_t lin_checksum;    // Checksum calculation area
extern void (*lin_frame_finish)(void);    // Pointer to frame rx handler
extern void (*lin_err)(void);    // Pointer to frame error handler

void lin_txrx_daemon(void);         // Send a sync break signal
void lin_rx_frame(void);	               
void lin_tx_frame(void);                
void lin_time_update(void);             
void lin_init_hw(void);         // Receive and compare to the calculated checksum

typedef enum {
    L_FRAME_INTERFRAME,
    L_FRAME_BREAK,
    L_FRAME_SYNC,
    L_FRAME_ID,
    L_FRAME_DATA,
    L_FRAME_CHECKSUM,
} L_FRAME_STATE_t;

L_FRAME_STATE_t lin_frame_state;

typedef union {
    uint8_t LIN_FLAGS;
    struct {
        unsigned L_STATE_TX:1;
        unsigned L_STATUS_BUSY:1;
        unsigned L_STATUS_SLEEP:1;
        unsigned L_STATUS_RWAKE:1;
    };
} LIN_FLAGS_t;
extern LIN_FLAGS_t lin_flags;

typedef union {
    uint8_t LIN_ERROR_FLAGS;
    struct {
        unsigned LE_BIT:1;
        unsigned LE_CHKSM:1;
        unsigned LE_FTO:1;
        unsigned LE_BTO:1;
    };
} LIN_ERROR_FLAGS_t;
extern LIN_ERROR_FLAGS_t lin_error_flags;
