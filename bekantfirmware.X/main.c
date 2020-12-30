#include <xc.h> // XC8 General Include File

#include <stdint.h>
#include <stdbool.h>

#include "system.h"        // System funct/params, like osc/peripheral config
#include "user.h"          // User funct/params, such as InitApp

#include "btn/btn.h"

#include <pic.h>

void main(void) {
    /* Configure the oscillator for the device */
    ConfigureOscillator();

    /* Initialize I/O and Peripherals for application */
    InitApp();
    
    TXSTAbits.TXEN = 1; // Transmit enabled

    ANSELB = 0b00000000;
    TRISB = 0b00000011;
    INPUT_t last_input = INPUT_IDLE;
    
    while (1) {
        ButtonState_t button_state = (ButtonState_t)PORTBbits;
        
        if (btn_debounce(button_state)) {
            INPUT_t input = btn_gesture(button_state);
            
            if (input != last_input) {
                TXREG = input;
                last_input = input;
            }
        }
        
        CLRWDT();
        while (!PIR1bits.TMR2IF);
        PIR1bits.TMR2IF = false;
    }
}
