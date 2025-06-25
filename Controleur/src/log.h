#ifndef LOG_H
#define LOG_H

#include <ncurses.h>
#include <stdarg.h>

extern WINDOW* output_win;

void init_logger(WINDOW* win);
void log_msg(const char* format, ...);

#endif  // LOG_H
