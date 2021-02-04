#include <xc.h> // XC8 General Include File

#include <stdint.h>
#include <stdbool.h>

#include "system.h"        // System funct/params, like osc/peripheral config
#include "user.h"          // User funct/params, such as InitApp

#include <pic.h>

void main(void) {
    /* Configure the oscillator for the device */
    ConfigureOscillator();

    /* Initialize I/O and Peripherals for application */
    InitApp();

    TRISB = 0b00000000;

    bool ledStatus = false;

    while (1) {
        if (ledStatus) {
            PORTB = 0b01000000;
        } else {
            PORTB = 0b00000000;
        }
        ledStatus = !ledStatus;

        CLRWDT();
        while (!PIR1bits.TMR2IF);
        PIR1bits.TMR2IF = false;
    }
}
