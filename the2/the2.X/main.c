// ============================ //
// Do not edit this part!!!!    //
// ============================ //
// 0x300001 - CONFIG1H
#pragma config OSC = HSPLL // Oscillator Selection bits (HS oscillator,
                           // PLL enabled (Clock Frequency = 4 x FOSC1))
#pragma config FCMEN = OFF // Fail-Safe Clock Monitor Enable bit
                           // (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF  // Internal/External Oscillator Switchover bit
                           // (Oscillator Switchover mode disabled)
// 0x300002 - CONFIG2L
#pragma config PWRT = OFF  // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF // Brown-out Reset Enable bits (Brown-out
                           // Reset disabled in hardware and software)
// 0x300003 - CONFIG1H
#pragma config WDT =                                                           \
    OFF // Watchdog Timer Enable bit
        // (WDT disabled (control is placed on the SWDTEN bit))
// 0x300004 - CONFIG3L
// 0x300005 - CONFIG3H
#pragma config LPT1OSC = OFF // Low-Power Timer1 Oscillator Enable bit
                             // (Timer1 configured for higher power operation)
#pragma config MCLRE = ON    // MCLR Pin Enable bit (MCLR pin enabled;
                             // RE3 input pin disabled)
// 0x300006 - CONFIG4L
#pragma config LVP = OFF   // Single-Supply ICSP Enable bit (Single-Supply
                           // ICSP disabled)
#pragma config XINST = OFF // Extended Instruction Set Enable bit
                           // (Instruction set extension and Indexed
                           // Addressing mode disabled (Legacy mode))

#pragma config DEBUG = OFF // Disable In-Circuit Debugger

#define KHZ 1000
#define MHZ (KHZ * KHZ)
#define _XTAL_FREQ 40 * MHZ
#include <xc.h>

#define SEG_A LATJbits.LATJ0
#define SEG_B LATJbits.LATJ1
#define SEG_C LATJbits.LATJ2
#define SEG_D LATJbits.LATJ3
#define SEG_E LATJbits.LATJ4
#define SEG_F LATJbits.LATJ5
#define SEG_G LATJbits.LATJ6
#define SEG_DP LATJbits.LATJ7

#define DIG_D0 LATHbits.LATH3
#define DIG_D1 LATHbits.LATH2
#define DIG_D2 LATHbits.LATH1
#define DIG_D3 LATHbits.LATH0

// ============================ //
//             End              //
// ============================ //

// ============================ //
//          HEADERS             //
// ============================ //

#define ROWS 4
#define COLS 8

// ============================ //
//           NOTES              //
// ============================ //
/*
 *      Kodu okudugum kadariyla eger blinking'den dolayi aktif parcanin isigi
 * sonmusse ve o parca hareket ettirilirse yeniden yaniyor gibime geldi, ama
 * runlayamadigimdan emin olamadim.
 * 
 * 
 */

// ============================ //
//        DEFINITIONS           //
// ============================ //

enum PieceTypes { DOT = 0, SQUARE = 1, L = 2 };

enum DIGITS { D0 = 0, D1 = 1, D2 = 2, D3 = 3 };

typedef unsigned char byte;

typedef struct {
    byte x;
    byte y;
} Point;

// ============================ //
//          GLOBALS             //
// ============================ //

void reset(){
    asm("reset");
}

byte activePieceGrid[4];
byte submittedGrid[4];
volatile byte submit = 0;
volatile byte portBPins = 0;
byte prevG = 0;
volatile byte rotationFlag = 0;
byte rotationIndex = 0;
byte currentPiece = 0; // 0 for dot, 1 for square, 2 for L
const byte displayValues[] = {0, 1, 5, 8, 9, 13, 16, 17, 21, 24, 25, 29, 32};
byte displayIterator = 0;
Point topLeft = {.x = 0, .y = 0};
// ============================ //
//          FUNCTIONS           //
// ============================ //

void setXthBit(byte *row, byte X) { *row |= (1 << X); }
byte getXthBit(byte row, byte X) { return (row >> X) & 1; }
void clearXthBit(byte *row, byte X) { *row &= ~(1 << X); }

void incrementCurrentPiece() { currentPiece = (currentPiece + 1) % 3; }

void printGrid() {
    LATC = activePieceGrid[0] | submittedGrid[0];
    LATD = activePieceGrid[1] | submittedGrid[1];
    LATE = activePieceGrid[2] | submittedGrid[2];
    LATF = activePieceGrid[3] | submittedGrid[3];
}

