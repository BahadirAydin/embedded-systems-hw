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

#define SEG_A   LATJbits.LATJ0
#define SEG_B   LATJbits.LATJ1
#define SEG_C   LATJbits.LATJ2
#define SEG_D   LATJbits.LATJ3
#define SEG_E   LATJbits.LATJ4
#define SEG_F   LATJbits.LATJ5
#define SEG_G   LATJbits.LATJ6
#define SEG_DP  LATJbits.LATJ7

#define DIG_D0  LATHbits.LATH3
#define DIG_D1  LATHbits.LATH2
#define DIG_D2  LATHbits.LATH1
#define DIG_D3  LATHbits.LATH0

// ============================ //
//             End              //
// ============================ //


// ============================ //
//          HEADERS             //
// ============================ //

#define ROWS 4
#define COLS 8

void createNewPiece();

// ============================ //
//           NOTES              //
// ============================ //
/*
 *  -The coordinate system is as follows:
 *      Top-left is 1,1
 *      Bottommost row is 8
 *      Rightmost column is 4
 *  -Rotation index of the L piece specifies its empty dot. 1 is bottom left,
 *  2 is top left, 3 is top right, 4 is bottom left
 */

// ============================ //
//        DEFINITIONS           //
// ============================ //

enum PieceTypes {
    DOT = 0, SQUARE = 1, L = 2
};

enum DIGITS {
    D0 = 0, D1 = 1, D2 = 2, D3 = 3
};

// What I thought while creating this piece is that
// boundary checking with 4 coordinates would be very easy
// although it might store unnecessary variables (such as
// in the case of piece 1) we can just do the rotation and
// shift operation by swapping variables very easily.
struct current_piece {
    unsigned char left_coord;
    unsigned char right_coord;
    unsigned char top_coord;
    unsigned char bottom_coord;
    enum PieceTypes current_piece_type;
    unsigned char
    rotation_index; // 1 means initial condition (empty dot on bottom left)
    // and rotates counterclockwise with each increment
};

// ============================ //
//          GLOBALS             //
// ============================ //

struct current_piece cp;
byte grid[4]; // TODO: enable CCI and make this one's type bit

unsigned char displayValues[4]; // left to right display values
unsigned char rotationFlag; // TODO: enable CCI and make this one's type bit
typedef unsigned char byte;

// ============================ //
//          FUNCTIONS           //
// ============================ //

// initialize any and all global variables
// do this last
void spawnShape(byte shape) {
    if (shape == DOT) {
        cp.bottom_coord = 0;
        cp.left_coord = 0;
        cp.right_coord = 0;
        cp.top_coord = 0;
        cp.current_piece_type = DOT;
    } else if (shape == L) {
        cp.left_coord = 0;
        cp.top_coord = 0;
        cp.bottom_coord = 1;
        cp.right_coord = 1;
        cp.current_piece_type = L;
        cp.rotation_index = 1;
    } else {
        cp.left_coord = 0;
        cp.top_coord = 0;
        cp.bottom_coord = 1;
        cp.right_coord = 1;
        cp.current_piece_type = SQUARE;
    }
}




