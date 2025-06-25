#include "log.h"
#include <pthread.h>
#include <stdarg.h>
#include <string.h>

WINDOW* output_win = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_logger(WINDOW* win) {
    output_win = win;
    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_WHITE, -1);  // [INFO]
        init_pair(2, COLOR_YELLOW, -1); // [WARN]
        init_pair(3, COLOR_RED, -1);    // [ERROR]
    }
}

static int get_color(const char* tag) {
    if (strncmp(tag, "[INFO]", 6) == 0) return 1;
    if (strncmp(tag, "[WARN]", 6) == 0) return 2;
    if (strncmp(tag, "[ERROR]", 7) == 0) return 3;
    return 0;
}

void log_msg(const char* format, ...) {
    if (!output_win) return;

    pthread_mutex_lock(&log_mutex);

    // Extract tag for color
    int color_pair = 0;
    if (format[0] == '[') {
        char tag[8] = {0};
        strncpy(tag, format, 7);
        color_pair = get_color(tag);
    }

    if (color_pair > 0) wattron(output_win, COLOR_PAIR(color_pair));

    va_list args;
    va_start(args, format);
    vw_printw(output_win, format, args);
    va_end(args);

    if (color_pair > 0) wattroff(output_win, COLOR_PAIR(color_pair));

    wrefresh(output_win);
    pthread_mutex_unlock(&log_mutex);
}
