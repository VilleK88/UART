#include "main.h"

void gpio_callback(uint gpio, uint32_t event_mask);
void init_button(); // Initialize button SW_0
void init_uart(); // Initialize UART

int main() {
    uart_sm tsm = { check_connection_st };
    bool start_st = false;

    // Initialize chosen serial port
    stdio_init_all();
    // Initialize buttons and event queue + interrupt
    init_button();
    // Initialize UART
    init_uart();

    event_t event;
    while (true) {
        // Process pending events from the queue
        while (queue_try_remove(&events, &event)) {
            // React only to button press (falling edge event, data == 1)
            if (event.type == EVENT_BUTTON && event.data == 1) {
                start_st = true;
            }
        }

        if (start_st)
            run_uart_sm(&tsm, &start_st);

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

void init_button() {
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

void init_uart() {
    // Initialize UART1 for LoRa module
    uart_init(UART, BAUD_RATE);
    gpio_set_function(UART_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_RX, GPIO_FUNC_UART);
    // Configure UART as 8 data bits, 1 stop bit, no parity (8N1)
    uart_set_format(UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART, true);
}