#include <xc.h>
#include <stdint.h>

#include "adc.h"
#include "communicate.h"
#include "pragmas.h"

#define _XTAL_FREQ 40000000UL

#define STATE_WAITING 0
#define STATE_ACTIVE  1
#define STATE_END     2

// NEW: sentinel meaning "no ACK pending this slot"
#define ACK_NONE 0xFFu

const uint8_t segment_map[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x00};

// NEW: start blank so WAITING shows nothing on the display
volatile uint8_t display_buffer[4] = {10, 10, 10, 10};
volatile uint8_t display_page = 0;

volatile uint8_t requested_limit = 0;
volatile uint8_t port_mask = 0;
volatile uint8_t system_state = STATE_WAITING;

// NEW: one-slot pending in-run command, applied at the next 100 ms tick
static volatile uint8_t pending_valid = 0;
static volatile cmd     pending_cmd;

// NEW: one-slot pending ACK drained at the next transmit slot
static volatile uint8_t pending_ack = ACK_NONE;

// NEW: ISR -> main-loop signal that a 100 ms tick has fired
static volatile uint8_t tick_flag = 0;

// NEW: ISR -> tick signal for an accepted RB6 release edge
static volatile uint8_t button_release_flag = 0;


void cabinet_tick_init(void)
{
    /*
    Fosc = 40 MHz -> Fcy = Fosc/4 = 10 MHz
    Tcy = 1 / 10 MHz = 0.1 us per tick
    Target time = 100 ms = 100,000 us
    Total ticks required = 100,000 us / 0.1 us = 1,000,000 ticks
    Using 1:16 Prescaler:
    1,000,000 / 16 = 62,500 ticks per timer overflow.
    Preload value = 65536 - 62,500 = 3036
    3036 in Hex = 0x0BDC
    TMR0H = 0x0B
    TMR0L = 0xDC
     */

    T0CON = 0x03;
    TMR0H = 0x0B;
    TMR0L = 0xDC;
    INTCONbits.TMR0IF = 0;
    INTCONbits.TMR0IE = 1;
    T0CONbits.TMR0ON = 1;
}

void rb6_ioc_init(void)
{
    TRISBbits.TRISB6 = 1 ;
    uint8_t dummy = PORTB ;
    INTCONbits.RBIF = 0 ;
    INTCONbits.RBIE = 1 ;
}

void display_init(void)
{
    TRISJ &= 0x80;
    LATJ  &= 0x80;

    TRISH &= 0xF0;
    LATH  &= 0xF0;

    /*  Timer1 Multiplexing for 5 ms)
     * Fcy = 10 MHz -> Tcy = 0.1 us
     * 5 ms = 50,000 ticks. 65536 - 50000 = 15536 (0x3CB0)
     */
    TMR1H = 0x3C;
    TMR1L = 0xB0;
    T1CON = 0x80;
    PIR1bits.TMR1IF = 0;
    PIE1bits.TMR1IE = 1;
    T1CONbits.TMR1ON = 1;
}

// NEW: helper so WAITING/END can write four blanks in one call
static void blank_display(void)
{
    display_buffer[0] = 10;
    display_buffer[1] = 10;
    display_buffer[2] = 10;
    display_buffer[3] = 10;
}

// NEW: replaces update_display_buffer with corrected Page 1 digit order (tens at pos 3, ones at pos 2)
static void refresh_display(uint16_t adc_val, uint8_t ee, uint8_t mask)
{
    if (display_page == 0) {
        display_buffer[3] = (adc_val / 1000) % 10;
        display_buffer[2] = (adc_val / 100)  % 10;
        display_buffer[1] = (adc_val / 10)   % 10;
        display_buffer[0] =  adc_val         % 10;
    } else {
        display_buffer[3] = (ee / 10) % 10;
        display_buffer[2] =  ee       % 10;
        display_buffer[1] = 10;
        display_buffer[0] = mask & 0x07;
    }
}


