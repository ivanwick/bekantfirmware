
#include <stdint.h>

typedef enum {
    BCTRL_AFTER_SCAN,
    BCTRL_STOP,
    BCTRL_PRE_MOVE,
    BCTRL_UP,
    BCTRL_DOWN,
    BCTRL_UP_DECEL,
    BCTRL_DOWN_DECEL,
    BCTRL_PRE_STOP,
} BCTRL_state_t;

void bctrl_timer(void);
void bctrl_set_target(BCTRL_state_t state);
void bctrl_rx_lin(void);
void bctrl_init(void);

extern void (*bctrl_report_pos)(int16_t pos);
