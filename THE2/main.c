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

#include "map.h"

// ============================ //
//        DEFINITIONS           //
// ============================ //
// You can write your definitions here...

// If a variable is going to be modified inside your Interrupt Service Routine (ISR)
// and read or modified inside your main() loop, it must be declared with the volatile keyword.

// ============================ //
//          GLOBALS             //
// ============================ //
// You can write globals definitions here...

unsigned next_scene = 0; 
volatile uint8_t is_bird_visible = 1; // 0 is off 1 is on, again it needs to be volatile i think 
volatile unsigned bird_row = 2; 
// volatile unsigned pos_y = 0; // i think y will never change we will always be at 0th column but again
volatile unsigned scroll_count = 0;
volatile unsigned scroll_count_needed = 20;  
volatile unsigned gravity_count = 0;
volatile unsigned alive_count = 0;
volatile uint8_t pipe_F0 = 0;
volatile uint8_t pipe_E0 = 0;
volatile uint8_t pipe_D0 = 0;
volatile uint8_t pipe_C0 = 0;

volatile uint8_t flag_blink = 1;
volatile uint8_t flag_gravity = 0;
volatile uint8_t flag_scroll = 0;
volatile uint16_t score = 0;

volatile uint8_t flag_flap = 0;
volatile uint16_t scroll_step_time = 1000;
volatile uint8_t is_game_over = 0;
volatile uint8_t game_over_counter = 0;

