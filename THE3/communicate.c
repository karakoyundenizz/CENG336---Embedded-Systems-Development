
#include <xc.h>
#include <stdint.h>
#include "pragmas.h"
#include "communicate.h"



#define RB_SIZE 64u  

typedef struct {
    volatile uint8_t head;
    volatile uint8_t tail;
    volatile uint8_t data[RB_SIZE];
} ByteRing;

static volatile ByteRing rx_ring;
static volatile ByteRing tx_ring;

static uint8_t rb_next(uint8_t index)
{
    index++;
    if (index >= RB_SIZE) {
        index = 0;
    }
    return index;
}

static uint8_t rb_is_empty(volatile ByteRing *rb)
{
    return rb->head == rb->tail;
}

static uint8_t rb_is_full(volatile ByteRing *rb)
{
    return rb_next(rb->head) == rb->tail;
}

/* Single-producer / single-consumer push: producer-only writes head. */
static uint8_t rb_push(volatile ByteRing *rb, uint8_t value)
{
    uint8_t next = rb_next(rb->head);
    if (next == rb->tail) {
        return 0;   /* full ? drop the byte */
    }
    rb->data[rb->head] = value;   /* write data first */
    rb->head = next;              /* then advance head (publishes the byte) */
    return 1;
}

/* SPSC pop: consumer-only writes tail. */
static uint8_t rb_pop(volatile ByteRing *rb, uint8_t *value)
{
    if (rb_is_empty(rb)) {
        return 0;
    }
    *value = rb->data[rb->tail];
    rb->tail = rb_next(rb->tail);
    return 1;
}



static uint8_t uart_write_byte(uint8_t value)
{
    if (!rb_push(&tx_ring, value)) {
        return 0;   /* TX buffer full */
    }
    /* Wake the TX ISR; it will drain tx_ring and disable itself
     * when the buffer is empty again. */
    PIE1bits.TX1IE = 1;
    return 1;
}

static uint8_t free_space(volatile ByteRing *ring)
{
    uint8_t head = ring -> head;
    uint8_t tail = ring->tail;
    if (head >= tail)
        return RB_SIZE - 1 - (head - tail);
    else
        return tail - head - 1;
}
void comm_init()
{
   
    /*
     * TODO: configure EUSART1 for 115200 baud, 8 data bits, no parity,
     * and 1 stop bit on RC6/RC7. Enable receive interrupts. Leave TX1IE
     * disabled until uart_write_byte() has queued data.
     */
    
    // DENIZ 
    
    // clearing the flag and enabling the receive interrupt 
    
    
    // setting the EUSART1, receive and transmit control register settings 
    // 1. PIN setting for receive and transmit 
    TRISCbits.RC7 = 1;
    TRISCbits.RC6 = 0;
    
    // 2. EUSART1 8 bit Mode for both receiving and transmitting 
    RCSTA1bits.RX9 = 0;
    TXSTA1bits.TX9 = 0;
    TXSTA1bits.SYNC = 0;
    
    // 3. Baud rate setting we set 4 divider and 86 load
    TXSTA1bits.BRGH = 1;
    BAUDCON1bits.BRG16 = 1;
    SPBRG1 = 86;
    SPBRGH1 = 0;
    
    // 4. EUSART1 enable 
    TXSTA1bits.TXEN = 1;
    RCSTA1bits.CREN = 1;
    RCSTA1bits.SPEN = 1;
   
    
    // 5. Interrupt enable 
    PIE1bits.TX1IE = 0;
    PIE1bits.RC1IE = 1;
    
    // LEFT THE GIE/PEIE/IPEN ENABLE: TO main
         
    
}


void comm_isr_handle()
{
    // TODO: handle the isr for communication with pc
    
    uint8_t value;
    // since the flag is raised independent of the enable bit we need to check the enable bit also 
    if(PIE1bits.TX1IE == 1 && PIR1bits.TX1IF == 1){
        // we can transmit another byte
        if(rb_pop(&tx_ring, &value)){
            TXREG1 = value;
        }
        else{
            // no data to send so sleep until write triggers the enable 
            PIE1bits.TX1IE = 0;
        }
    }
    
    if(PIE1bits.RC1IE == 1 && PIR1bits.RC1IF == 1){
        
        uint8_t has_error = (RCSTA1bits.OERR || RCSTA1bits.FERR);
        if (RCSTA1bits.OERR) {
                RCSTA1bits.CREN = 0;
                RCSTA1bits.CREN = 1;
                // discard everything frame state will resync on the next $ 
            }
        // Framing error: just discard the byte read clears FERR)
        uint8_t value = RCREG1;  

        if (!has_error) {
            rb_push(&rx_ring, value);
        }
    
}
}

