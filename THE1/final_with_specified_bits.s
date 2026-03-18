;Name_Sname: Deniz Karakoyun
;StudentID: 2580678
PROCESSOR 18F8722

#include <xc.inc>

; CONFIGURATION (DO NOT EDIT)
; CONFIG1H
CONFIG OSC = HSPLL      ; Oscillator Selection bits (HS oscillator, PLL enabled (Clock Frequency = 4 x FOSC1))
CONFIG FCMEN = OFF      ; Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
CONFIG IESO = OFF       ; Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)
; CONFIG2L
CONFIG PWRT = OFF       ; Power-up Timer Enable bit (PWRT disabled)
CONFIG BOREN = OFF      ; Brown-out Reset Enable bits (Brown-out Reset disabled in hardware and software)
; CONFIG2H
CONFIG WDT = OFF        ; Watchdog Timer Enable bit (WDT disabled (control is placed on the SWDTEN bit))
; CONFIG3H
CONFIG LPT1OSC = OFF    ; Low-Power Timer1 Oscillator Enable bit (Timer1 configured for higher power operation)
CONFIG MCLRE = ON       ; MCLR Pin Enable bit (MCLR pin enabled; RE3 input pin disabled)
; CONFIG4L
CONFIG LVP = OFF        ; Single-Supply ICSP Enable bit (Single-Supply ICSP disabled)
CONFIG XINST = OFF      ; Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))
CONFIG DEBUG = OFF      ; Disable In-Circuit Debugger


    
    
; GLOBAL is needed for variables to be seen by other files 
GLOBAL var1
GLOBAL var2
GLOBAL var3
GLOBAL prev_button_status
GLOBAL prog_dir_flags
GLOBAL progress_bar_bit_index
GLOBAL current_button_status
GLOBAL direction_of_counter
GLOBAL out_C
GLOBAL main_counter_high
GLOBAL main_counter_low   

; Define space for the variables in RAM
PSECT udata_acs
var1:
    DS 1 ; Allocate 1 byte for var1
var2:
    DS 1 
var3:
    DS 1
prev_button_status:
    DS 1
prog_dir_flags:
    DS 1
progress_bar_bit_index:
    DS 1   
current_button_status:
    DS 1
direction_of_counter:
    DS 1
main_counter_high:
    DS 1 
main_counter_low:
    DS 1 
out_C: 
    DS 1

; Do NOT remove or modify the resetVec section below.
PSECT resetVec,class=CODE,reloc=2
resetVec:
    goto       main

PSECT CODE
 
 
 
 
; TODO: CHANGE EVERY INSTRUCTION STRUCTURE AND ADD d, a BITS TO AVOID ANY ERROR 
 
main:
    
    ; CONFIGURATION
    
    ; Setting PORTB, PORTC and PORTD for output.
    clrf TRISB, A	; PORTB shall display the progress bar pattern. 
    clrf TRISC, A	; PORTC shall display the binary counter value
    clrf TRISD, A	; RD0 shall be a blinking LED

    ; Setting PORTE for input
    setf TRISE, A	; RE0: Pause/resume the progress bar (but not the binary counter or the blinking LED).
			; RE1: Change the counting direction of the binary counter
    
    
    
    
    ; INITIALIZATION
    
    ; Clearing all the variables and ports before writing on them
    clrf var1, A			    ; var1 = 0		
    clrf var2, A			    ; var2 = 0
    clrf var3, A			    ; var3 = 0
    clrf prev_button_status, A		    ; prev_button_status = 0
    setf prog_dir_flags, A		    ; prog_dir_flags = 1 initially running without any input
    clrf progress_bar_bit_index, A	    ; progress_bar_bit_index = 0
    clrf current_button_status, A	    ; current_button_status = 0
    clrf direction_of_counter, A	    ; direction_of_counter = 0	    0 means forward, 1 means backward 
    clrf main_counter_high, A		    ; main_counter_high = 0
    clrf main_counter_low, A		    ; main_counter_low = 0
    clrf out_C, A			    ; out_C = 0
    
    
    clrf LATB, A
    clrf LATC, A
    clrf LATD, A	
    
    
    ; Calling the light up function to turn all lights on
    ; there is no such thing as "local scope". Every single variable you define in your Static RAM is completely global
    
    call lights_on	; turns every light on
    
    
    call busy_wait	; call busy_wait to wait for 1000 -/+ 50 ms 
    
    call lights_off	; turn all the lights off 
    
    
    
    
    ; MAIN_LOOP FOR ROUND-ROBIN METHOD
    main_loop:
	
	; STATE: BLINKING LED (RD0)
	
	counter_check:
	    ; check if counter reached to 256
	    movlw 1 
	    xorwf  main_counter_high, W, A	
	    bnz wait_in_main			; if it is not 256 then wait a bit more

	    ; check if low counter part is 244. never reached here if high part isnt 256
	    movlw 244 
	    xorwf  main_counter_low, W, A	
	    bnz wait_in_main			; if it is not 244 then wait a bit more
	    
	    ; reset the counter if it reached 500
	    movlw 0
	    movwf main_counter_high, A
	    movwf main_counter_low, A
	    ; control the RD0 if it is 0 it means after we hit 500 we need to go set_RD0 so skip clear_RD0 otherwise the opposite 
	    btfsc LATD, 0, A
	    bra clear_RD0
	

	    
	set_RD0:
	    ; turn on the RD0 
	    bsf LATD, 0, A

	    ; The progress bar shall advance one step each time RD0 changes state (every 500 ±50 ms)
	    ; PROGRESS BAR
	    call progress_bar
	    
	    ; count
	    call count
	    ; after we set RD0 we need to wait another 500ms so goto wait part
	    bra wait_in_main

	clear_RD0: 
	    ; turn RD0 back off
	    bcf LATD, 0, A

	    ; The progress bar shall advance one step each time RD0 changes state (every 500 ±50 ms)
	    ; PROGRESS BAR 
	    call progress_bar
	    
	    ; count  
	    call count
	
	
	
	wait_in_main:
	    ; wait for another 1007 (1.007 ms) until it reaches 500ms 
	    
	    call light_busy_wait		; call busy_wait to wait for (1.007 ms)
	    call check_E_button_release		; check if E button released and click(1 ? 0) happened in E
	    incf main_counter_low, F, A		; main_counter_low++
	    btfsc STATUS, 0, A			; check the carry bit if it is 0 skip incrementing the high part 
	    incf main_counter_high, F, A	; if main_counter_low carry outs we increment the main_counter_high 
	    ; goto ma?n_loop
	    goto main_loop
    
    
    
    

