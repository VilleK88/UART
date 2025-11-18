#include "tools.h"

// Send a string to the LoRa module using UART
void write_str(const char *string) {
    while (*string) {
        uart_putc_raw(UART, *string++);
    }
    uart_putc_raw(UART, '\r');
    uart_putc_raw(UART, '\n');
}

// Read a single line from UART into buffer with timeout
bool read_line(char *buffer, const int len, const int timeout_ms) {
    const uint32_t us = timeout_ms * 1000; // convert to microseconds
    // Wait for data to become available within timeout
    if (uart_is_readable_within_us(UART, us)) {
        int i = 0;
        while (i < len -1) {
            //const char c = getchar_timeout_us(UART);
            //const char c = uart_getc(UART);
            const char c = get_c(UART);
            if (c != '\n') {
                if (c != '\r') // Ignore carriage return
                    buffer[i++] = c;
            }
            else break; // End of line
        }
        buffer[i] = '\0'; // Null-terminate resulting string
        return true;
    }
    // No data received within timeout
    return false;
}

// Convert DevEui response line into hex string and print it
void convert_and_print(const char *line) {
    const char *line_after_comma = strchr(line, ','); // Find comma after "DevEui"
    line_after_comma += 2; // Skip ", " to point at first hex digit
    const int len = (int)strlen(line_after_comma);
    char current_hexadecimal[5]; // Temporary buffer for each group
    int j = 0;
    for (int i = 0; i <= len; i++) {
        // Copy characters until ':' or temporary buffer is full
        if (line_after_comma[i] != ':' && j < 4) {
            current_hexadecimal[j++] = tolower((unsigned char)line_after_comma[i]);
        }
        else {
            // Terminate current group and print it
            current_hexadecimal[j] = '\0';
            printf("%s", current_hexadecimal);
            j = 0;
        }
    }
    printf("\r\n");
}

static inline void uart_read(uart_inst_t *uart, uint8_t *dst, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (uart_is_readable_within_us(uart, 500 * 10)) {
            tight_loop_contents();
        }
        *dst++ = (uint8_t) uart_get_hw(uart)->dr;
    }
}

static inline char get_c(uart_inst_t *uart) {
    char c;
    uart_read(uart, (uint8_t *) &c, 1);
    return c;
}