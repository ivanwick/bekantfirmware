#include <xc.h> // XC8 General Include File

#include <stdint.h>
#include <stdbool.h>

#include "system.h"        // System funct/params, like osc/peripheral config
#include "user.h"          // User funct/params, such as InitApp

#include "btn/btn.h"
#include "lin/lin_d.h"
#include "bekant/bscan.h"
#include "bekant/bui.h"
#include "bekant/bctrl.h"

#include <pic.h>

void main(void) {
    /* Configure the oscillator for the device */
    ConfigureOscillator();

    /* Initialize I/O and Peripherals for application */
    InitApp();
    
    TXSTAbits.TXEN = 1; // Transmit enabled

    ANSELB = 0b00000000;
    TRISB = 0b00000011;
    
    LATC = 1;  // Enables LIN input to echo back from the bus
    TRISC = 0b10000000; // C7: input (serial RX)

    lin_init_hw();

    bscan_init();
    bscan_scan();

    bctrl_init();

    bui_init();
    
    btn_init();

    // TEST CODE TO DEBUG IN SIMULATOR
    //lin_id = 0x11;
    //lin_tx_frame();
    
    //lin_id = 0x12;
    //lin_data_count = 3;
    //lin_data = (uint8_t[]){0, 0, 0};
    //lin_tx_frame();
    
    while (1) {
        CLRWDT();
    }
}