void spawnShape(byte shape) {
    topLeft.x = 0;
    topLeft.y = 0;
    rotationIndex = 0;
    activePieceGrid[0] = 0;
    activePieceGrid[1] = 0;
    activePieceGrid[2] = 0;
    activePieceGrid[3] = 0;
    if (shape == 0) {
        // Dot
        setXthBit(&activePieceGrid[0], 0); // Assuming position (0,1) for Dot
    } else if (shape == 1) {
        // Square shape
        setXthBit(&activePieceGrid[0], 0);
        setXthBit(&activePieceGrid[0], 1);
        setXthBit(&activePieceGrid[1], 0);
        setXthBit(&activePieceGrid[1], 1);
    } else if (shape == 2) {
        // L shape
        // Vertical part
        setXthBit(&activePieceGrid[0], 0);
        setXthBit(&activePieceGrid[1], 0);
        // Horizontal part
        setXthBit(&activePieceGrid[1], 1);
    }
    printGrid();
}
byte can_submit() {
    for (int i = 0; i < 4; i++) {
        if (submittedGrid[i] & activePieceGrid[i]) {
            return 0;
        }
    }
    return 1;
}

void initializeVariables() {
    LATA = 0x00;
    LATB = 0x00;
    LATC = 0x00;
    LATD = 0x00;
    LATE = 0x00;
    LATF = 0x00;
    LATG = 0x00;
    LATH = 0x00;
    LATJ = 0x00;
    portBPins = PORTB;
    for (int i = 0; i < 4; i++) {
        activePieceGrid[i] = 0;
        submittedGrid[i] = 0;
    }
}
// set up interrupts
// rb should fire high prio,
// timer0 should fire low prio
void initializeInterrupts() {
    // Disable all interrupts during configuration
    INTCONbits.GIE = 0;

    // Priority levels on interrupts enabled
    RCONbits.IPEN = 1;

    // Reset interrupt flags
    INTCONbits.TMR0IF = 0; // Clear Timer0 interrupt flag
    INTCONbits.RBIF = 0;   // Clear PORTB change interrupt flag

    // Configure Timer0 interrupt
    INTCONbits.TMR0IE = 1;  // Enable Timer0 interrupt
    INTCON2bits.TMR0IP = 0; // Set Timer0 interrupt to low priority

    // Configure PORTB change interrupt
    INTCONbits.RBIE = 1;  // Enable RB port change interrupt
    INTCON2bits.RBIP = 1; // Set RB port change interrupt to high priority

    INTCONbits.PEIE = 1; // Enable peripheral interrupts
    // Re-enable global interrupts
    INTCONbits.GIE = 1;
}

// initialize and start timers
void initializeTimers() {
    T0CON = 0b00000101;   // 16-bit mode, pre-scaler 1:64
    TMR0H = 0x65;         // High byte of 26000
    TMR0L = 0x90;         // Low byte of 26000
    T0CONbits.TMR0ON = 1; // Start Timer0
}

void initializePorts() {
    TRISA = 0x00; // Debug output from PORTA
    TRISB = 0b11001111;
    TRISC = 0x00; // Set PORTC as output
    TRISD = 0x00; // Set PORTD as output
    TRISE = 0x00; // Set PORTE as output
    TRISF = 0x00; // Set PORTF as output
    TRISJ = 0x00; // Configure all port J pins as outputs
    TRISH = 0x00; // Configure all port H pins as outputs
    TRISG = 0b00011101;
}

byte digitPatterns[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66,
                          0x6D, 0x7D, 0x07, 0x7F, 0x6F};

void init() {
    initializePorts();
    initializeVariables();
    initializeInterrupts();
    initializeTimers();
    spawnShape(currentPiece);
    printGrid();
}
void displayDigit(byte num, byte digitIndex) {
    LATJ = 0;                   // CLear latJ
    if (digitIndex == 0) {  // birler
        LATH = 0b00001000; // Activate D0 (Units)
        __delay_ms(1);              // Change this to adjust brightness
    } else if (digitIndex == 1) {   // onlar
        LATH = 0b00000100; // Activate D1 (Tens)
        __delay_ms(1);              // Change this to adjust brightness
    }else if (digitIndex == 2) {   // yuzler
        LATH = 0b00000010; // Activate D2 
        __delay_ms(1);              // Change this to adjust brightness
    }else if (digitIndex == 3) {   // binler
        LATH = 0b00000001; // Activate D3 
    __delay_ms(1);              // Change this to adjust brightness
        }
    LATJ = num;
    __delay_ms(10);              // Change this to adjust brightness
    LATH = 0;
}
// ============================ //
//   INTERRUPT SERVICE ROUTINE  //
// ============================ //

