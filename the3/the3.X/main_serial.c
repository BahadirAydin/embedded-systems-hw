/*
 * File:   main_serial.c
 * Author: e2380798
 *
 * Created on May 25, 2024, 5.17 PM
 */
#include "pragmas.h"
#include <string.h>
#include <xc.h>

#define BUFFER_SIZE 128
typedef unsigned char uint8_t;
// Commands
typedef enum {
    GOO,  // Start command, 4 bytes after
    END,  // End command, NO bytes after
    SPD,  // Speed, 4 bytes after
    ALTC, // Altitude control, 4 bytes after
    MAN,  // Manual control, 2 bytes after
    LED,  // LED control, 2 bytes after
} Command;
typedef enum {
    DST,  // Distance, 4 bytes after
    ALTR, // Altitude response, 4 bytes after
    PRS,  // Button press, 4 bytes after
} Response;

typedef enum {
    PACKET_WAIT,
    PACKET_GET,
    PACKET_ACK,
} State;

#define MAX_PACKET_SIZE 128
#define PACKET_HEADER '$'
#define PACKET_END '#'
uint8_t packet_data[MAX_PACKET_SIZE];
uint8_t packet_size = 0;
uint8_t packet_index = 0; // pkt_id in uluc hoca's code
uint8_t packet_valid = 0;
State packet_state = PACKET_WAIT;
#define BUFSIZE 128
typedef enum { INBUF,
               OUTBUF } buf_t;
volatile uint8_t inbuf[BUFSIZE];
volatile uint8_t outbuf[BUFSIZE];
volatile uint8_t head[2] = {0, 0};
volatile uint8_t tail[2] = {0, 0};

volatile int buffer_index = 0;
volatile int message_started = 0;

inline void disable_rxtx(void) {
    PIE1bits.RC1IE = 0;
    PIE1bits.TX1IE = 0;
}
inline void enable_rxtx(void) {
    PIE1bits.RC1IE = 1;
    PIE1bits.TX1IE = 1;
}

#pragma interrupt_level 2 // Prevents duplication of function
uint8_t buf_isempty(buf_t buf) { return (head[buf] == tail[buf]) ? 1 : 0; }

#pragma interrupt_level 2 // Prevents duplication of function
void buf_push(uint8_t v, buf_t buf) {
    if (buf == INBUF)
        inbuf[head[buf]] = v;
    else
        outbuf[head[buf]] = v;
    head[buf]++;
    if (head[buf] == BUFSIZE)
        head[buf] = 0;
    if (head[buf] == tail[buf]) {
        return;
    }
}
/* Retrieve data from buffer */
#pragma interrupt_level 2 // Prevents duplication of function
uint8_t buf_pop(buf_t buf) {
    uint8_t v;
    if (buf_isempty(buf)) {
        return 0;
    } else {
        if (buf == INBUF)
            v = inbuf[tail[buf]];
        else
            v = outbuf[tail[buf]];
        tail[buf]++;
        if (tail[buf] == BUFSIZE)
            tail[buf] = 0;
        return v;
    }
}

void receive_isr() {
    PIR1bits.RC1IF = 0;      // Acknowledge interrupt
    buf_push(RCREG1, INBUF); // Buffer incoming byte
}
void transmit_isr() {
    PIR1bits.TX1IF = 0; // Acknowledge interrupt
    // If all bytes are transmitted, turn off transmission
    if (buf_isempty(OUTBUF))
        TXSTA1bits.TXEN = 0;
    // Otherwise, send next byte
    else
        TXREG1 = buf_pop(OUTBUF);
}

void __interrupt(high_priority) highPriorityISR(void) {
    if (PIR1bits.RC1IF)
        receive_isr();
    if (PIR1bits.TX1IF)
        transmit_isr();
    if (INTCONbits.TMR0IF) {
        TMR0H = 0x65; // High byte of 26000
        TMR0L = 0x90; // Low byte of 26000
        INTCONbits.TMR0IF = 0;
    }
}
void __interrupt(low_priority) lowPriorityISR(void) {}

void init_usart() {
    unsigned int brg = 21; // (40000000 / (16 * 115200));
    SPBRG1 = brg;
}
#define SPBRG_VAL (21)
void init_ports() {
    TRISCbits.RC7 = 1;
    TRISCbits.RC6 = 0;
    TXSTA1bits.TX9 = 0;  // No 9th bit
    TXSTA1bits.TXEN = 0; // Transmission is disabled for the time being
    TXSTA1bits.SYNC = 0;
    TXSTA1bits.BRGH = 0;
    RCSTA1bits.SPEN = 1; // Enable serial port
    RCSTA1bits.RX9 = 0;  // No 9th bit
    RCSTA1bits.CREN = 1; // Continuous reception
    BAUDCON1bits.BRG16 = 1;
    SPBRGH1 = (SPBRG_VAL >> 8) & 0xff;
    SPBRG1 = SPBRG_VAL & 0xff;
}

