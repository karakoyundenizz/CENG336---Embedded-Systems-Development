// ============================ //
// Do not edit this part!!!!    //
// ============================ //
// 0x300001 - CONFIG1H
#pragma config OSC = HSPLL  // Oscillator Selection bits (HS oscillator,
                            // PLL enabled (Clock Frequency = 4 x FOSC1))
#pragma config FCMEN = OFF  // Fail-Safe Clock Monitor Enable bit
                            // (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF   // Internal/External Oscillator Switchover bit
                            // (Oscillator Switchover mode disabled)
// 0x300002 - CONFIG2L
#pragma config PWRT = OFF   // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF  // Brown-out Reset Enable bits (Brown-out
                            // Reset disabled in hardware and software)
// 0x300003 - CONFIG1H
#pragma config WDT = OFF  // Watchdog Timer Enable bit
                          // (WDT disabled (control is placed on the SWDTEN bit))
// 0x300004 - CONFIG3L
// 0x300005 - CONFIG3H
#pragma config LPT1OSC = OFF  // Low-Power Timer1 Oscillator Enable bit
                              // (Timer1 configured for higher power operation)
#pragma config MCLRE = ON     // MCLR Pin Enable bit (MCLR pin enabled;
                              // RE3 input pin disabled)
// 0x300006 - CONFIG4L
#pragma config LVP = OFF    // Single-Supply ICSP Enable bit (Single-Supply
                            // ICSP disabled)
#pragma config XINST = OFF  // Extended Instruction Set Enable bit
                            // (Instruction set extension and Indexed
                            // Addressing mode disabled (Legacy mode))
#pragma config DEBUG = OFF  // Disable In-Circuit Debugger
#define KHZ 1000UL
#define MHZ (KHZ * KHZ)
#define _XTAL_FREQ (40UL * MHZ)
// ============================ //
//             End              //
// ============================ //

#include <stdint.h>
#include <xc.h>


// ============================ //
//  Constants & Lookup Tables   //
// ============================ //

// 7-segment patterns (common cathode, bit order: DP G F E D C B A)
// TODO: fill in if needed


// ============================ //
//  Global State Variables      //
// ============================ //
// Remember: anything touched in an ISR and read elsewhere must be volatile.



// ============================ //
//  Helper Functions            //
// ============================ //



// ============================ //
//  Interrupt Service Routine   //
// ============================ //

__interrupt(high_priority) void HandleInterrupt() {
    // INT0 — PORTB0
    if (INTCONbits.INT0IF) {
        INTCONbits.INT0IF = 0;
        // TODO
    }

    // INT1 — PORTB1
    if (INTCON3bits.INT1IF) {
        INTCON3bits.INT1IF = 0;
        // TODO
    }

    // INT2 — PORTB2
    if (INTCON3bits.INT2IF) {
        INTCON3bits.INT2IF = 0;
        // TODO
    }

    // TIMER0 overflow
    if (INTCONbits.TMR0IF) {
        INTCONbits.TMR0IF = 0;
        // TODO: reload TMR0H / TMR0L, do periodic work
    }

    // TIMER1 overflow (if used)
    // if (PIR1bits.TMR1IF) {
    //     PIR1bits.TMR1IF = 0;
    //     // TODO
    // }
}


// ============================ //
//  Initialization              //
// ============================ //

void init_ports(void) {
    // TODO: set TRIS for inputs/outputs
    // TODO: clear LATs
}

void init_interrupts(void) {
    // TODO: configure INT0/INT1/INT2 edge (INTCON2)
    // TODO: clear flags, enable interrupts (INTCON, INTCON3)
    // TODO: configure TIMER0 (T0CON, TMR0H, TMR0L)
    // TODO: RCONbits.IPEN, GIE, PEIE
}


// ============================ //
//  Main                        //
// ============================ //

void main(void) {
    init_ports();

    // TODO: initial state setup

    __delay_ms(1000);

    init_interrupts();

    while (1) {
        // main loop
    }
}
