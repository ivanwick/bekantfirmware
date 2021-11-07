#include <xc.h> // XC8 General Include File

#include <stdint.h>
#include <stdbool.h>

#include "system.h"

void ConfigureOscillator(void)
{
    OSCCONbits.IRCF = 0b1111; // 16 MHz HF
    OSCCONbits.SCS = 0b11; // Internal oscillator block

    // Watchdog timer has an independent clock source
    WDTCONbits.WDTPS = 0b00101; // 1:1024 (Interval 32ms)
    WDTCONbits.SWDTEN = 1; // WDT turned on
    // 32 ms @ Fosc/4 (instruction timer) = 
    // 32 ms * 4 Mhz = 128_000 instructions before watchdog reset
    
    OPTION_REGbits.nWPUEN = 1; // !WPUEN Weak Pull ups disabled
    OPTION_REGbits.INTEDG = 1; // INTEDG Interrupt on rising edge of INT pin
    OPTION_REGbits.TMR0CS = 0; // TMR0CS Timer0 clock source: Internal Instruction cycle clock (Fosc/4)
    OPTION_REGbits.TMR0SE = 1; // TMR0SE Increment on high-to-low transition on T0CKI pin
    OPTION_REGbits.PSA = 0; // PSA Prescaler is assigned to the Timer0 module
    OPTION_REGbits.PS = 0b010; // 010 Prescaler 1:8
}