;;;;;;; WAIT FUNCTIONS 	  
	    
light_busy_wait:
    ; to wait 1ms 
    movlw 250               
    movwf var1, A           
    
    ; for i = 250; i > 0; i--
    light_loop:
        nop                 
        decfsz var1, F, A   
        bra light_loop      
        
    return               

  
    
busy_wait:
    
    ; for 1000ms it runs for   1000471 (1.000471 s)


    set_1000_ms:
	movlw 252

    execute_delay:
	movwf var1, A          
	movlw 0
	movwf var2, A
	movwf var3, A

    ; it goes for i = 252; i < 257; i++ .      for 500ms
    outer_loop:
	
	middle_loop:

	    ;  it goes for i = 0; i < 257; i++ 
	    innermost_loop1:
		incf var3, F, A
		bnc innermost_loop1
	    
	    movlw 4	; unnecessary lines to prlong the time 
	    movlw 4	; unnecessary lines to prlong the time
	    movlw 188
	    movwf var3, A
	    ;  it goes for i = 188; i < 257; i++ .      for 500ms
	    innermost_loop2:
		incf var3, F, A
		bnc innermost_loop2

	    incf var2, F, A 
	    bnc middle_loop  

	incf var1, F, A
	bnc outer_loop	
    return
    
    
  
    
;;;;;;; LIGHT FUNCTIONS 
    
lights_on:
    
    ; turn PORTB on
    setf LATB, A 
    ; turn PORTC on
    setf LATC, A 
    ; turn PORTD on
    setf LATD, A
    
    ; Equivalent: use either PORT or LAT register to write to output ports
    ; setf PORTE
    ; setf LATE
    
    return
    
    
    
lights_off:
    ; turn PORTB off
    clrf LATB, A  
    ; turn PORTC off
    clrf LATC, A 
    ; turn PORTD off
    clrf LATD, A
    
    return

    
    
;;;; BUTTON RELEASE CHECK 
 
check_E_button_release:
    ; when button release happens take toggle the prog_dir_flags
    movff PORTE, current_button_status	    ; get the input to current_button_status
    
    comf current_button_status, W, A	    ; take the complement of current_button_status = !current_button_status
    andwf prev_button_status, F, A	    ; do and operation prev_button_status & !current_button_status 
    
    btfsc prev_button_status, 0, A	    ; if prev_button_status[0] = 0 dont toggle the resume/pause flag
    btg prog_dir_flags, 0, A		    ; toggle the prog_dir_flags[0] bit
    
    btfsc prev_button_status, 1, A	    ; if prev_button_status[1] = 0 dont toggle the direction flag
    btg prog_dir_flags, 1, A		    ; toggle the prog_dir_flags[1] direction bit
    
    movff current_button_status, prev_button_status ; update the prev = current
    
    return
    

; STATE: PROGRESS BAR (PORTB)
progress_bar:
    
    ; check progress_bar_bit_index >= 8
    ; TODO: actually there is no need for to check negativity since we always check for progress_bar_bit_index == 0
    ; but in case of something we can do that 
    
    
    ; check if user released the RE0 button 
    btfss prog_dir_flags, 0, A	    ; check if user wants to resume = 1 or pause = 0 if 1 dont return otherwise rreturn immediatlry 
    return 
    
    ; 11111111 ? 00000000 conversion needs checking for bit_index == 8
    checking:
	movlw 8
	subwf progress_bar_bit_index, W, A ; dest as wreg
	bz reset_portb
    
    ; if progress_bar_bit_index < 8
    ; shift the bits to left and set the carry 1 
    bsf STATUS, 0, A			; set the CARRY status 1 
    rlcf LATB, F, A			; shift the bits and add 1 from right 
    
    incf progress_bar_bit_index, F, A	; progress_bar_bit_index++
    
    return
    
    ;else:
    reset_portb:
	; clear PORTB
	clrf LATB, A
	clrf progress_bar_bit_index, A	; progress_bar_bit_index = 0 reset it 
	return
   
    
; STATE: BINARY COUNTER (PORTC)    
count:
    ; check the direction if prog_dir_flags[1] is 1 it means forward otherwise backward
    movff LATC, out_C		    ; out_C = LATC
    btfss prog_dir_flags, 1, A	    ; skip next line if it is 1 meaning forward
    bra backward
    
    
    forward:
	; increment the PORTC(0-3) up to max 15
	incf out_C, F, A
	bra out 
    backward:
	; decrement the PORTC(0-3) down to min 0
	decf out_C, F, A
    
    out:
	movlw 0x0f		    ; move 00001111 to wreg 
	andwf out_C, F, A	    ; and it with the current output so clear out the first 4 bit
	movff out_C, LATC	    ; give it back to output LATCH
	return
    
end resetVec