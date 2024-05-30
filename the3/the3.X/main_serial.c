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
typedef enum {
  PACKET_WAIT,
  PACKET_GET,
  PACKET_ACK,
} State;

#define MAX_PACKET_SIZE 16
#define PACKET_HEADER '$'
#define PACKET_END '#'

char *packet_header_str = "$";

char *packet_end_str = "#";

// counter to check called functions;
int debug_counter = 0;

uint8_t send_flag100ms = 0;
uint16_t remaining_dist;
uint16_t adc_interval = 0;
uint8_t prev_portb = 0;
typedef enum { AUTOMATIC, MANUAL } mode_t;
mode_t mode = AUTOMATIC;

uint8_t packet_data[MAX_PACKET_SIZE];
uint8_t packet_size = 0;
uint8_t packet_index = 0;
uint8_t packet_valid = 0;
State packet_state = PACKET_WAIT;
#define BUFSIZE 128
typedef enum { INBUF, OUTBUF } buf_t;
volatile uint8_t inbuf[BUFSIZE];
volatile uint8_t outbuf[BUFSIZE];
volatile uint8_t head[2] = {0, 0};
volatile uint8_t tail[2] = {0, 0};

volatile int buffer_index = 0;
volatile int message_started = 0;

uint16_t adc_value = 0;
uint16_t height = 0;

// disables receive and transmit interrupts
inline void disable_rxtx(void) {
  PIE1bits.RC1IE = 0;
  PIE1bits.TX1IE = 0;
}

// enables receive and transmit interrupts
inline void enable_rxtx(void) {
  PIE1bits.RC1IE = 1;
  PIE1bits.TX1IE = 1;
}

void reset_timer_values() {
  TMR0H = 0x85;
  TMR0L = 0xEE;
}

#pragma interrupt_level 2 // Prevents duplication of function
uint8_t buf_isempty(buf_t buf) { return (head[buf] == tail[buf]) ? 1 : 0; }

#pragma interrupt_level 2 // Prevents duplication of function
// writes the character v to the buffer specified by buf
void buf_push(uint8_t v, buf_t buf) {
  if (buf == INBUF)
    inbuf[head[buf]] = v;
  else
    outbuf[head[buf]] = v;
  head[buf]++;
  if (head[buf] ==
      BUFSIZE) // if we have reached to the end, go back to the start of buffer
    head[buf] = 0;
  if (head[buf] == tail[buf]) { // if the buffer is full (head reached tail)
    return;
  }
}
/* Retrieve data from buffer */
#pragma interrupt_level 2 // Prevents duplication of function
// reads and returns the char currently at the tail of the buffer,
// the buffer is specified by the buf parameter
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
    if (tail[buf] == BUFSIZE) // end of the buffer array, go to start.
      tail[buf] = 0;
    return v;
  }
}

// receive interrupt subroutine
// clear the receive interrupt flag
// write the received character to the input buffer (push buffer)
void receive_isr() {
  PIR1bits.RC1IF = 0;      // Acknowledge interrupt
  buf_push(RCREG1, INBUF); // Buffer incoming byte
}

// transmit interrupt subroutine
// clear the transmit interrupt flag
// write the tail end of buffer to TXREG (pop buffer)
// if there is no data left in the buffer to write (head == tail), turn of
// transmission
void transmit_isr() {
  PIR1bits.TX1IF = 0; // Acknowledge interrupt
  // If all bytes are transmitted, turn off transmission
  if (buf_isempty(OUTBUF))
    TXSTA1bits.TXEN = 0;
  // Otherwise, send next byte
  else
    TXREG1 = buf_pop(OUTBUF);
  while (TXSTA1bits.TRMT == 0) {
  }
}

uint8_t send_altitude =
    0; // flag to check if we should send altitude information
uint8_t send_buttonpress[4] = {
    0, 0, 0,
    0}; // flag to check if we should send button press information RB7-4

uint8_t alt_counter = 0;
uint8_t start_adc_flag = 0;

void __interrupt(high_priority) highPriorityISR(void) {
  if (PIR1bits.ADIF) {
    // ADC interrupt
    // read the ADC result
    // set the flag to send the altitude information
    // clear the interrupt flag
    adc_value = (ADRESH << 8) + ADRESL;
    send_altitude = 1;
    PIR1bits.ADIF = 0;
  }
  if (PIR1bits.RC1IF)
    receive_isr();
  if (PIR1bits.TX1IF)
    transmit_isr();
  if (INTCONbits.TMR0IF) {
    send_flag100ms = 1;
    reset_timer_values();
    INTCONbits.TMR0IF = 0;
    if (adc_interval != 0) {
      if (alt_counter == adc_interval) {
        alt_counter = 0;
        start_adc_flag = 1;
      }
      alt_counter++;
    }
  }
  if (INTCONbits.RBIF) {
    uint8_t portb_read = PORTB;
    uint8_t changed_pins =
        (prev_portb ^ portb_read) & portb_read; // taken from our the2 code
    if (changed_pins & 0b10000000) {
      send_buttonpress[0] = 1;
    }
    if (changed_pins & 0b01000000) {
      send_buttonpress[1] = 1;
    }
    if (changed_pins & 0b00100000) {
      send_buttonpress[2] = 1;
    }
    if (changed_pins & 0b00010000) {
      send_buttonpress[3] = 1;
    }
    prev_portb = PORTB;
    INTCONbits.RBIF = 0;
  }
}
void __interrupt(low_priority) lowPriorityISR(void) {}

