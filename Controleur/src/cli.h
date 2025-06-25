#ifndef CLI_H
#define CLI_H

#include <ncurses.h>

typedef struct {
    WINDOW* input_win;
    WINDOW* output_win;
} CliContext;

// handles load <aquarium> command
void handle_load(WINDOW* output_win, const char* message);

// handles show <aquarium> command
void handle_show(WINDOW* output_win, const char* message);

// handles add view <Name> <geometry> command
void handle_add(WINDOW* output_win, const char* message);

// handles del view <Name> command
void handle_del(WINDOW* output_win, const char* message);

// handles save <aquarium> command
void handle_save(WINDOW* output_win, const char* message);

// handles help command
void handle_help(WINDOW* output_win);

// main loop for CLI prompt
int cli(WINDOW* input_win, WINDOW* output_win);

void set_input_window(WINDOW* win);

#endif // CLI_H