static uint8_t validate_cmd(const uint8_t *body, uint8_t len, cmd *out)
{
    // $GO# 
    if (len == 2 && body[0]=='G' && body[1]=='O') {
        out->cmd_type = CMD_GO;
        out->payload = 0;
        return 1;
    }
    
    // $END#
    if (len == 3 && body[0]=='E' && body[1]=='N' && body[2]=='D') {
        out->cmd_type = CMD_END;
        out->payload = 0;
        return 1;
    }
    
    // $CONp# / $DISp# 
    if (len == 4 && (body[3]=='0' || body[3]=='1' || body[3]=='2')) {
        if (body[0]=='C' && body[1]=='O' && body[2]=='N') {
            out->cmd_type = CMD_CON;
            out->payload = body[3] - '0';
            return 1;
        }
        if (body[0]=='D' && body[1]=='I' && body[2]=='S') {
            out->cmd_type = CMD_DIS;
            out->payload = body[3] - '0';
            return 1;
        }
    }
    
    // $LIMxx# 
    if (len == 5 && body[0]=='L' && body[1]=='I' && body[2]=='M') {
        // next two byte codes one of the  {00, 08, 16, 24} 
        if (body[3]=='0' && body[4]=='0'){
            out->cmd_type = CMD_LIM; 
            out->payload =  0; 
            return 1; 
        }
        if (body[3]=='0' && body[4]=='8'){
            out->cmd_type = CMD_LIM; 
            out->payload =  8; 
            return 1; 
        }
        if (body[3]=='1' && body[4]=='6'){
            out->cmd_type = CMD_LIM; 
            out->payload =  16; 
            return 1; 
        }
        if (body[3]=='2' && body[4]=='4'){
            out->cmd_type = CMD_LIM; 
            out->payload =  24; 
            return 1; 
        }
    }
    
    // if it reaches here it means it is malformed 
    return 0;   
}


uint8_t comm_get_command(cmd *out)
{
    
    static enum { wait_start, read_cmd } state = wait_start;
    static uint8_t body[8];
    static uint8_t body_len = 0;
    
    uint8_t byte;
    
    
    while (rb_pop(&rx_ring, &byte)) {
        
        if (state == wait_start) {
            
            if (byte == '$') {
                state = read_cmd;
                body_len = 0;
            }
        }
        else {  
            
            if (byte == '#') {
                if (validate_cmd(body, body_len, out)) {
                    state = wait_start;
                    return 1;   
                }
              
                state = wait_start;
            }
            else if (byte == '$') {
                body_len = 0;
            }
            else if (body_len < sizeof(body)) {
                body[body_len++] = byte;
            }
            else {
                state = wait_start;
            }
        }
    }
    
    return 0;  
}









void comm_send_sts(char mode_char, uint16_t adc_value,
                   uint8_t port_mask, uint8_t ee)
{
    /* TODO: build $STSmxxxxcee# (13 bytes) and enqueue via uart_write_byte. */
    // just empty work 
    if(free_space(&tx_ring) <13){
        return;
    }
    uart_write_byte('$');
    uart_write_byte('S');
    uart_write_byte('T');
    uart_write_byte('S');
    uart_write_byte(mode_char);
    
    // 4 digit adc with +a
    uart_write_byte('0' + (adc_value / 1000) % 10);
    uart_write_byte('0' + (adc_value / 100) % 10);
    uart_write_byte('0' + (adc_value / 10)% 10);
    uart_write_byte('0' + adc_value % 10);
    
    // since it will max 07 
    uart_write_byte('0' + (port_mask & 0x07));
    
    uart_write_byte('0' + (ee / 10) % 10);
    uart_write_byte('0' + ee % 10);
    
    uart_write_byte('#');
}

void comm_send_ack(uint8_t code)
{
    if(free_space(&tx_ring) < 7){
        return;
    }
    uart_write_byte('$');
    uart_write_byte('A');
    uart_write_byte('C');
    uart_write_byte('K');
    
    // 4 digit adc with +a
    uart_write_byte('0' + (code / 10) % 10);
    uart_write_byte('0' + code % 10);
    
  
    uart_write_byte('#');
}