void init_usart() {
  unsigned int brg = 21; // (40000000 / (16 * 115200));
  SPBRG1 = brg;
}
void init_adc() {
  ADCON0 = 0b00110001;
  ADCON1 = 0x00;
  ADCON2bits.ADFM = 1;     // Right justified
  ADCON2bits.ACQT = 0b101; // 12 tad
  ADCON2bits.ADCS = 0b010; // Fosc/32
  ADRESH = 0x00;
  ADRESL = 0x00;
  PIR1bits.ADIF = 0; // Clear ADC interrupt flag
  PIE1bits.ADIE = 1; // Enable ADC interrupt
}
#define SPBRG_VAL (21)
void init_ports() {
  TRISB = 0b11110000; // RB4-7 are input
  TRISCbits.RC7 = 1;
  TRISCbits.RC6 = 0;
  TRISHbits.RH4 = 1;
  TRISAbits.RA0 = 0;
  TRISBbits.RB0 = 0;
  TRISCbits.RC0 = 0;
  TRISDbits.RD0 = 0;
  LATA = 0;
  LATB = 0;
  LATC = 0;
  LATD = 0;
  prev_portb = PORTB;
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

  INTCONbits.RBIF = 0;  // clear interrupt flag
  INTCONbits.RBIE = 0;  // disabled because we start at automatic mode
  INTCON2bits.RBIP = 0; // high priority

  RCONbits.IPEN = 1;     // Enable interrupt priority
  INTCONbits.PEIE = 1;   // Enable peripheral interrupts
  INTCONbits.TMR0IE = 1; // Enable Timer0 interrupt
  INTCONbits.TMR0IF = 0; // Clear Timer0 interrupt flag
  INTCONbits.GIE = 1;    // globally enable interrupts
}

void init_timers() {
  INTCON2bits.TMR0IP = 1; // Timer0 high priority
  T0CON = 0b00000100;     // 16-bit mode, pre-scaler 1:32
  reset_timer_values();
}
void start_timer() { T0CONbits.TMR0ON = 1; }

// disable interrupts, since we are working on the packet
// read the incoming data (written into the buffer) into the packet, one
// character if reading the packet is complete, set packet_valid to one reenable
// interrupts on the way out.
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
        // End of packet is encountered
        packet_state = PACKET_ACK;
        packet_valid = 1;
      } else if (v == PACKET_HEADER) {
        // Unexpected packet start. Abort current packet and restart
        packet_size = 0;
      } else {
        // write the character into the end of the packet
        packet_data[packet_size] = v;
        packet_size++;
      }
    }
    if (packet_state == PACKET_ACK) {
      // Packet is processed, reset state
      packet_state = PACKET_WAIT;
      packet_index++; // not necessary for us, which packet we have received now
    }
  }
  enable_rxtx();
}

uint8_t tk_start; // Start index for the token
uint8_t tk_size;  // Size of the token (0 if no token found)

// We don't use the following two, but let's keep it in case
void tk_reset() {
  tk_start = 0;
  tk_size = 0;
}
// Finds the next token in the packet data body
void tk_next() {
  if (tk_start > packet_size)
    return;                      // Return if no more data
  tk_start = tk_start + tk_size; // Adjust starting location
  tk_size = 0;
  // Skip trailing whitespace, return if no data left in packet
  while (packet_data[tk_start] == ' ' && tk_start < packet_size)
    tk_start++;
  if (tk_start > packet_size)
    return;
  // Search for the next whitespace or end of packet
  while (packet_data[tk_start + tk_size] != ' ' &&
         tk_start + tk_size < packet_size) {
    tk_size++;
  }
}

void output_packet(void) {
  uint8_t ind = 0;
  disable_rxtx();
  buf_push(PACKET_HEADER, OUTBUF); // Push the packet header first
  while (ind < packet_size) {
    buf_push(packet_data[ind++], OUTBUF);
  }
  buf_push(PACKET_END, OUTBUF); // Push the packet end character
  enable_rxtx();
}

// push the string (char*) in the parameter to outbuf
void output_str(char *str) {
  uint8_t ind = 0;
  while (str[ind] != 0) {
    disable_rxtx();
    buf_push(str[ind++], OUTBUF);
    enable_rxtx();
  }
}

