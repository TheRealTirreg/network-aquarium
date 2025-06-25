// load <aquarium> (e.g. load aquarium1 
//     -> loads a specific aquarium and tells
//        how many users are connected)
// show aquarium (shows the total size of the aquarium and
//     the positioning of the connected screens)
// add view <Name> <top_left+w+h> 
//     (e.g. add view N5 400x400+400+200)
// del view <Name> (e.g. del view N5)
// save <aquarium> (e.g. save aquarium2)
// help (shows this message)
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include "aquarium.h"
#include "cli.h"

#define NUM_COMMANDS 6
#define BUFFER_SIZE 1024

void handle_load(WINDOW* output_win, const char* message) {
    char buffer[BUFFER_SIZE];
    strncpy(buffer, message, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char* tok = strtok(buffer, " ");  // load
    tok = strtok(NULL, " ");          // aquarium name
    strncpy(buffer, message, sizeof(buffer));

    if (tok == NULL) {
        wprintw(output_win, "No aquarium name provided.\n");
        wprintw(output_win, "Did you mean load <aquarium>?\n");
        return;
    }
    
    pthread_mutex_lock(&mutex_aquarium);

    // Load the aquarium
    load_aquarium(tok);
    if (current_aquarium == NULL) {
        pthread_mutex_unlock(&mutex_aquarium);
        wprintw(output_win, "Could not load aquarium named '%s'.\n", tok);
        return;
    }

    wprintw(output_win, "Loaded aquarium: %s\n", current_aquarium->name);
    wprintw(output_win, "Size: %d x %d\n", current_aquarium->w, current_aquarium->h);
    wprintw(output_win, "Views:\n");
    Afficheur* view = current_aquarium->afficheurs;
    while (view != NULL) {
        wprintw(output_win, "  %s: %d x %d + %d + %d\n", view->name, view->x, view->y, view->w, view->h);
        view = view->suivant;
    }

    pthread_mutex_unlock(&mutex_aquarium);
}

void handle_show(WINDOW* output_win, const char* message) {
    char buffer[BUFFER_SIZE];
    strncpy(buffer, message, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char* tok = strtok(buffer, " ");  // "show"
    tok = strtok(NULL, " ");          // "aquarium"

    // Check if tok is "aquarium"
    if (tok == NULL || strcmp(tok, "aquarium") != 0) {
        wprintw(output_win, "Did you mean show aquarium?\n");
        return;
    }
    
    pthread_mutex_lock(&mutex_aquarium);

    // Check if the aquarium is loaded
    if (current_aquarium == NULL) {
        pthread_mutex_unlock(&mutex_aquarium);
        wprintw(output_win, "No aquarium loaded. Load an aquarium first.\n");
        return;
    }

    wprintw(output_win, "Aquarium: %s\n", current_aquarium->name);
    wprintw(output_win, "Size: %d x %d\n", current_aquarium->w, current_aquarium->h);
    wprintw(output_win, "Views:\n");
    Afficheur* view = current_aquarium->afficheurs;
    while (view != NULL) {
        wprintw(output_win, "  %s: %d x %d + %d + %d\n", view->name, view->w, view->h, view->x, view->y);
        view = view->suivant;
    }

    pthread_mutex_unlock(&mutex_aquarium);
}

void handle_add(WINDOW* output_win, const char* message) {
    char viewName[BUFFER_SIZE];
    char geometry[BUFFER_SIZE];
    int xTopLeft;
    int yTopLeft;
    int w;
    int h;

    char buffer[BUFFER_SIZE];
    strncpy(buffer, message, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char* tok = strtok(buffer, " ");  // "add"
    tok = strtok(NULL, " ");          // "view"

    // check if tok is "view"
    if (!tok || strcmp(tok, "view") != 0) {
        wprintw(output_win, "Did you mean add view <Name> <geometry>?\n");
        return;
    }

    tok = strtok(NULL, " ");          // view name
    if (!tok) {
        wprintw(output_win, "Did you mean add view <Name> <geometry>?\n");
        return;
    }
    strncpy(viewName, tok, sizeof(viewName));
    viewName[sizeof(viewName) - 1] = '\0';

    tok = strtok(NULL, " ");          // geometry
    if (!tok) {
        wprintw(output_win, "Did you mean add view <Name> <geometry>?\n");
        return;
    }
    strncpy(geometry, tok, sizeof(geometry));
    geometry[sizeof(geometry) - 1] = '\0';

    // Now parse the geometry string like "400x300+100+200"
    strncpy(buffer, geometry, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char* w_str = strtok(buffer, "x");
    char* h_str = strtok(NULL, "+");
    char* x_str = strtok(NULL, "+");
    char* y_str = strtok(NULL, "+");

    if (!w_str || !h_str || !x_str || !y_str) {
        wprintw(output_win, "Invalid geometry format. Expected WxH+X+Y\n");
        return;
    }

    w = atoi(w_str);
    h = atoi(h_str);
    xTopLeft = atoi(x_str);
    yTopLeft = atoi(y_str);

    pthread_mutex_lock(&mutex_aquarium);

    // Check if the view name already exists in the aquarium
    if (current_aquarium == NULL) {
        pthread_mutex_unlock(&mutex_aquarium);
        wprintw(output_win, "No aquarium loaded. Load an aquarium first.\n");
        return;
    }  

    Afficheur* current_view = current_aquarium->afficheurs;
    while (current_view != NULL) {
        if (strcmp(current_view->name, viewName) == 0) {
            pthread_mutex_unlock(&mutex_aquarium);
            wprintw(output_win, "Could not add view '%s':\n", viewName);
            wprintw(output_win, "View with the same name '%s' already exists.\n", viewName);
            return;
        }
        current_view = current_view->suivant;
    }

    // Create the view
    Afficheur* view = (Afficheur*)malloc(sizeof(Afficheur));
    strncpy(view->name, viewName, sizeof(view->name));
    view->name[sizeof(view->name) - 1] = '\0';
    view->w = w;
    view->h = h;
    view->x = xTopLeft;
    view->y = yTopLeft;

    wprintw(output_win, "Parsed view '%s' with geometry:\n", viewName);
    wprintw(output_win, "  Width:  %d\n", w);
    wprintw(output_win, "  Height: %d\n", h);
    wprintw(output_win, "  X:      %d\n", xTopLeft);
    wprintw(output_win, "  Y:      %d\n", yTopLeft);
    
    // Add to end of list
    view->suivant = NULL;
    if (current_aquarium->afficheurs == NULL) {
        current_aquarium->afficheurs = view;
    } else {
        Afficheur* current_view = current_aquarium->afficheurs;
        while (current_view->suivant != NULL) {
            current_view = current_view->suivant;
        }
        current_view->suivant = view;
    }

    wprintw(output_win, "Added view '%s' to aquarium '%s'\n", viewName, current_aquarium->name);
    
    pthread_mutex_unlock(&mutex_aquarium);
}

void handle_del(WINDOW* output_win, const char* message) {
    char viewName[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    strncpy(buffer, message, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char* tok = strtok(buffer, " ");  // "del"
    tok = strtok(NULL, " ");          // "view"

    // check if tok is "view"
    if (!tok || strcmp(tok, "view") != 0) {
        wprintw(output_win, "Did you mean del view <Name>?\n");
        return;
    }

    tok = strtok(NULL, " ");          // view name
    if (!tok) {
        wprintw(output_win, "Did you mean del view <Name>?\n");
        return;
    }
    strncpy(viewName, tok, sizeof(viewName));
    viewName[sizeof(viewName) - 1] = '\0';
    
    pthread_mutex_lock(&mutex_aquarium);

    // Find the view in the list
    Afficheur* current_view = current_aquarium->afficheurs;
    Afficheur* previous_view = NULL;
    while (current_view != NULL) {
        if (strcmp(current_view->name, viewName) == 0) {
            // Found the view
            if (previous_view == NULL) {
                // If first view, update aquarium list start
                current_aquarium->afficheurs = current_view->suivant;
            } else {
                // Else, update view list links
                previous_view->suivant = current_view->suivant;
            }

            free(current_view);
            wprintw(output_win, "Deleted view '%s' from aquarium '%s'\n", viewName, current_aquarium->name);
            pthread_mutex_unlock(&mutex_aquarium);
            return;
        }
        previous_view = current_view;
        current_view = current_view->suivant;
    }

    wprintw(output_win, "View '%s' not found in aquarium '%s'\n", viewName, current_aquarium->name);
    
    pthread_mutex_unlock(&mutex_aquarium);
}

void handle_save(WINDOW* output_win, const char* message) {
    char aquarium[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    strncpy(buffer, message, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char* tok = strtok(buffer, " ");  // "save"

    tok = strtok(NULL, " ");          // aquarium to save
    if (!tok) {
        wprintw(output_win, "Did you mean save <aquarium>?\n");
        return;
    }
    strncpy(aquarium, tok, sizeof(aquarium));
    aquarium[sizeof(aquarium) - 1] = '\0';
    
    pthread_mutex_lock(&mutex_aquarium);

    // Check if the aquarium is loaded
    if (current_aquarium == NULL) {
        pthread_mutex_unlock(&mutex_aquarium);
        wprintw(output_win, "No aquarium loaded. Load an aquarium first.\n");
        return;
    }

    save_aquarium(aquarium);

    pthread_mutex_unlock(&mutex_aquarium);
    // TODO should this function delete the aquarium from memory?
}

void handle_help(WINDOW* output_win) {
    wprintw(output_win, "Available commands:\n");
    wprintw(output_win, "  load <aquarium>\n");
    wprintw(output_win, "  show\n");
    wprintw(output_win, "  add view <Name> <geometry>\n");
    wprintw(output_win, "  del view <Name>\n");
    wprintw(output_win, "  save <aquarium>\n");
    wprintw(output_win, "  help\n");
}

// Create a list of available commands:
// load <aquarium> (e.g. load aquarium1 
//     -> loads a specific aquarium and tells
//        how many users are connected)
// show aquarium (shows the total size of the aquarium and
//     the positioning of the connected screens)
// add view <Name> <geometry> 
//     (e.g. add view N5 400x400+400+200)
// del view <Name> (e.g. del view N5)
// save <aquarium> (e.g. save aquarium2)
// help (shows this message)
int cli(WINDOW* input_win, WINDOW* output_win) {
    char input[BUFFER_SIZE];
    
    while (1) {
        werase(input_win);
        wattron(input_win, A_BOLD);
        mvwprintw(input_win, 0, 0, "Entrez une commande > ");
        wattroff(input_win, A_BOLD);
        wmove(input_win, 0, strlen("Entrez une commande > "));
        wclrtoeol(input_win);
        wrefresh(input_win);

        wgetnstr(input_win, input, BUFFER_SIZE - 1);
        if (input[0] == '\0') continue;

        if (strncmp(input, "load", 4) == 0 &&
            (input[4] == ' ' || input[4] == '\0'))
        {
            handle_load(output_win, input);
        }
        else if (strncmp(input, "show", 4) == 0 &&
                 (input[4] == ' ' || input[4] == '\0'))
        {
            handle_show(output_win, input);
        }
        else if (strncmp(input, "add", 3) == 0 &&
                 (input[3] == ' ' || input[3] == '\0'))
        {
            handle_add(output_win, input);
        }
        else if (strncmp(input, "del", 3) == 0 &&
                 (input[3] == ' ' || input[3] == '\0'))
        {
            handle_del(output_win, input);
        }
        else if (strncmp(input, "save", 4) == 0 &&
                 (input[4] == ' ' || input[4] == '\0'))
        {
            handle_save(output_win, input);
        }
        else if (strncmp(input, "help", 4) == 0 &&
                 (input[4] == ' ' || input[4] == '\0'))
        {
            handle_help(output_win);
        }
        else {
            wprintw(output_win, "Commande inconnue: %s\n", input);
            handle_help(output_win);
        }
        wrefresh(output_win);
    }

    // Never reached, but for completeness
    delwin(output_win);
    delwin(input_win);
    endwin();
    return 0;
}
