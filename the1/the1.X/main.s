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

GLOBAL counter
GLOBAL counter_2
GLOBAL counter_3
; for button press detection
GLOBAL prev_state_r0
GLOBAL prev_state_r1
GLOBAL pbar_c_on
GLOBAL pbar_b_on


; Define space for the variables in RAM
PSECT udata_acs
counter:
    DS 1
counter_2:
    DS 1
counter_3:
    DS 1
prev_state_r0:
    DS 1
prev_state_r1:
    DS 1
pbar_c_on:
    DS 1
pbar_b_on:
    DS 1


PSECT resetVec,class=CODE,reloc=2
resetVec:
    goto       main

PSECT CODE

main:
    call init
    call main_loop
init:
    clrf pbar_c_on ; disabled by default
    clrf pbar_b_on ; disabled by default
    movlw 255
    movwf counter
    ; PORTX consists of 8 LEDs (RX0-RX7)
    ; LATX determines whether the pins are high or low
    ; TRISX determines whether the pins are inputs or outputs
    ; Set PORT(B,C,D) as output
    movlw 75
    movwf counter_2

    clrf TRISB
    clrf TRISC
    clrf TRISD
    
    setf TRISE ; set PORTE as input

    
    ; light up all LEDs
    setf PORTB
    setf LATB 
    setf PORTC
    setf LATC 
    setf PORTD
    setf LATD
    ; wait for 1 second
    call busy_wait
    clrf PORTB
    clrf LATB
    clrf PORTC
    clrf LATC
    clrf PORTD
    clrf LATD
    return
main_loop:
    call update
    goto main_loop
    return

; 1000 +- 50 ms wait
busy_wait:
    movlw 6
    movwf counter
    loop1:
        movlw 217
        movwf counter_2 ; var2 = 217
        loop2:
            movlw 255
            movwf counter_3 ; var1 = 255
            loop3:
                decfsz counter_3,1
                goto loop3
            decfsz counter_2,1
            goto loop2
        decfsz counter,1
        goto loop1
    movlw 80
    movwf counter_2 ; I set counter_2 to 80 to make it correct for first loop of update.
    return

update:
    btfsc prev_state_r0,0 ; NOT skipped if button is pressed
    call r0_released
    btfsc prev_state_r1,0
    call r1_released
    register_re0:
    btfsc prev_state_r0,0
    goto register_re1
    movf PORTE,0 ; WREG = PORTE
    andlw 00000001B   ; WREG = WREG AND b'00000001
    movwf prev_state_r0 ; prev_state_r0 is either 0 or 1
    register_re1:
    btfsc prev_state_r1,0
    goto incr
    movf PORTE,0
    andlw 00000010B
    movwf prev_state_r1
    incr:
    incfsz counter,1
    return
    counter_1_overflow:
        ; decrement counter_2 and skip if zero
        decfsz counter_2,1
        return
        toggle:
        btg LATD,0 ; toggle RD0
	movlw 80
	movwf counter_2
	pbar_c:
	movf PORTC,0
	andwf pbar_c_on
	movwf LATC
	pbar_d:
	movf PORTB,0
	andwf pbar_b_on
	movwf LATB
	return
x
r0_released:
    btfsc PORTE,0 ; if button is released toggle PORTC 
    return
    btg pbar_c_on,0
    clrf prev_state_r0
    return
r1_released:
    btfss PORTE,1
    return
    clrf prev_state_r1
    btg pbar_b_on,0
    return
    

; Example case:
; RE0 is 0.
; I press RE0.
; RE0 is 1. I need to detect this change and set prev_state_r0_r0 to 1.
; I release RE0.
; RE0 is 0. I need to detect this change and toggle the progress bar on PORTC and set prev_state_r0_r0 to 0.
    
end resetVec


