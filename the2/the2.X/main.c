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

#define KHZ 1000
#define MHZ (KHZ * KHZ)
#define _XTAL_FREQ 40 * MHZ

// ============================ //
//             End              //
// ============================ //

#include <xc.h>


// ============================ //
//          HEADERS             //
// ============================ //

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

enum PieceTypes{
    DOT = 0,
    SQUARE = 1,
    L = 2
};

// What I thought while creating this piece is that 
// boundary checking with 4 coordinates would be very easy
// although it might store unnecessary variables (such as
// in the case of piece 1) we can just do the rotation and 
// shift operation by swapping variables very easily.
struct current_piece{
    unsigned char left_coord;
    unsigned char right_coord;
    unsigned char top_coord;
    unsigned char bottom_coord;
    enum PieceTypes current_piece_type;
    unsigned char rotation_index;   // 1 means initial condition (empty dot on bottom left) and rotates counterclockwise with each increment
};

// ============================ //
//          GLOBALS             //
// ============================ //

struct current_piece cp;
unsigned char grid[8][4];       // TODO: enable CCI and make this one's type bit
unsigned char displayValues[4]; // left to right display values
unsigned char rotationFlag;     // TODO: enable CCI and make this one's type bit

// ============================ //
//          FUNCTIONS           //
// ============================ //

// initialize any and all global variables
// do this last
void initializeVariables(){
    
}

// set up interrupts
// rb should fire high prio,
// timer0 should fire low prio
void initializeInterrupts(){
    RCON = 0b10000000;  // enable priority interrupts
    INTCON = 0b11101000; // enable low, high prio, timer0 and rbChange
    INTCON2 = 0b10000001; // tmor0 low interrupt, rb high interrupt, 
    // INTCON3 is for external interrupts (INT0 etc)
    // Don't think we will be using PIE and PIR registers
}

// initialize and start timers
void initializeTimers(){
    
}

void initializePorts(){
    
}

void createNewPiece(){
    static unsigned char pieceTypeCounter = 0;  // TODO: see if you can transform this into an enum
    cp.left_coord = 1;
    cp.top_coord = 1;
    cp.rotation_index = 1;
    cp.current_piece_type = pieceTypeCounter;
    switch(pieceTypeCounter++){
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
    if(pieceTypeCounter > 3)
        pieceTypeCounter = 1;
    //TODO: reset the timer, and flicking variable
}

void init(){
    __delay_ms(1000);   // wait for one second
    initializeVariables();
    initializeInterrupts();
    initializeTimers();
    initializePorts();
    
    createNewPiece();
}



void movePieceDown(){
    if(cp.bottom_coord < 8){
        cp.top_coord++;
        cp.bottom_coord++;
    }
}

void movePieceUp(){
    if(cp.top_coord > 1){
        cp.top_coord--;
        cp.bottom_coord--;
    }
}

void movePieceLeft(){
    if(cp.left_coord > 1){
        cp.left_coord--;
        cp.right_coord--;
    }
}

void movePieceRight(){
    if(cp.right_coord < 4){
        cp.left_coord++;
        cp.right_coord++;
    }
}

void rotatePiece(){
    cp.rotation_index++;
    if(cp.rotation_index > 4)
        cp.rotation_index = 1;
}

void writeTo7SegmentDisplay(const unsigned char val){
    static const unsigned char sevenSegmentDisplayValues[] = {
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
    displayValues[2] = sevenSegmentDisplayValues[val/10];
    displayValues[3] = sevenSegmentDisplayValues[val%10];
}

// 7-sd display values are predetermined
// only iterate through the values
void update7SegmentDisplay(){
    static unsigned char display_iterator = 0;
    static const unsigned char display_values[] = {1, 5, 8, 9, 13, 16, 17, 21, 24, 25, 29, 32};
    writeTo7SegmentDisplay(display_values[display_iterator++]);
}

// successful submission
void submitPiece(){
    grid[cp.bottom_coord][cp.left_coord] = 1;
    grid[cp.top_coord][cp.left_coord] = 1;
    grid[cp.top_coord][cp.right_coord] = 1;
    grid[cp.bottom_coord][cp.right_coord] = 1;
    if(cp.current_piece_type == 2){
        switch(cp.rotation_index){
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
void trySubmitPiece(){
    unsigned char available_pixels = 4;   
    if(grid[cp.bottom_coord][cp.left_coord] == 0)
        available_pixels--;
    if(grid[cp.top_coord][cp.left_coord] == 0)
        available_pixels--;
    if(grid[cp.bottom_coord][cp.right_coord] == 0)
        available_pixels--;
    if(grid[cp.top_coord][cp.right_coord] == 0)
        available_pixels--;
    if(cp.current_piece_type == L){
        switch(cp.rotation_index){
            case 1:
                if(grid[cp.bottom_coord][cp.left_coord] == 1)
                    available_pixels++;
                break;
            case 2:
                if(grid[cp.top_coord][cp.left_coord] == 1)
                    available_pixels++;
                break;
            case 3:
                if(grid[cp.top_coord][cp.right_coord] == 1)
                    available_pixels++;
                break;
            case 4:
                if(grid[cp.bottom_coord][cp.right_coord] == 1)
                    available_pixels++;
                break;
        }
    }
    if(available_pixels == 4)
        submitPiece();
}

// This will probably create a problem.
// Last year's homework on 7-sd
void lightUpSevenSegmentDisplay(){
    1;
}

// ============================ //
//   INTERRUPT SERVICE ROUTINE  //
// ============================ //

// Implement ONLY rb interrupts as high
// because a high interrupt may intercept a low interrupt routine
// we should still register if user presses RB when the timer0 interrupt is going on
__interrupt(high_priority)
void HandleHighInterrupt()
{
    // High ISR
}
// Implement ONLY timer0 interrupts as low
// because a high interrupt may intercept a low interrupt routine
// we should still register if user presses RB when the timer0 interrupt is going on
__interrupt(low_priority)
void HandleLowInterrupt(){
    // Low ISR
}
// ============================ //
//            MAIN              //
// ============================ //
int main(void)
{
    // Pseudo:
    // Poll PORTA for movement
    // Handle the movement
    // Check if there is a rotation waiting
    // Handle the rotation
    lightUpSevenSegmentDisplay();
    
    return 1;
}