void init_interrupt() {
    enable_rxtx();
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1; // globally enable interrupts
}
void init_timers() {
    INTCON2bits.TMR0IP = 1; // Timer0 high priority
    T0CON = 0b00000010;     // 16-bit mode, pre-scaler 1:64
    TMR0H = 0xCF;           // High byte of 26000
    TMR0L = 0xD4;           // Low byte of 26000
}
void start_timer() { T0CONbits.TMR0ON = 1; }

void packet_task() {
    disable_rxtx();
    // Wait until new bytes arrive
    if (!buf_isempty(INBUF)) {
        uint8_t v;
        if (packet_state == PACKET_WAIT) {
            v = buf_pop(INBUF);
            if (v == PACKET_HEADER) {
                // Packet header is encountered, retrieve the rest of the packet
                packet_state = PACKET_GET;
                packet_size = 0;
            }
        }
        if (packet_state == PACKET_GET) {
            v = buf_pop(INBUF);
            if (v == PACKET_END) {
                // End of packet is encountered, signal calc_task())
                packet_state = PACKET_ACK;
                packet_valid = 1;
            } else if (v == PACKET_HEADER) {
                // Unexpected packet start. Abort current packet and restart
                packet_size = 0;
            } else
                packet_data[packet_size++] = v;
        }
        if (packet_state == PACKET_ACK) {
            // Packet is processed, reset state
            packet_state = PACKET_WAIT;
            packet_index++;
        }
    }
    enable_rxtx();
}

uint8_t tk_start; // Start index for the token
uint8_t tk_size;  // Size of the token (0 if no token found)

void tk_reset() {
    tk_start = 0;
    tk_size = 0;
}
// Finds the next token in the packet data body
void tk_next() {
    if (tk_start > packet_size)
        return;                    // Return if no more data
    tk_start = tk_start + tk_size; // Adjust starting location
    tk_size = 0;
    // Skip trailing whitespace, return if no data left in packet
    while (packet_data[tk_start] == ' ' && tk_start < packet_size)
        tk_start++;
    if (tk_start > packet_size)
        return;
    // Search for the next whitespace or end of packet
    while (packet_data[tk_start + tk_size] != ' ' && tk_start + tk_size < packet_size) {
        tk_size++;
    }
}

void output_packet(void) {
    uint8_t ind = 0;
    while (ind < packet_size) {
        disable_rxtx();
        if (ind == tk_start)
            buf_push('$', OUTBUF);
        else if (ind == tk_start + tk_size)
            buf_push('#', OUTBUF);
        buf_push(packet_data[ind++], OUTBUF);
        enable_rxtx();
    }
}
void output_str(char *str) {
    uint8_t ind = 0;
    while (str[ind] != 0) {
        disable_rxtx();
        buf_push(str[ind++], OUTBUF);
        enable_rxtx();
    }
}
void output_int(int32_t v) {
    const char hex_digits[] = "0123456789ABCDEF";

    char vstr[16];
    uint8_t str_ptr = 0;

    if (v == 0) {
        vstr[str_ptr++] = '0';
    } else {
        while (v != 0) {
            vstr[str_ptr++] = hex_digits[v % 16];
            v = v / 16;
        }
    }

    while (str_ptr != 0) {
        disable_rxtx();
        buf_push(vstr[--str_ptr], OUTBUF);
        enable_rxtx();
    }
}

typedef enum { OUTPUT_INIT,
               OUTPUT_RUN } output_st_t;
output_st_t output_st = OUTPUT_INIT;
void output_task() {
    if (output_st == OUTPUT_INIT) {
        output_str("*** CENG 336 Serial Calculator V1 ***\n");
        output_st = OUTPUT_RUN;
    } else if (output_st == OUTPUT_RUN) {
        disable_rxtx();
        if (!buf_isempty(OUTBUF) && TXSTA1bits.TXEN == 0) {
            TXSTA1bits.TXEN = 1;
            TXREG1 = buf_pop(OUTBUF);
        }
        enable_rxtx();
    }
}

void main(void) {
    init_ports();
    init_usart();
    init_interrupt();
    init_timers();
    start_timer();
    while (1) {
        packet_task();
        output_task();
    }
    return;
}
