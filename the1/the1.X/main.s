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

GLOBAL var1
GLOBAL var2
GLOBAL var3
GLOBAL counter
GLOBAL counter_2
GLOBAL result

; Define space for the variables in RAM
PSECT udata_acs
var1:
    DS 1 ; Allocate 1 byte for var1
var2:
    DS 1 
var3:
    DS 1
result: 
    DS 1
counter:
    DS 1
counter_2:
    DS 1


PSECT resetVec,class=CODE,reloc=2
resetVec:
    goto       main

PSECT CODE
main:
    movlw 255
    movwf counter
    ; PORTX consists of 8 LEDs (RX0-RX7)
    ; LATX determines whether the pins are high or low
    ; TRISX determines whether the pins are inputs or outputs
    ; Set PORT(B,C,D) as output
    movlw 255
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
    call main_loop
main_loop:
    call blinking
    goto main_loop
    return

; 1000 +- 50 ms wait
busy_wait:
    movlw 6
    movwf var3
    loop1:
        movlw 217
        movwf var2 ; var2 = 217
        loop2:
            movlw 255
            movwf var1 ; var1 = 255
            loop3:
                decfsz var1,1
                goto loop3
            decfsz var2,1
            goto loop2
        decfsz var3,1
        goto loop1
    return

blinking:
    incfsz counter,1
    return
    counter_1_overflow:
        ; decrement counter_2 and skip if zero
        decfsz counter_2,1
        return
        toggle:
        ; toggle RD0
        btg LATD,0
        return

end resetVec