__interrupt(high_priority) void HandleHighInterrupt() {
    if (INTCONbits.RBIF) {
        __delay_ms(50);      // manual schmitt trigger
        byte portBRead = PORTB;
        byte changedPins = (portBPins ^ portBRead) & portBRead;
        portBPins = portBRead;
        if (changedPins & 0b01000000) {
            // RB6: submit button
            submit = 1;
        } else if (changedPins & 0b10000000) {
            // RB7: rotate button
            rotationFlag = 1;
        }
        INTCONbits.RBIF = 0;
    }
}
volatile byte move_down = 0;
volatile byte counter = 0;
byte displayNumber = 0;
volatile byte blink = 0;
void __interrupt(low_priority) HandleLowInterrupt(void) {

    if (INTCONbits.TMR0IF) {
        TMR0H = 0x67;
        TMR0L = 0x75;
        INTCONbits.TMR0IF = 0; // Clear Timer0 interrupt flag

        if (counter == 8) {
            move_down = 1;
        } else {
            counter++;
            blink = 1;
        }
    }
}
// ============================ //
//            MAIN              //
// ============================ //

void moveActivePieceDown() {
    for (int i = 0; i < 4; i++) {
        if (getXthBit(activePieceGrid[i], 7)) {
            return;
        }
    }
    for (int i = 0; i < 4; i++) {
        activePieceGrid[i] = activePieceGrid[i] << 1;
    }
    topLeft.y++;
    printGrid();
}

void moveActivePieceRight() {
    if (activePieceGrid[3] & 0b11111111) {
        return;
    }
    for (int i = 3; i > 0; i--) {
        activePieceGrid[i] = activePieceGrid[i - 1];
    }
    activePieceGrid[0] = 0;
    topLeft.x++;
    printGrid();
}

void moveActivePieceLeft() {
    if (activePieceGrid[0] & 0b11111111) {
        return;
    }
    for (int i = 1; i < 4; i++) {
        activePieceGrid[i - 1] = activePieceGrid[i];
    }
    activePieceGrid[3] = 0;
    topLeft.x--;
    printGrid();
}

void moveActivePieceUp() {
    for (int i = 0; i < 4; i++) {
        if (getXthBit(activePieceGrid[i], 0)) {
            return;
        }
    }
    for (int i = 0; i < 4; i++) {
        activePieceGrid[i] = activePieceGrid[i] >> 1;
    }
    topLeft.y--;
    printGrid();
}

void refresh7SegmentDisplay(){
    displayDigit(0x3F, D3);                                    // binler
    displayDigit(0x3F, D2);                                    
    displayDigit(digitPatterns[displayValues[displayIterator] / 10], D1);                                    
    displayDigit(digitPatterns[displayValues[displayIterator] % 10], D0);                                    
}

void update7SegmentDisplay(){
    ++displayIterator;
    if((displayIterator) == 12){
        reset();
    }
}

int main(void) {
    init();
    __delay_ms(1000);
    while (1) {
        if (blink) {
            LATC ^= activePieceGrid[0];
            LATD ^= activePieceGrid[1];
            LATE ^= activePieceGrid[2];
            LATF ^= activePieceGrid[3];
            blink = 0;
        }
        if (move_down) {
            // TIMER INTERRUPT OCCURED
            __delay_ms(2);
            moveActivePieceDown();
            counter = 0;
            move_down = 0;
        }
        if (submit) {
            __delay_ms(2);
            if (can_submit()) {
                for (int i = 0; i < 4; i++) {
                    submittedGrid[i] |= activePieceGrid[i];
                }
                incrementCurrentPiece();
                spawnShape(currentPiece);
                update7SegmentDisplay();
                refresh7SegmentDisplay();
            }
            submit = 0;
        }
        byte currentG = PORTG;
        byte movements = (prevG ^ currentG) & currentG;
        prevG = currentG;
        if (movements & 0b00000001) {
            // RG0
            moveActivePieceRight();
        }
        if (movements & 0b00000100) {
            // RG2
            moveActivePieceUp();
        }
        if (movements & 0b00001000) {
            // RG3
            moveActivePieceDown();
        }
        if (movements & 0b00010000) {
            // RG4
            moveActivePieceLeft();
        }
        if(rotationFlag){
            rotationFlag = 0;
            
            if(currentPiece == 2){
                ++rotationIndex;
                rotationIndex = (rotationIndex)%4;
                if(rotationIndex == 0){
                    clearXthBit(&activePieceGrid[topLeft.x], topLeft.y+1);
                    setXthBit(&activePieceGrid[topLeft.x+1], topLeft.y+1);
                }
                else if(rotationIndex == 1){
                    clearXthBit(&activePieceGrid[topLeft.x], topLeft.y);
                    setXthBit(&activePieceGrid[topLeft.x], topLeft.y+1);
                }
                else if(rotationIndex == 2){
                    clearXthBit(&activePieceGrid[topLeft.x+1], topLeft.y);
                    setXthBit(&activePieceGrid[topLeft.x], topLeft.y);
                }
                else if(rotationIndex == 3){
                    clearXthBit(&activePieceGrid[topLeft.x+1], topLeft.y+1);
                    setXthBit(&activePieceGrid[topLeft.x+1], topLeft.y);
                }
                
                
                
                printGrid();
            }
        }
        refresh7SegmentDisplay();       // periodic refresh
    }

    return 1;
}