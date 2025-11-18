#ifndef TOOLS_H
#define TOOLS_H

#include "../main.h"
#include "hardware/uart.h"

void write_str(const char *string); // Send a null-terminated string over UART
bool read_line(char *buffer, int len, int timeout_ms); // Read one line from UART with timeout
void convert_and_print(const char *line); // Convert DevEui response to required format
static inline void uart_read(uart_inst_t *uart, uint8_t *dst, size_t len);
static inline char get_c(uart_inst_t *vart);

#endif