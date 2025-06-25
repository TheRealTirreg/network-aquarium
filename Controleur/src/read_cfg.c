#include <stdio.h>
#include "read_cfg.h"
#include "utils.h"
#include "log.h"

int CONTROLLER_PORT = 12345;
int DISPLAY_TIMEOUT = 45;      // s
int FISH_UPDATE_INTERVAL = 1;  // s

bool read_cfg(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        log_msg("[ERROR] Error opening config file: %s\n", filename);
        return false;
    }
    log_msg("[INFO] Found config file: %s\n", filename);

    char line[BUFFER_SIZE_CFG];
    while (fgets(line, sizeof(line), file)) {
        // Trim leading and trailing whitespaces
        trim(line);

        // Read line "controller-port = <port>"
        if (sscanf(line, "controller-port = %d", &CONTROLLER_PORT) == 1) {
            log_msg("[INFO] Controller port set to: %d\n", CONTROLLER_PORT);
        }
        // Read line "display-timeout-value = <timeout>"
        else if (sscanf(line, "display-timeout-value = %d", &DISPLAY_TIMEOUT) == 1) {
            log_msg("[INFO] Display timeout set to: %d seconds\n", DISPLAY_TIMEOUT);
        }
        // Read line "fish-update-interval = <interval>"
        else if (sscanf(line, "fish-update-interval = %d", &FISH_UPDATE_INTERVAL) == 1) {
            log_msg("[INFO] Fish update interval set to: %d seconds\n", FISH_UPDATE_INTERVAL);
        }
    }

    fclose(file);
    return true;
}
