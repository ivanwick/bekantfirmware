#include <xc.h> // XC8 General Include File

#include <stdint.h>
#include <stdbool.h>

#include "lin/lin_d.h"

void __interrupt() isr(void)
{
    if (PIR1bits.RCIF) {
        lin_txrx_daemon();
        PIR1bits.RCIF = 0;
    }
}