const unsigned char seg_codes[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
// ============================ //
//          FUNCTIONS           //
// ============================ //
// You can write function definitions here...

void wait_1sec(){
    for(uint8_t i = 0; i < 10; i++){
        __delay_ms(100);
    }
    return;
}



// since we always need to turn the current bit off
    // directly turn the bit where the bird is off
void delete_bird(){
    
    switch(bird_row){
        case(0):
            // we are at PORTF 
            LATFbits.LATF0 = pipe_F0;
            break;
        case(1):
            // we are at PORTE 
            LATEbits.LATE0 = pipe_E0;
            break;   
        case(2):
            // we are at PORTD
            LATDbits.LATD0 = pipe_D0;
            break;
        case(3):
            // we are at PORTC 
            LATCbits.LATC0 = pipe_C0;
    }
    return;
}


void draw_bird(){
    
    
    switch(bird_row){
        case(0):
            // we are at PORTF 
            LATFbits.LATF0 = 1;
            break;
        case(1):
            // we are at PORTE 
            LATEbits.LATE0 = 1;
            break;   
        case(2):
            // we are at PORTD
            LATDbits.LATD0 = 1;
            break;
        case(3):
            // we are at PORTC 
            LATCbits.LATC0 = 1;
    }
    return;
}


void scroll(){
    if(next_scene == MAP_SIZE){
        // no more scene to load, basa sar 
        next_scene = 0;
    }
    if ((pipe_F0 && pipe_E0 && !pipe_D0 && !pipe_C0) || (!pipe_F0 && !pipe_E0 && pipe_D0 && pipe_C0)) {   // there is a pipe and bird did not collide with it
        score += 2;
    }
    
    
    
    int bitF, bitE, bitD, bitC;
    bitF = SCENE_ARRAY[0][next_scene];
    bitE = SCENE_ARRAY[1][next_scene];
    bitD = SCENE_ARRAY[2][next_scene];
    bitC = SCENE_ARRAY[3][next_scene];
    
    // shift the port values by using the next scene to come to the 7th bit place
    // For now ? will update from top to bottom
    LATF = (LATF >> 1) | (bitF << 7);
    LATE = (LATE >> 1) | (bitE << 7);
    LATD = (LATD >> 1) | (bitD << 7);
    LATC = (LATC >> 1) | (bitC << 7);
    
    // update the 0th coulmn pipes so that when deleting the bird dont delet the pipe
    pipe_F0 = LATFbits.LATF0;
    pipe_E0 = LATEbits.LATE0;
    pipe_D0 = LATDbits.LATD0;
    pipe_C0 = LATCbits.LATC0;
    
    if(is_bird_visible){
        draw_bird();
    }
    // eets say interrupt happened here if there is something depending 
    // on the next_scene number it will be problematic 
    next_scene++;
    return;
}




void bird_blink(){
    
    is_bird_visible = !is_bird_visible;
    switch(is_bird_visible){
        case(0):
            // now bird needs to be invisible
            delete_bird();
            break;
            
        case(1):
            // now bird needs to be visible
            draw_bird();
    }
    
    return;
}



void gravity(){
    // puls the bird one row down, if it is at portc no effect
    
    if(bird_row == 3) return;
    
    delete_bird();
    bird_row++;
    if (is_bird_visible) {
    draw_bird();  
    }
    
    return;
}

void flap() {
    // if we are at the top PORTF, dont move 
    if (bird_row == 0) return;
    
    delete_bird();
    bird_row--; 
    if(is_bird_visible){
        draw_bird(); 
    }
    
    return;
}

void Game_Over(){
    is_game_over = 1;
    // only timer0 interrupt is open
    INTCONbits.INT0IE = 0;
    INTCON3bits.INT1IE = 0;  
    INTCON3bits.INT3IE = 0;
            
    
    
}


void check_collision() {
    if (bird_row == 0 && pipe_F0 == 1) { Game_Over(); }
    if (bird_row == 1 && pipe_E0 == 1) { Game_Over(); }
    if (bird_row == 2 && pipe_D0 == 1) { Game_Over(); }
    if (bird_row == 3 && pipe_C0 == 1) { Game_Over(); }
}

void update_display() {
    uint8_t birler = score % 10;
    uint8_t onlar = (score / 10) % 10;
    uint8_t yuzler = (score / 100) % 10;

    //PORTH1
    LATH = 0x02; 
    LATJ = seg_codes[yuzler];
    __delay_ms(2);

    // PORTH2
    LATH = 0x04; 
    LATJ = seg_codes[onlar];
    __delay_ms(2);

    // PORTH3
    LATH = 0x08; 
    LATJ = seg_codes[birler];
    __delay_ms(2);

    LATH = 0x00;
}






// ============================ //
//   INTERRUPT SERVICE ROUTINE  //
// ============================ //
__interrupt(high_priority) void HandleInterrupt() {
    
    
    // reset the timer and load the preloader again
    if(INTCONbits.TMR0IF == 1){
        // 50ms is up toggle the visiblty of the bird 
        
        // reload the initial timer value 
        TMR0H = 0x0B;
        TMR0L = 0xDC;
        
        if (is_game_over == 0) {
        flag_blink = 1;
        
        if(++gravity_count == 15){
            flag_gravity = 1;
            gravity_count = 0;
        }
        
        
        if(++scroll_count == scroll_count_needed){  //////////// not hard-coded 20, changes based on button presses
            flag_scroll = 1;
            scroll_count = 0;
        }
        if (++alive_count == 40   ) {  // survived 2 second 
            score += 3;
            alive_count = 0 ;
        }
        }
        else {   // game over
            game_over_counter++;
            if (game_over_counter >= 80) { 
            INTCONbits.TMR0IF = 0;
    
            asm("RESET"); 
            }
        }
        
        INTCONbits.TMR0IF = 0;
    }
    
    if (INTCON3bits.INT1IF == 1) {  // RB1 button press slow downn 
        // UPDATED STARTS
       
        if (scroll_count_needed < 30 ){
            scroll_count_needed += 5;
        } 
        
        INTCON3bits.INT1IF = 0;
    }
    
    if (INTCON3bits.INT3IF == 1) {   // rb3 pressed speedup
        
        if (scroll_count_needed > 5 ){
            scroll_count_needed -= 5;
        } 
        // UPDATED ENDS
        
        if (scroll_count >= scroll_count_needed) {  
            scroll_count = scroll_count- scroll_count_needed;
            flag_scroll =1 ;   
        } 
         
        INTCON3bits.INT3IF = 0 ;

    }
    if (INTCONbits.INT0IF == 1) {  // RB0 pressed flapp 
        flag_flap = 1; 
        INTCONbits.INT0IF = 0;
    }
    
}

// ============================ //
//            MAIN              //
// ============================ //
void main() {
    
    // 1. INITIALIZATION STARTS
    
        
        // clear all the LEDs
    TRISC = 0;
    TRISD = 0;
    TRISE = 0;
    TRISF = 0;
   
    LATC = 0;
    LATD = 0;
    LATE = 0;
    LATF = 0;
   
    
    TRISH = 0; 
    TRISJ = 0; //for 7 segment
    LATH = 0; 
    LATJ = 0;
    
    
        // set the interrupts
            // int 0,1,2 in portb are external interrupts
    TRISB = 0b00001111;
    INTCONbits.INT0IF  = 0;
    INTCONbits.INT0IE  = 1;
    INTCON3bits.INT1IF = 0;
    INTCON3bits.INT3IF = 0;
    INTCON3bits.INT1IE = 1;
    INTCON3bits.INT3IE = 1;
    
    
            // timer0 set
            // timer 0 is for 50ms timing
    T0CONbits.TMR0ON = 0;   
    T0CONbits.T08BIT = 0;   
    T0CONbits.T0CS = 0;     
    T0CONbits.PSA = 0;    
    
    T0CONbits.T0PS2 = 0;
    T0CONbits.T0PS1 = 1;
    T0CONbits.T0PS0 = 0;

    TMR0H = 0x0B;  
    TMR0L = 0xDC;

    INTCONbits.TMR0IE = 1;  
    INTCONbits.TMR0IF = 0;  

        // wait 1 second 
    wait_1sec();
    
    

    
    // MISSED S-25
    //UPDATED STARTS
    LATFbits.LATF7 = SCENE_ARRAY[0][0];
    LATEbits.LATE7 = SCENE_ARRAY[1][0];
    LATDbits.LATD7 = SCENE_ARRAY[2][0];
    LATCbits.LATC7 = SCENE_ARRAY[3][0];
    next_scene = 1;
    draw_bird(); 
    // UPDATED ENDS
    
    T0CONbits.TMR0ON = 1;
    INTCONbits.GIE = 1;
    // 1. INITIALIZATION ENDS
    
    
    // 2. SCENE ARRAY STARTS
        // game map definition
    
    
     
    
    // no need for if(step == 0)) check in the main loop it starts from portd assign to that 
    
    // game loop
    while(1){
        // I think each iteration needs to represent one column movement 
        if (is_game_over == 0){ // if game is over just display score
            

            if(flag_blink == 1){
                bird_blink();
                flag_blink = 0;
            }
            if(flag_gravity == 1){
                gravity();
                flag_gravity = 0;
                check_collision();

            }
            if(flag_scroll == 1){
                scroll();
                flag_scroll = 0;
                check_collision();

            }
            if (flag_flap == 1 ){
                flap();
                flag_flap = 0;
                check_collision();

            }
        
        }
        update_display(); 
    }
        
    
    
    
    
}


