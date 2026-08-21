#ifndef COMM_H
#define COMM_H

#include <stdint.h>
#include <stdio.h>

typedef enum {
    CMD_NONE = 0,
    CMD_GO,
    CMD_END,
    CMD_CON,   
    CMD_DIS,    
    CMD_LIM    
} cmd_kind_t;



typedef struct {
    cmd_kind_t cmd_type;
    uint8_t payload;
} cmd;


// $ACK00# GO
// $ACK01# $CONp#
// $ACK02# $DISp#
// $ACK03# $LIMxx#

#define ACK_GO 0u 
#define ACK_CON 1u
#define ACK_DIS 2u  
#define ACK_LIM 3u   
#define MODE_NORMAL    'N'
#define MODE_DERATED   'D'
#define MODE_OVERHEAT  'H'



/* Initialize EUSART1 (115200 8N1) and arm RX interrupts.
 * Caller must enable GIE/PEIE in main() after all module inits. */
void comm_init();

/* Pull the next parsed command from the RX path.
 * Returns 1 and fills *out if a complete, well-formed frame was parsed.
 * Returns 0 if no command is available. Non-blocking.
 * Note: lifecycle/idempotency checks are the caller's responsibility. */
uint8_t comm_get_command(cmd *out);

/* Build and enqueue $STSmxxxxcee# (13 bytes).
 *   mode_char: 'N' / 'D' / 'H'
 *   adc_value: 0..1023 (formatted as 4-digit decimal)
 *   port_mask: 0..7   (bit i set => port i connected)
 *   ee:        0/8/16/24 (formatted as 2-digit decimal) */
void comm_send_sts(char mode_char, uint16_t adc_value,
                   uint8_t port_mask, uint8_t ee);

/* Build and enqueue $ACKCC# (7 bytes). code in {0,1,2,3}.
 * Use ACK_GO / ACK_CON / ACK_DIS / ACK_LIM macros. */
void comm_send_ack(uint8_t code);

/* Hook called from the global ISR in main.c.
 * Handles RX byte capture, TX byte transmission, and EUSART error recovery. */
void comm_isr_handle();

#endif /* COMM_H */