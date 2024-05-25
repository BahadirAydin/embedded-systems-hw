/*
 * File:   main_serial.c
 * Author: e2380798
 *
 * Created on May 25, 2024, 5.17 PM
 */
#include "pragmas.h"
#include <xc.h>
#include <string.h>

#define BUFFER_SIZE 128

volatile char buffer[BUFFER_SIZE];
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

void __interrupt(high_priority) highPriorityISR(void) {
  if (PIR1bits.RC1IF == 1) {
    char rcvd_chr = RCREG1;
    PIR1bits.RC1IF = 0;

    if (rcvd_chr == '$') {
      message_started = 1;
      buffer_index = 0;
    }

    if (message_started) {
      if (rcvd_chr == '#') {
        message_started = 0;
        buffer[buffer_index] = '\0'; // Null-terminate the string
        // Process the received message here
        // For example, you can print it or set a flag to indicate a new message is ready
      } else if (buffer_index < BUFFER_SIZE - 1) {
        buffer[buffer_index++] = rcvd_chr;
      } else {
        // Buffer overflow, reset the message state
        message_started = 0;
        buffer_index = 0;
      }
    }
  }
}

void init_usart() {
  unsigned int brg = 21; // (40000000 / (16 * 115200));
  SPBRG1 = brg;
}

void init_ports() {
  TRISCbits.RC7 = 1;
  TRISCbits.RC6 = 0;
}

void init_interrupt() {
  /* configure I/O ports */
  TRISCbits.RC7 = 1; // TX1 and RX1 pin configuration
  TRISCbits.RC6 = 0;

  /* configure USART transmitter/receiver */
  TXSTA1 = 0x04; // (= 00000100) 8-bit transmit, transmitter NOT enabled,TXSTA1.TXEN
                 // not enabled! asynchronous, high speed mode
  RCSTA1 = 0x90; // (= 10010000) 8-bit receiver, receiver enabled,
                 // continuous receive, serial port enabled RCSTA.CREN = 1
  BAUDCON1bits.BRG16 = 0;

  /* configure the interrupts */
  INTCON = 0;         // clear interrupt register completely
  PIE1bits.TX1IE = 1; // enable USART transmit interrupt
  PIE1bits.RC1IE = 1; // enable USART receive interrupt
  PIR1 = 0;           // clear all peripheral flags

  INTCONbits.PEIE = 1; // enable peripheral interrupts
  INTCONbits.GIE = 1;  // globally enable interrupts
}

void main(void) {
  init_ports();
  init_usart();
  init_interrupt();
  while (1) {
    if (!message_started && buffer_index > 0) {
      buffer_index = 0;
    }
  }
  return;
}

