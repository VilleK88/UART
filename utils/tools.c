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
    int i = 0;
    while (i <= len) {
        if (uart_is_readable_within_us(UART, us)) {
            const char c = uart_getc(UART);
            if (c != '\n') {
                if (c != '\r') // Ignore carriage return
                    buffer[i++] = c;
            }
            else break; // End of line
        }
        else return false; // No data received within timeout
    }
    buffer[i] = '\0'; // Null-terminate resulting string
    return true;
}

// Convert DevEui response line into hex string and print it
bool convert_and_print(const char *line) {
    if (strchr(line, ',') != NULL) {
        char *line_after_comma = strchr(line, ','); // Find comma after "DevEui"
        line_after_comma += 2; // Skip ", " to point at first hex digit
        strcat(line_after_comma, "\n");
        const int len = (int)strlen(line_after_comma);
        char current_hexadecimal[4]; // Temporary buffer for each group
        int j = 0;
        for (int i = 0; i < len; i++) {
            // Copy characters until ':' or '\n' or temporary buffer is full
            if (line_after_comma[i] != ':' && line_after_comma[i] != '\n') {
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
        return true;
    }
    return false;
}