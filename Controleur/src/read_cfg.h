#ifndef READ_CFG_H
#define READ_CFG_H

#include <stdbool.h>

#define BUFFER_SIZE_CFG 256

extern int CONTROLLER_PORT;
extern int DISPLAY_TIMEOUT;
extern int FISH_UPDATE_INTERVAL;

bool read_cfg(const char* filename);

#endif // READ_CFG_H
