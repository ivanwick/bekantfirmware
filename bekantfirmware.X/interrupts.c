#include <xc.h> // XC8 General Include File

#include <stdint.h>
#include <stdbool.h>

#include "lin/lin_d.h"
#include "bekant/bctrl.h"
#include "btn/btn.h"

void __interrupt() isr(void)
{
    if (PIR1bits.RCIF) {
        lin_txrx_daemon();
        PIR1bits.RCIF = 0;
    } else if (PIR3bits.TMR4IF) {
        bctrl_timer();
        PIR3bits.TMR4IF = false;
    } else if (PIR1bits.TMR2IF) {
        btn_timer();
        PIR1bits.TMR2IF = false;
    }
}