// push int v in the parameter to outbuf
void output_int(int32_t v, uint8_t is_four) {
  char vstr[16];
  int str_len, str_ptr;

  // Convert integer to hex string
  if (is_four)
    sprintf(vstr, "%04x", v);
  else
    sprintf(vstr, "%02x", v);

  // Get the length of the string
  str_len = strlen(vstr);

  // Output each character to the buffer
  for (str_ptr = 0; str_ptr < str_len; str_ptr++) {
    disable_rxtx();
    buf_push(vstr[str_ptr], OUTBUF);
    enable_rxtx();
  }
}

// push commands to the buffer
void push_dst() {
  output_str("DST");
  output_int(remaining_dist, 1);
}
void push_alt() {
  output_str("ALT");
  output_int(height, 1);
}
void push_buttonpress(uint8_t button) {
  output_str("PRS");
  output_int(button, 0);
}

void start_adc() { GODONE = 1; }

void convert_adc_to_height() {
  if (adc_value < 256)
    height = 9000;
  else if (adc_value < 512)
    height = 10000;
  else if (adc_value < 768)
    height = 11000;
  else {
    height = 12000;
  }
}

void send_sensor_information() {
  output_str(packet_header_str);
  if (send_altitude == 1) {
    convert_adc_to_height();
    push_alt();
    send_altitude = 0;
    output_str(packet_end_str);
    return;
  }
  for (int i = 0; i < 4; i++) {
    if (send_buttonpress[i] == 1) {
      push_buttonpress(i);
      send_buttonpress[i] = 0;
      output_str(packet_end_str);
      return;
    }
  }
  push_dst();
  output_str(packet_end_str);
}

typedef enum { OUTPUT_INIT, OUTPUT_RUN } output_st_t;

output_st_t output_st = OUTPUT_INIT;

void output_task() {
  disable_rxtx();
  if (!buf_isempty(OUTBUF) && TXSTA1bits.TXEN == 0) {
    TXSTA1bits.TXEN = 1;
    // TXREG1 = buf_pop(OUTBUF);
  }
  enable_rxtx();
}

uint16_t val; // writing the value read from command here

void process_go() {
  remaining_dist = val;
  start_timer();
}

void process_spd() { remaining_dist -= val; }

void process_alt() {
  // 0 or 200 or 400 or 600 ms
  adc_interval = val / 100;
  // divide it by 100 to how many times our timer should overflow to send
  // altitude message
  alt_counter = 0;
}

void process_man() {
  if (val == 1) {
    mode = MANUAL;
    INTCONbits.RBIE = 1; // enable button press interrupt
  } else if (val == 0) {
    mode = AUTOMATIC;
    INTCONbits.RBIE = 0; // disable button press interrupt
  }
}

void process_led() {
  if (val == 0) {
    LATA = 0;
    LATB = 0;
    LATC = 0;
    LATD = 0;
  } else if (val == 1) {
    LATD = 0b00000001;
  } else if (val == 2) {
    LATC = 0b00000001;
  } else if (val == 3) {
    LATB = 0b00000001;
  } else if (val == 4) {
    LATA = 0b00000001;
  }
}

void process_end() {
  // maybe we can just exit instead of doing these idk
  disable_rxtx();
  send_flag100ms = 0;
  send_altitude = 0;
  for (int i = 0; i < 4; i++) {
    send_buttonpress[i] = 0;
  }
  // disable all interrupts
  INTCONbits.RBIE = 0;
  INTCONbits.TMR0IE = 0;
  INTCONbits.PEIE = 0;
  INTCONbits.GIE = 0;
}

void process() {
  uint8_t ind = 0;

  // sometimes our packets would start with null values.
  // we couldn't find why, we solved it this way.
  while (packet_data[ind] == 0)
    ind++;
  if (packet_data[ind] == 'G') {
    // GOO
    sscanf(&packet_data[ind + 3], "%04x", &val);
    process_go();
  } else if (packet_data[ind] == 'M') {
    // MAN
    sscanf(&packet_data[ind + 3], "%02x", &val);
    process_man();
  } else if (packet_data[ind] == 'L') {
    // LED
    sscanf(&packet_data[ind + 3], "%02x", &val);
    process_led();
  } else if (packet_data[ind] == 'A') {
    // ALT
    sscanf(&packet_data[ind + 3], "%04x", &val);
    process_alt();
  } else if (packet_data[ind] == 'S') {
    // SPD
    sscanf(&packet_data[ind + 3], "%04x", &val);
    process_spd();
  } else if (packet_data[ind] == 'E') {
    // END
    process_end();
  }
}

void main(void) {
  init_ports();
  init_usart();
  init_adc();
  init_timers();
  init_interrupt();
  while (1) {
    if (start_adc_flag) {
      start_adc();
      start_adc_flag = 0;
    }

    packet_task();
    output_task();
    if (packet_valid) {
      process();
      packet_valid = 0;
    }
    if (send_flag100ms) {
      send_sensor_information();
      send_flag100ms = 0;
      output_st = OUTPUT_INIT;
    }
  }
  return;
}
