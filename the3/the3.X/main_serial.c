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
}
void __interrupt(low_priority) lowPriorityISR(void) {}

void init_usart() {
    unsigned int brg = 21; // (40000000 / (16 * 115200));
    SPBRG1 = brg;
}

void init_ports() {
    TRISCbits.RC7 = 1;
    TRISCbits.RC6 = 0;
}

void init_interrupt() {
    enable_rxtx();
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1; // globally enable interrupts
}

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

void main(void) {
    init_ports();
    init_usart();
    init_interrupt();
    while (1) {
        packet_task();
    }
    return;
}
