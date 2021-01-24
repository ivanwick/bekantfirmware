
#include <stdint.h>

typedef enum {
    BCTRL_STOP = 0xfc,
    BCTRL_PRE_MOVE = 0xc4,
    BCTRL_UP = 0x86,
    BCTRL_DOWN = 0x85,
    BCTRL_PRE_STOP_A = 0x87,
    BCTRL_PRE_STOP_B = 0x84,
} BCTRL_state_t;

void bctrl_timer(void);
void bctrl_set_target(BCTRL_state_t state);
void bctrl_rx_lin(void);

extern void (*bctrl_report_pos)(uint16_t pos);