void __interrupt() isr(void)
{
    // NEW: communicate.c owns the RX/TX rings; one call replaces the old in-place EUSART byte handling
    comm_isr_handle();

    if (PIE1bits.ADIE && PIR1bits.ADIF) {
        adc_isr_handle();
    }

    if (INTCONbits.RBIE && INTCONbits.RBIF) {
        uint8_t value = PORTB;
        if ((value & (1 << 6)) && system_state == STATE_ACTIVE) {
            // NEW: defer page toggle to the tick handler instead of doing it in ISR
            button_release_flag = 1;
        }
        INTCONbits.RBIF = 0;
    }

    if (INTCONbits.TMR0IE && INTCONbits.TMR0IF) {
        TMR0H = 0x0B;
        TMR0L = 0xDC;
        INTCONbits.TMR0IF = 0;
        // NEW: heavy tick work moved to the main loop; ISR only raises the flag
        tick_flag = 1;
    }

    if (PIE1bits.TMR1IE && PIR1bits.TMR1IF) {
        TMR1H = 0x3C;
        TMR1L = 0xB0;
        PIR1bits.TMR1IF = 0;

        static uint8_t digit_index = 0;

        LATH &= 0xF0;
        LATJ = segment_map[display_buffer[digit_index]];
        LATH |= (1 << digit_index);

        digit_index++;
        if (digit_index > 3) digit_index = 0;
    }
}

// NEW: applies the held in-run command at the tick boundary with idempotency checks
static void apply_pending_command(void)
{
    if (!pending_valid) return;
    pending_valid = 0;

    switch (pending_cmd.cmd_type) {
        case CMD_CON: {
            uint8_t bit = 1u << pending_cmd.payload;
            if (!(port_mask & bit)) {
                port_mask |= bit;
                pending_ack = ACK_CON;
            }
            break;
        }
        case CMD_DIS: {
            uint8_t bit = 1u << pending_cmd.payload;
            if (port_mask & bit) {
                port_mask &= (uint8_t)~bit;
                pending_ack = ACK_DIS;
            }
            break;
        }
        case CMD_LIM:
            if (requested_limit != pending_cmd.payload) {
                requested_limit = pending_cmd.payload;
                pending_ack = ACK_LIM;
            }
            break;
        default:
            break;
    }
}

// NEW: forces the first tick after $GO# to be exactly 100 ms later
static void restart_tick(void)
{
    T0CONbits.TMR0ON  = 0;
    TMR0H = 0x0B;
    TMR0L = 0xDC;
    INTCONbits.TMR0IF = 0;
    tick_flag = 0;
    T0CONbits.TMR0ON  = 1;
}


void main(void)
{
    adc_init();
    // NEW: bring EUSART up via communicate.c (replaces the old eusart_init stub)
    comm_init();
    cabinet_tick_init();
    rb6_ioc_init();
    display_init();

    INTCONbits.PEIE = 1;
    INTCONbits.GIE  = 1;

    // NEW: derives the 500 ms ADC cadence from the 100 ms tick counter
    uint8_t adc_div = 0;

    for (;;) {
        // NEW: drain parsed frames and dispatch lifecycle vs. in-run commands
        cmd c;
        while (comm_get_command(&c)) {
            if (c.cmd_type == CMD_GO) {
                if (system_state == STATE_WAITING) {
                    system_state    = STATE_ACTIVE;
                    port_mask       = 0;
                    requested_limit = 0;
                    display_page    = 0;
                    pending_valid   = 0;
                    pending_ack     = ACK_GO;
                    adc_div         = 0;
                    adc_start_conversion();
                    restart_tick();
                }
            }
            else if (c.cmd_type == CMD_END) {
                if (system_state == STATE_ACTIVE) {
                    system_state  = STATE_END;
                    pending_valid = 0;
                    pending_ack   = ACK_NONE;
                    ADCON0bits.GO = 0;
                    PIE1bits.ADIE = 0;
                    blank_display();
                }
            }
            else {
                if (system_state == STATE_ACTIVE) {
                    pending_cmd   = c;
                    pending_valid = 1;
                }
            }
        }

        // NEW: per-tick work in Algorithm 1 order
        if (tick_flag) {
            tick_flag = 0;

            if (system_state != STATE_ACTIVE) {
                continue;
            }

            apply_pending_command();

            adc_div++;
            if (adc_div >= 5) {
                adc_div = 0;
                adc_start_conversion();
            }

            if (button_release_flag) {
                button_release_flag = 0;
                display_page ^= 1u;
            }

            uint8_t cap = adc_get_thermal_cap();
            uint8_t ee  = (requested_limit < cap) ? requested_limit : cap;

            uint16_t adc_val = adc_get_raw_value();
            refresh_display(adc_val, ee, port_mask);

            if (pending_ack != ACK_NONE) {
                comm_send_ack(pending_ack);
                pending_ack = ACK_NONE;
            } else {
                comm_send_sts(adc_get_mode_char(), adc_val, port_mask, ee);
            }
        }
    }
}
