#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "utils/uart_states.h"
#include "utils/tools.h"

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "pico/util/queue.h"

#define SW_0 9 // left button

#define UART uart1 // LoRa module UART1
#define UART_TX 4 // UART0 TX (GP4) - to LoRa
#define UART_RX 5 // UART0 RX (GP5) - from LoRa

#define BAUD_RATE 9600 // LoRa module UART speed

#define LINE_LEN 128 // Maximum line length for UART input buffer

// AT commands for the LoRa-E5 module
#define CMD_AT "AT"
#define CMD_VERSION "AT+VER"
#define CMD_DEV_EUI "AT+ID=DevEui"

#define DEBOUNCE_MS 20 // Debounce delay in milliseconds

// Type of event coming from the interrupt callback
typedef enum { EVENT_BUTTON } event_type;

// Generic event passed from ISR to main loop through a queue
typedef struct {
    event_type type; // EVENT_BUTTON
    int32_t data; // BUTTON: 1 = press, 0 = release;
} event_t;

// Global event queue used by ISR (Interrupt Service Routine) and main loop
static queue_t events;

#endif