void initializeVariables() {
    PORTA = 0x00;
    PORTB = 0x00;
    PORTC = 0x00;
    PORTD = 0x00;
    PORTE = 0x00;
    PORTF = 0x00;

    // Loop through all grid cells and set them to zero
    for (byte i = 0; i < 4; i++) {
        for (byte j = 0; j < 8; j++)
            grid[i][j] = 0;
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
    INTCONbits.RBIF = 0; // Clear PORTB change interrupt flag

    // Configure Timer0 interrupt
    INTCONbits.TMR0IE = 1; // Enable Timer0 interrupt
    INTCON2bits.TMR0IP = 0; // Set Timer0 interrupt to low priority

    // Configure PORTB change interrupt
    INTCONbits.RBIE = 1; // Enable RB port change interrupt
    INTCON2bits.RBIP = 1; // Set RB port change interrupt to high priority

    // Enable low priority interrupt
    INTCONbits.PEIE = 1;

    // Re-enable global interrupts
    INTCONbits.GIE = 1;
}

// initialize and start timers
void initializeTimers() {
    T0CON = 0b00000101; // 16-bit mode, pre-scaler 1:64
    TMR0H = 0x65; // High byte of 26000
    TMR0L = 0x90; // Low byte of 26000
    T0CONbits.TMR0ON = 1; // Start Timer0
}

void initializePorts() {
    TRISB = 0b01101111;
    TRISC = 0x00; // Set PORTC as output
    TRISD = 0x00; // Set PORTD as output
    TRISE = 0x00; // Set PORTE as output
    TRISF = 0x00; // Set PORTF as output
    TRISJ = 0x00; // Configure all port J pins as outputs
    TRISH = 0x00; // Configure all port H pins as outputs
}

void createNewPiece() {
    static unsigned char pieceTypeCounter =
        0; // TODO: see if you can transform this into an enum
    cp.left_coord = 1;
    cp.top_coord = 1;
    cp.rotation_index = 1;
    cp.current_piece_type = pieceTypeCounter;
    switch (pieceTypeCounter++) {
    case 0: // dot
        cp.bottom_coord = 1;
        cp.left_coord = 1;
        break;
    case 1: // square
        // fall down
    case 2: // L
        cp.bottom_coord = 2;
        cp.right_coord = 2;
        break;
    }
    if (pieceTypeCounter > 3) {
        pieceTypeCounter = 1;
    }
    // TODO: reset the timer, and flicking variable
}
byte digitPatterns[10] = {
    0x3F,
    0x06,
    0x5B,
    0x4F,
    0x66,
    0x6D,
    0x7D,
    0x07,
    0x7F,
    0x6F
};

void init() {
    __delay_ms(1000); // wait for one second
    initializeVariables();
    initializeInterrupts();
    initializeTimers();
    initializePorts();
    createNewPiece();
}

void movePieceDown() {
    if (cp.bottom_coord < 8) {
        cp.top_coord++;
        cp.bottom_coord++;
    }
}

void movePieceUp() {
    if (cp.top_coord > 1) {
        cp.top_coord--;
        cp.bottom_coord--;
    }
}

void movePieceLeft() {
    if (cp.left_coord > 1) {
        cp.left_coord--;
        cp.right_coord--;
    }
}

void movePieceRight() {
    if (cp.right_coord < 4) {
        cp.left_coord++;
        cp.right_coord++;
    }
}

void rotatePiece() {
    cp.rotation_index++;
    if (cp.rotation_index > 4)
        cp.rotation_index = 1;
}

void writeTo7SegmentDisplay(const unsigned char val) {
    static
    const unsigned char sevenSegmentDisplayValues[] = {
        0x3f,
        0x06,
        0x5b,
        0x4f,
        0x66,
        0x6D,
        0x7D,
        0x07,
        0x7F,
        0x67
    };
    displayValues[2] = sevenSegmentDisplayValues[val / 10];
    displayValues[3] = sevenSegmentDisplayValues[val % 10];
}

// 7-sd display values are predetermined
// only iterate through the values
void update7SegmentDisplay() {
    static unsigned char display_iterator = 0;
    static
    const unsigned char display_values[] = {
        1,
        5,
        8,
        9,
        13,
        16,
        17,
        21,
        24,
        25,
        29,
        32
    };
    writeTo7SegmentDisplay(display_values[display_iterator++]);
}

// successful submission
void submitPiece() {
    grid[cp.bottom_coord][cp.left_coord] = 1;
    grid[cp.top_coord][cp.left_coord] = 1;
    grid[cp.top_coord][cp.right_coord] = 1;
    grid[cp.bottom_coord][cp.right_coord] = 1;
    if (cp.current_piece_type == 2) {
        switch (cp.rotation_index) {
        case 1:
            grid[cp.bottom_coord][cp.left_coord] = 0;
            break;
        case 2:
            grid[cp.top_coord][cp.left_coord] = 0;
            break;
        case 3:
            grid[cp.top_coord][cp.right_coord] = 0;
            break;
        case 4:
            grid[cp.bottom_coord][cp.right_coord] = 0;
            break;
        }
    }

    update7SegmentDisplay();

    createNewPiece();
}

// check 4 boundaries of each piece
// if any is occupied, decrement available pixels by one
// if it is an L piece, check its empty space and increment
// available pixels by one (meaning it is available)
// if available pixels is still 4 at the end, it is submittable.
// since this is called in a high priority interrupt, it is certain
// that it will not be interrupted.
void trySubmitPiece() {
    unsigned char available_pixels = 4;
    if (grid[cp.bottom_coord][cp.left_coord] == 0)
        available_pixels--;
    if (grid[cp.top_coord][cp.left_coord] == 0)
        available_pixels--;
    if (grid[cp.bottom_coord][cp.right_coord] == 0)
        available_pixels--;
    if (grid[cp.top_coord][cp.right_coord] == 0)
        available_pixels--;
    if (cp.current_piece_type == L) {
        switch (cp.rotation_index) {
        case 1:
            if (grid[cp.bottom_coord][cp.left_coord] == 1)
                available_pixels++;
            break;
        case 2:
            if (grid[cp.top_coord][cp.left_coord] == 1)
                available_pixels++;
            break;
        case 3:
            if (grid[cp.top_coord][cp.right_coord] == 1)
                available_pixels++;
            break;
        case 4:
            if (grid[cp.bottom_coord][cp.right_coord] == 1)
                available_pixels++;
            break;
        }
    }
    if (available_pixels == 4)
        submitPiece();
}

void displayDigit(byte num, byte digitIndex) {
    PORTJ = digitPatterns[num]; // Send segment pattern to PORTJ
    PORTH = 0xFF; // Turn off all digits (common anode: HIGH = OFF)
    if (digitIndex == 0) {
        PORTH &= ~0b00001000; // Activate D0 (Units)
    } else if (digitIndex == 1) {
        PORTH &= ~0b00000100; // Activate D1 (Tens)
    }
}

// ============================ //
//   INTERRUPT SERVICE ROUTINE  //
// ============================ //

// Implement ONLY rb interrupts as high
// because a high interrupt may intercept a low interrupt routine
// we should still register if user presses RB when the timer0 interrupt is
// going on
__interrupt(high_priority)
void HandleHighInterrupt() {
    if (INTCONbits.RBIF) {
        INTCONbits.RBIF = 0;
    }
}
// Implement ONLY timer0 interrupts as low
// because a high interrupt may intercept a low interrupt routine
// we should still register if user presses RB when the timer0 interrupt is
// going on
byte toggle = 0;
unsigned char counter = 0;
unsigned char displayNumber = 0;
void __interrupt(low_priority) HandleLowInterrupt(void) {
    if (INTCONbits.TMR0IF) {
        TMR0H = 0x67; // High byte of 26000
        TMR0L = 0x75; // Low byte of 26000
        INTCONbits.TMR0IF = 0; // Clear Timer0 interrupt flag

        if (counter == 4) {
            toggle = 1;
        } else {
            counter++;
        }
        // TODO: active piece blink
    }
}
// ============================ //
//            MAIN              //
// ============================ //

int main(void) {
    // Pseudo:
    // Poll PORTA for movement
    // Handle the movement
    // Check if there is a rotation waiting
    // Handle the rotation
    init();
    __delay_ms(1000);
    while (1) {
        if (toggle) {
            // TIMER INTERRUPT OCCURED
            byte units = displayNumber % 10; // Extract units digit
            byte tens = displayNumber / 10; // Extract tens digit
            // Display tens digit on D1
            for (byte i = 0; i < 4; i++) {
                displayDigit(displayNumber, i);
                __delay_ms(50); // Adjust delay as needed for visibility
            }
            displayNumber += 1;
            toggle = 0;
            counter = 0;
        }
    }

    return 1;
}