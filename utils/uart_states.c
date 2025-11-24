#include "uart_states.h"

void run_uart_sm(uart_sm *smi, bool *continue_loop) {
    switch (smi->state) {
        case check_connection_st:
            if (check_connection()) {
                printf("Connected to LoRa module\r\n");
                smi->state = check_version_st;
            }
            else {
                printf("Module not responding\r\n");
                smi->state = stop_st;
            }
            break;
        case check_version_st:
            if (check_version())
                smi->state = check_dev_eui_st;
            else {
                printf("Module stopped responding\r\n");
                smi->state = stop_st;
            }
            break;
        case check_dev_eui_st:
            if (check_dev_eui()) {
                smi->state = check_connection_st;
                *continue_loop = false;
            }
            else {
                printf("Module stopped responding\r\n");
                smi->state = stop_st;
            }
            break;
        case stop_st:
            smi->state = check_connection_st;
            *continue_loop = false;
    }
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
            return convert_and_print(line);
        }
    }
    return false;
}