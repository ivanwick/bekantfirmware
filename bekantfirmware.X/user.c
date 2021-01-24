/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#if defined(__XC)
    #include <xc.h>         /* XC8 General Include File */
#elif defined(HI_TECH_C)
    #include <htc.h>        /* HiTech General Include File */
#endif

#include <stdint.h>         /* For uint8_t definition */
#include <stdbool.h>        /* For true/false definition */

#include "user.h"
#include "lin/lin_d.h"
#include "bekant/bctrl.h"
#include "bekant/bui.h"

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/
void InitUsart(void);

/* <Initialize variables in user.h and insert code for user algorithms.> */

void InitApp(void)
{
    // hook up LIN driver to BEKANT control module
    lin_frame_finish = bctrl_rx_lin;
    // hook up BEKANT control module to BEKANT UI module
    bctrl_report_pos = bui_set_pos;

    /* TODO Initialize User Ports/Peripherals/Project here */

    /* Setup analog functionality and port direction */

    /* Initialize peripherals */
    InitUsart();
    
    /* Enable interrupts */
}

void InitUsart(void) {
    RCSTAbits.SPEN = 0; // Serial port disable (enabled below after everything configured)

    TXSTAbits.SYNC = 0; // Async mode
    TXSTAbits.BRGH = 1; // High speed

    BAUDCONbits.BRG16 = 1; // 16-bit baud rate generator
    // Set baud rate generator frequency 0x00cf
    SPBRGL = 0xcf;
    SPBRGH = 0;
    BAUDCONbits.WUE = 0; // async receiver operates normally
    BAUDCONbits.ABDEN = 0; // Auto-baud detect mode disabled

    TXSTAbits.SENDB = 0; // Sync Break transmission completed

    RCSTAbits.ADDEN = 0; // Disable address detection
    // All bytes are received and  ninth bit can be used as parity bit
    RCSTAbits.CREN = 1; // Enable receiver
    TXSTAbits.TXEN = 0; // Transmit disabled ???

    PIR1bits.RCIF = 0; // clear Receive interrupt flag (initially)
    PIE1bits.RCIE = 1; // USART Receive interrupt enable
    
    INTCONbits.PEIE = 1; // Enable peripheral interrupts
    INTCONbits.GIE = 1; // Enable global interrupts

    BAUDCONbits.SCKP = 1; // invert

    RCSTAbits.SPEN = 1; // Serial port enable
}
