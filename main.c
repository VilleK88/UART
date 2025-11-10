#include <stdio.h>
#include "hardware/pwm.h"
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "pico/util/queue.h"

#define SW_0 9 // left button

#define UART uart0 // LoRa module UART0
#define UART_TX 0 // UART0 TX (GP0) - to LoRa
#define UART_RX 1 // UART0 RX (GP1) - from LoRa

#define BAUD_RATE 9600 // LoRa module UART speed

#define LINE_LEN 128 // Maximum line length for UART input buffer

// AT commands for the LoRa-E5 module
#define CMD_AT "AT\r\n"
#define CMD_VERSION "AT+VER\r\n"
#define CMD_DEV_EUI "AT+ID=DevEui\r\n"

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

void gpio_callback(uint gpio, uint32_t event_mask);
void ini_button(); // Initialize button SW_0
bool check_connection(); // Send "AT" and verify that the module responds
bool check_version(); // Read and print firmware version with "AT+VER"
bool check_dev_eui(); // Read, print, and format DevEui with "AT+ID=DevEui"
void write_str(const char *string); // Send a null-terminated string over UART
bool read_line(char *buffer, int len, int timeout_ms); // Read one line from UART with timeout
void convert_and_print(const char *line); // Convert DevEui response to required format

int main() {
    // Initialize chosen serial port
    stdio_init_all();
    // Initialize buttons and event queue + interrupt
    ini_button();

    // Initialize UART0 for LoRa module
    uart_init(UART, BAUD_RATE);
    gpio_set_function(UART_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_RX, GPIO_FUNC_UART);
    // Configure UART as 8 data bits, 1 stop bit, no parity (8N1)
    uart_set_format(UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART, true);

    event_t event;
    while (true) {
        // Process pending events from the queue
        while (queue_try_remove(&events, &event)) {
            // React only to button press (falling edge event, data == 1)
            if (event.type == EVENT_BUTTON && event.data == 1) {
                // 1. Check AT connectivity
                if (check_connection()) {
                    printf("Connected to LoRa module\r\n");
                    // 2. Read firmware version
                    if (check_version()) {
                        // 3. Read and process DevEui
                        if (!check_dev_eui())
                            printf("Module not responding\r\n");
                    }
                    else
                        printf("Module not responding\r\n");
                }
                else
                    printf("Module not responding\r\n");
            }
        }

        sleep_ms(10); // 10 ms delay (0.01 second) to reduce CPU usage
    }
}

// Interrupt callback for pressing SW_0
void gpio_callback(uint const gpio, uint32_t const event_mask) {
    // Button press/release with debounce to ensure one physical press counts as one event
    if (gpio == SW_0) {
        static uint32_t last_ms = 0; // Store last interrupt time
        const uint32_t now = to_ms_since_boot(get_absolute_time());

        // Detect button release (rising edge)
        if (event_mask & GPIO_IRQ_EDGE_RISE && now - last_ms >= DEBOUNCE_MS) {
            last_ms = now;
            const event_t event = { .type = EVENT_BUTTON, .data = 0 };
            queue_try_add(&events, &event); // Add event to queue
        }

        // Detect button press (falling edge)
        if (event_mask & GPIO_IRQ_EDGE_FALL && now - last_ms >= DEBOUNCE_MS){
            last_ms = now;
            const event_t event = { .type = EVENT_BUTTON, .data = 1 };
            queue_try_add(&events, &event); // Add event to queue
        }
    }
}

void ini_button() {
    gpio_init(SW_0); // Initialize GPIO pin
    gpio_set_dir(SW_0, GPIO_IN); // Set as input
    gpio_pull_up(SW_0); // Enable internal pull-up resistor (button reads high = true when not pressed)

    // Initialize event queue for Interrupt Service Routine (ISR)
    // 32 chosen as a safe buffer size: large enough to handle bursts of interrupts
    // without losing events, yet small enough to keep RAM usage minimal.
    queue_init(&events, sizeof(event_t), 32);

    // Configure button interrupt and callback
    gpio_set_irq_enabled_with_callback(SW_0, GPIO_IRQ_EDGE_FALL |
        GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
}

// Send "AT" command and check if module responds with a line that contains "OK"
// Tries up to 5 times, each with a 500 ms timeout
bool check_connection() {
    char line[LINE_LEN];
    for (int i = 0; i < 5; i++) {
        write_str(CMD_AT);
        if (read_line(line, sizeof(line), 500)) {
            if (strstr(line, "OK") != NULL)
                return true;
        }
    }
    return false;
}

// Send "AT+VER" and print the firmware version line
// Returns true if a line containing "VER" is received within timeout
bool check_version() {
    char line[LINE_LEN];
    write_str(CMD_VERSION);
    if (read_line(line, sizeof(line), 500)) {
        if (strstr(line, "VER") != NULL) {
            printf("%s\r\n", line);
            return true;
        }
    }
    return false;
}

// Send "AT+ID=DevEui" and print both the raw response and the processed DevEui
// Returns true if a valid DevEui line is received
bool check_dev_eui() {
    char line[LINE_LEN];
    write_str(CMD_DEV_EUI);
    if (read_line(line, sizeof(line), 500)) {
        if (strstr(line, "DevEui") != NULL) {
            printf("%s\r\n", line);
            convert_and_print(line);
            return true;
        }
    }
    return false;
}

// Send a string to the LoRa module using UART
void write_str(const char *string) {
    while (*string) {
        uart_putc_raw(UART, *string++);
    }
}

// Read a single line from UART into buffer with timeout
bool read_line(char *buffer, const int len, const int timeout_ms) {
    const uint32_t us = timeout_ms * 1000; // convert to microseconds
    // Wait for data to become available within timeout
    if (uart_is_readable_within_us(UART, us)) {
        int i = 0;
        while (i < len) {
            const char c = uart_getc(UART);
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
    char current_hexadecimal[5]; // Temporary buffer for each grou
    int j = 0;
    for (int i = 0; i <= len; i++) {
        // Copy characters until ':' or temporary buffer is full
        if (line_after_comma[i] != ':' && j < 4) {
            current_hexadecimal[j++] = line_after_comma[i];
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