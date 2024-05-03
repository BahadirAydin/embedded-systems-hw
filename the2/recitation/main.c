// ============================ //
// Do not edit this part!!!!    //
// ============================ //
// 0x300001 - CONFIG1H
#pragma config OSC = HSPLL      // Oscillator Selection bits (HS oscillator,
                                // PLL enabled (Clock Frequency = 4 x FOSC1))
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit
                                // (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal/External Oscillator Switchover bit
                                // (Oscillator Switchover mode disabled)
// 0x300002 - CONFIG2L
#pragma config PWRT = OFF       // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bits (Brown-out
                                // Reset disabled in hardware and software)
// 0x300003 - CONFIG1H
#pragma config WDT = OFF        // Watchdog Timer Enable bit
                                // (WDT disabled (control is placed on the SWDTEN bit))
// 0x300004 - CONFIG3L
// 0x300005 - CONFIG3H
#pragma config LPT1OSC = OFF    // Low-Power Timer1 Oscillator Enable bit
                                // (Timer1 configured for higher power operation)
#pragma config MCLRE = ON       // MCLR Pin Enable bit (MCLR pin enabled;
                                // RE3 input pin disabled)
// 0x300006 - CONFIG4L
#pragma config LVP = OFF        // Single-Supply ICSP Enable bit (Single-Supply
                                // ICSP disabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit
                                // (Instruction set extension and Indexed
                                // Addressing mode disabled (Legacy mode))

#pragma config DEBUG = OFF      // Disable In-Circuit Debugger

#define KHZ 1000UL
#define MHZ (KHZ * KHZ)
#define _XTAL_FREQ (40UL * MHZ)

// ============================ //
//             End              //
// ============================ //

#include <xc.h>
#include <stdint.h>

#define T_PRELOAD_LOW 0xC1
#define T_PRELOAD_HIGH 0x34

uint8_t prevB;

void Init()
{
    // B
    LATB = 0x00; PORTB = 0x00; TRISB = 0x10;
    // C
    LATC = 0x00; PORTC = 0x00; TRISC = 0x00;
    // D
    LATD = 0x00; PORTD = 0x00; TRISD = 0x00;
}

void InitializeTimerAndInterrupts()
{
    // Enable pre-scalar
    // Full pre-scale
    // we also need to do in-code scaling
    T0CON = 0x00;
    T0CONbits.TMR0ON = 1;
    T0CONbits.T0PS2 = 1;
    T0CONbits.T0PS1 = 0;
    T0CONbits.T0PS0 = 1;
    // Pre-load the value
    TMR0H = T_PRELOAD_HIGH;
    TMR0L = T_PRELOAD_LOW;

    RCONbits.IPEN = 0;
    INTCON = 0x00;
    INTCONbits.TMR0IE = 1;
    INTCONbits.RBIE = 1;
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
}

// ============================ //
//   INTERRUPT SERVICE ROUTINE  //
// ============================ //
__interrupt(high_priority)
void HandleInterrupt()
{
    // Timer overflowed (333 ms)
    if(INTCONbits.TMR0IF)
    {
        INTCONbits.TMR0IF = 0;
        // Pre-load the value
        TMR0H = T_PRELOAD_HIGH;
        TMR0L = T_PRELOAD_LOW;

        // Fully lit LATC
        LATD = ~PORTD;
    }
    if(INTCONbits.RBIF)
    {
        // Read the value to satisfy the interrupt
        prevB = PORTB;

        LATC = ~PORTC;

        // Then clear the bit
        INTCONbits.RBIF = 0;
    }
}

__interrupt(low_priority)
void HandleInterrupt2()
{

}
// ============================ //
//            MAIN              //
// ============================ //
void main()
{
    Init();
    InitializeTimerAndInterrupts();

    prevB = PORTB;
    __delay_ms(100);

    // Main Loop
    while(1)
    {
        //...
    }
}