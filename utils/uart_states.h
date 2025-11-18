#ifndef UART_STATES_H
#define UART_STATES_H

#include "../main.h"

typedef enum {
    check_connection_st,
    check_version_st,
    check_dev_eui_st,
    stop_st
} uart_st;

typedef struct uart_sm {
    uart_st state;
} uart_sm;

void run_uart_sm(uart_sm *smi, bool *continue_loop);
bool check_connection(); // Send "AT" and verify that the module responds
bool check_version(); // Read and print firmware version with "AT+VER"
bool check_dev_eui(); // Read, print, and format DevEui with "AT+ID=DevEui"

#endif