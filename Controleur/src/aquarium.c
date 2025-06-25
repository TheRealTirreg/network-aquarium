#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <arpa/inet.h>
#include "aquarium.h"
#include "log.h"

#define MAX_PATH_LEN 256
#define MAX_FISH_LIST_SIZE 4096
#define MAX_FISH_INFO_SIZE 256

// Mutex for aquarium access
pthread_mutex_t mutex_aquarium = PTHREAD_MUTEX_INITIALIZER;

// Currently selected aquarium
Aquarium* current_aquarium = NULL;
int fish_count = 0;

struct FunctionMapping table[] = {
    {"RandomWayPoint", RandomWayPoint}
};

int table_size = sizeof(table) / sizeof(table[0]);


void create_aquarium(const char* name, int w, int h) {  // Assumes the mutex is locked
    if (current_aquarium == NULL) {
        current_aquarium = (Aquarium*)malloc(sizeof(Aquarium));
    }

    strncpy(current_aquarium->name, name, MAX_NAME_LEN - 1);
    current_aquarium->name[MAX_NAME_LEN - 1] = '\0';
    current_aquarium->w = w;
    current_aquarium->h = h;
    current_aquarium->poissons = NULL;
    current_aquarium->afficheurs = NULL;

    log_msg("Created aquarium: %s\n", name);
}

void destroy_aquarium() {  // Assumes the mutex is locked
    if (current_aquarium == NULL) {
        log_msg("No aquarium to destroy\n");
        return;
    }

    log_msg("Destroying aquarium: %s\n", current_aquarium->name);

    // Release fish
    Fish* current_fish = current_aquarium->poissons;
    while (current_fish != NULL) {
        Fish* next_fish = current_fish->suivant;
        destroy_list(current_fish->future_positions);  // Free the future positions list
        free(current_fish);
        current_fish = next_fish;
    }

    // Free the list of views
    Afficheur* current_view = current_aquarium->afficheurs;
    while (current_view != NULL) {
        Afficheur* next_view = current_view->suivant;
        free(current_view);
        current_view = next_view;
    }

    free(current_aquarium);
}

// -------------------------- Fish --------------------------------

bool add_fish(  // Assumes the mutex is locked
    char* name,
    int x, int y,
    int w, int h,
    char move_function[MAX_NAME_LEN]
) {
    // Check if name is too long. If so, truncate it
    if (strlen(name) >= MAX_NAME_LEN) {
        log_msg("Name '%s' is too long, truncating to %d characters\n", name, MAX_NAME_LEN - 1);
        name[MAX_NAME_LEN - 1] = '\0';
    }

    // Check if the fish already exists
    Fish* current_fish = current_aquarium->poissons;
    while (current_fish != NULL) {
        if (strncmp(current_fish->name, name, MAX_NAME_LEN) == 0) {
            // Fish with the same name already exists
            log_msg("Fish with the same name %s already exists\n", name);
            return false;
        }
        current_fish = current_fish->suivant;
    }

    // Check the position
    if (x > 100 || x < 0 || y > 100 || y < 0)
        return false;

    // Check the size
    if (w * h > MAX_FISH_SIZE)
        return false;

    // Check mobility
    int index=fonctionExiste(move_function);
    if (index == 0)
        return 0;

    // Calculate aquarium coordinates from view coordinates
    Tuple aquarium_coords = get_aquarium_coordinates(x, y, current_aquarium->afficheurs);
    log_msg("Aquarium size: %d x %d\n", current_aquarium->w, current_aquarium->h);
    log_msg("Fish: (%d, %d) View: %dx%d, %dx%d\n", x, y, current_aquarium->afficheurs->x, current_aquarium->afficheurs->y, current_aquarium->afficheurs->w, current_aquarium->afficheurs->h);
    log_msg("Aquarium coordinates: (%d, %d)\n", aquarium_coords.x, aquarium_coords.y);

    // Initialize the new fish
    Fish* fish = (Fish*)malloc(sizeof(Fish));
    strncpy(fish->name, name, MAX_NAME_LEN - 1);
    fish->name[MAX_NAME_LEN - 1] = '\0';
    fish->w = w;
    fish->h = h;
    fish->started = false;
    fish->arrived = false;
    fish->to_delete = false;
    fish->move_function = table[index-1].fonction;
    fish->future_positions = create_list();  // Initialize the future positions queue
    fish->speed = get_random_fish_speed_px_per_sec();

    // Add to the beginning of the list
    fish->suivant = current_aquarium->poissons;
    current_aquarium->poissons = fish;
    fish_count++;

    // Add current fish position to the future positions list with the current time
    FishNextPos current_position;
    current_position.x = aquarium_coords.x;
    current_position.y = aquarium_coords.y;
    current_position.arrival_time = get_time_usec();
    insert_back(fish->future_positions, current_position);

    // Generate n=3 future positions
    add_n_fish_target_positions(fish, 3);

    return true;
}

bool release_fish(const char* name) {  // Assumes the mutex is locked
    // Catch the fish (find in the list)
    Fish* current_fish = current_aquarium->poissons;
    Fish* previous_fish = NULL;
    while (current_fish != NULL) {
        if (strncmp(current_fish->name, name, MAX_NAME_LEN) == 0) {
            // Found the fish

            // Actually release/delete the fish
            if (current_fish->to_delete) {
                if (previous_fish == NULL) {
                    // If we release the first fish, update the aquarium
                    current_aquarium->poissons = current_fish->suivant;
                } else {
                    // Else, update fish list links
                    previous_fish->suivant = current_fish->suivant;
                }

                // Release the fish into the wilderness
                destroy_list(current_fish->future_positions);  // Free the future positions list
                free(current_fish);
                fish_count--;
            } else {
                // Mark the fish for deletion
                current_fish->to_delete = true;
            }
            return true;
        }
        previous_fish = current_fish;
        current_fish = current_fish->suivant;
    }

    log_msg("Fish with name %s not found\n", name);
    return false;  // Fish not found
}

// Removes the first position of each fish where the target position has been reached
// (and thus the arrival_time is in the past)
void pop_reached_targets(microseconds_t curr_time_us) {  // Assumes the mutex is locked TODO Function not needed?
    // Loop through all fishes
    Fish* current_fish = current_aquarium->poissons;
    while (current_fish != NULL) {
        // Get the next position
        FishNextPos* next_position = peek_front(current_fish->future_positions);

        // If the fish has reached its target position
        if (curr_time_us >= next_position->arrival_time) {
            pop_front(current_fish->future_positions);
            add_n_fish_target_positions(current_fish, 1);
        }
        current_fish = current_fish->suivant;
    }
}

// Returns true if the fish has reached its target position (This means, we can send the fish list to the subscribed clients)
bool update_fish(Fish* p, microseconds_t curr_time_us) {  // Assumes the mutex is locked
    // Check if the fish is NULL or if it hasnt started yet
    if (p == NULL || !p->started) {
        return false;
    }

    // Fill up the future positions list if needed
    if (p->future_positions == NULL || p->future_positions->size <= 1) {
        fill_up_fish_positions_list(p, 3);
    }

    // Get the next position
    FishNextPos* next_position = peek_front(p->future_positions);

    // If the fish has reached its target position
    if (curr_time_us >= next_position->arrival_time) {
        p->arrived = true;
        return true;
    }

    return false;
}

// Create string of the fish list
char* create_fish_list_string(microseconds_t curr_time_us, bool mode_ls, Afficheur* view) {  // Assumes the mutex is locked
    // Check if the aquarium is loaded
    if (current_aquarium == NULL) {
        return NULL;
    }

    // Create a string to hold the fish list
    char* fish_list = (char*)malloc(MAX_FISH_LIST_SIZE);
    strcpy(fish_list, "list");

    // Loop through all fishes
    Fish* current_fish = current_aquarium->poissons;
    while (current_fish != NULL) {
        // If the fish is not started, skip it
        if (!current_fish->started) {
            if (current_fish->to_delete) release_fish(current_fish->name);
            current_fish = current_fish->suivant;
            continue;
        }

        // Get the next position of the fish
        FishNextPos* next_position = peek_front(current_fish->future_positions);
        if (next_position == NULL) {
            log_msg("[create_fish_list_string] Fish %s has no target position\n", current_fish->name);  // This should not happen
            current_fish = current_fish->suivant;
            continue;  // Skip this fish
        }

        int seconds_to_reach = (next_position->arrival_time - curr_time_us) / 1000000;
        if (seconds_to_reach < 0) seconds_to_reach = 0;  // No time left
        // If the fish is marked for deletion, we want to send it one last time with seconds_to_reach = -1
        if (current_fish->to_delete) seconds_to_reach = -1;

        // Convert to view coordinates
        Tuple view_coords = get_view_coordinates(
            next_position->x,
            next_position->y,
            view
        );
        // log_msg("Converted aquarium coordinates (%d, %d) to view coordinates (%d, %d)\n",
        //        next_position->x, next_position->y, view_coords.x, view_coords.y);

        // Format the fish information
        char fish_info[MAX_FISH_INFO_SIZE];
        snprintf(
            fish_info, MAX_FISH_INFO_SIZE, " [\"%s\" at %dx%d,%dx%d,%d]",
            current_fish->name,
            view_coords.x, view_coords.y,
            current_fish->w, current_fish->h,
            seconds_to_reach
        );
        strcat(fish_list, fish_info);

        // If the fish is marked for deletion, we want to send it one last time with seconds_to_reach = -1
        if (current_fish->to_delete) {
            Fish* next_fish = current_fish->suivant;
            // Release the fish
            release_fish(current_fish->name);
            current_fish = next_fish;
            continue;
        }

        // If the fish has not reached its target position, continue to the next fish (we are done with this fish).
        // Also, if mode_ls, we don't want to remove the fish from the list (as we can just cover that in the next call)
        if (seconds_to_reach != 0 || mode_ls) {
            current_fish = current_fish->suivant;
            continue;
        }

        // If seconds_to_reach is 0, we will add the same fish AGAIN to the list, with a new target position.
        if (current_fish->future_positions->size == 0) {
            add_n_fish_target_positions(current_fish, 1);  // If needed, add a new target position
            log_msg("[create_fish_list_string] Fish %s has no target position, adding a new one\n", current_fish->name);
        }

        // Get the new next position
        next_position = peek_at_index(current_fish->future_positions, 1);
        seconds_to_reach = (next_position->arrival_time - curr_time_us) / 1000000;
        if (seconds_to_reach < 3) {
            log_msg("[create_fish_list_string] seconds_to_reach < 3, setting to 3\n");
            seconds_to_reach = 3;  // No time left
        }
        view_coords = get_view_coordinates(
            next_position->x,
            next_position->y,
            view
        );
        snprintf(
            fish_info, MAX_FISH_INFO_SIZE, " [\"%s\" at %dx%d,%dx%d,%d]",
            current_fish->name,
            view_coords.x, view_coords.y,
            current_fish->w, current_fish->h,
            seconds_to_reach
        );
        strcat(fish_list, fish_info);
        
        current_fish = current_fish->suivant;
    }

    strcat(fish_list, "\n");
    return fish_list;
}

void send_fish_list_to_view(Afficheur* view, char* fish_list) {  // Assumes the mutex is locked
    // Send the fish list to the view
    if (view->socket != -1) {
        send(view->socket, fish_list, strlen(fish_list), 0);
    } else {
        log_msg("View %s is not connected\n", view->name);
    }
}

void update_fishes() {  // Assumes the mutex is locked
    // Check if the aquarium is loaded
    if (current_aquarium == NULL) {
        return;
    }

    microseconds_t current_time_us = get_time_usec();
    bool send_fish_list = false;
    bool arrived_arr[fish_count];

    // Loop through all fishes
    Fish* current_fish = current_aquarium->poissons;
    int i = 0;
    while (current_fish != NULL) {
        bool arrived = update_fish(current_fish, current_time_us);
        arrived_arr[i] = arrived;
        send_fish_list = send_fish_list || arrived;
        current_fish = current_fish->suivant;
        i++;
    }

    if (!send_fish_list) {
        return;  // Nothing to do if no fish has reached its target position
    }

    Afficheur* current_view = current_aquarium->afficheurs;
    while (current_view != NULL) {
        if (current_view->subscribed) {

            // If any fish has reached its target position, send the fish list to the subscribed views
            char* fish_list = create_fish_list_string(current_time_us, false, current_view);
            if (fish_list == NULL) {
                log_msg("No fish list available\n");
                return;
            }
            log_msg("=============Continuous update:==============\n");
            log_msg("[%s] %s\n", current_view->name, fish_list);
            // Send the fish list to the view
            send_fish_list_to_view(current_view, fish_list);
            free(fish_list);
        }
        current_view = current_view->suivant;
    }

    // Iterate over the fishes again to remove the reached targets
    current_fish = current_aquarium->poissons;
    i = 0;
    while (current_fish != NULL) {
        // If the fish has reached its target position
        if (arrived_arr[i]) {
            pop_front(current_fish->future_positions);
            // add_n_fish_target_positions(current_fish, 1);
        }
        current_fish = current_fish->suivant;
        i++;
    }

    long long elapsed_us = get_time_usec() - current_time_us;
    double elapsed_ms = elapsed_us / 1000.0;
    log_msg("[update_fishes] Execution time: %.3f ms\n", elapsed_ms);
}

// -------------------------- Views --------------------------------

bool add_view(  // Assumes the mutex is locked
    char* name,
    int x, int y,
    int w, int h,
    int socket  // -1 if not connected
) {
    // Check if name is too long. If so, truncate it
    if (strlen(name) >= MAX_NAME_LEN) {
        log_msg("Name '%s' is too long, truncating to %d characters\n", name, MAX_NAME_LEN - 1);
        name[MAX_NAME_LEN - 1] = '\0';
    }

    // Check if the view already exists
    Afficheur* current_view = current_aquarium->afficheurs;
    while (current_view != NULL) {
        if (strcmp(current_view->name, name) == 0) {
            // View with the same ID already exists
            log_msg("View with the same name '%s' already exists\n", current_view->name);
            return false;
        }
        current_view = current_view->suivant;
    }

    // Initialize the new view
    Afficheur* view = (Afficheur*)malloc(sizeof(Afficheur));
    strncpy(view->name, name, MAX_NAME_LEN - 1);
    view->name[MAX_NAME_LEN - 1] = '\0';
    view->x = x;
    view->y = y;
    view->w = w;
    view->h = h;
    view->socket = socket;
    view->subscribed = 0;

    if (socket != -1) {
        view->last_update_time = get_time_usec();
    } else {
        view->last_update_time = 0;  // Not connected
    }
    
    // Add to the beginning of the list
    view->suivant = current_aquarium->afficheurs;
    current_aquarium->afficheurs = view;

    return true;
}

Afficheur* find_free_view() {  // Assumes the mutex is locked
    // Check if the aquarium is loaded
    if (current_aquarium == NULL) {
        log_msg("No aquarium loaded\n");
        return NULL;
    }

    // Find a free view
    Afficheur* current_view = current_aquarium->afficheurs;
    while (current_view != NULL) {
        if (current_view->socket == -1) {
            return current_view;  // Found a free view
        }
        current_view = current_view->suivant;
    }
    return NULL;  // No free view found
}

void disconnect_views() {  // Assumes the mutex is locked
    // Check if the aquarium is loaded
    if (current_aquarium == NULL) {
        return;
    }

    // Loop through all views and check if the last update time is older than the timeout
    microseconds_t current_time_us = get_time_usec();
    microseconds_t timeout_us = 5 * 1000000;  // 5 second timeout
    Afficheur* current_view = current_aquarium->afficheurs;
    while (current_view != NULL) {
        if (current_view->socket != -1 && current_time_us - current_view->last_update_time > timeout_us) {
            // Disconnect the view
            log_msg("[disconnect_views] Disconnecting view %s after a timeout of %d seconds\n",
                current_view->name, (int)(timeout_us / 1000000));
            send(current_view->socket, "bye timeout\n", 4, 0);
            current_view->socket = -1;
            current_view->subscribed = 0;  // Unsubscribe the view
        }
        current_view = current_view->suivant;
    }
}

// view coordinates (e.g. (40, 60) are percentage of the view size)
Tuple get_aquarium_coordinates(int xView, int yView, Afficheur* view) {  // Assumes the mutex is locked
    if (current_aquarium == NULL) {
        log_msg("No aquarium loaded\n");
        return (Tuple){-1, -1};  // Invalid coordinates
    }
    if (view == NULL) {
        log_msg("Invalid view\n");
        return (Tuple){-1, -1};  // Invalid coordinates
    }

    // Calculate aquarium coordinates
    int x_aquarium = ((xView / 100.0) * view->w) + view->x;
    int y_aquarium = ((yView / 100.0) * view->h) + view->y;

    // Check if the coordinates are within the aquarium bounds
    if (x_aquarium < 0 || x_aquarium > current_aquarium->w ||
        y_aquarium < 0 || y_aquarium > current_aquarium->h) {
        log_msg("[get_aquarium_coordinates] Coordinates (%d, %d) are outside the aquarium bounds (%d, %d)\n",
            x_aquarium, y_aquarium, current_aquarium->w, current_aquarium->h);
        return (Tuple){-1, -1};  // Invalid coordinates
    }

    log_msg("Aquarium coordinates: (%d, %d) from view coordinates (%d, %d)\n",
        x_aquarium, y_aquarium, xView, yView);
    return (Tuple){x_aquarium, y_aquarium};
}

Tuple get_view_coordinates(int x, int y, Afficheur* view) {  // Assumes the mutex is locked
    if (current_aquarium == NULL) {
        log_msg("No aquarium loaded\n");
        return (Tuple){-1, -1};  // Invalid coordinates
    }
    if (view == NULL) {
        log_msg("Invalid view\n");
        return (Tuple){-1, -1};  // Invalid coordinates
    }

    // Calculate percentage in relation to the view size
    // (x, y) in aquarium coordinates, (x_view, y_view) in view coordinates
    // View coordinates example: 40x50 means 40% of the view width and 50% of the view height
    // Example: view top left corner (10, 20), view size 50x50
    // Example coordinates (20, 30) in aquarium coordinates
    // View coordinates in percentage of view size:
    // x_view = (20 - 10) * 100 / 50 = 20%
    // y_view = (30 - 20) * 100 / 50 = 20%
    int x_view = (int)((float)(x - view->x) / (float)view->w * 100.0f);
    int y_view = (int)((float)(y - view->y) / (float)view->h * 100.0f);
    return (Tuple){x_view, y_view};
}

bool delete_view(const char* name) {  // Assumes the mutex is locked TODO Function not needed?
    // Find view in list
    Afficheur* current_view = current_aquarium->afficheurs;
    Afficheur* previous_view = NULL;
    while (current_view != NULL) {
        if (strcmp(current_view->name, name) == 0) {
            // Found the view
            if (previous_view == NULL) {
                // If first view, update aquarium list start
                current_aquarium->afficheurs = current_view->suivant;
            } else {
                // Else, update view list links
                previous_view->suivant = current_view->suivant;
            }

            // Release the view into the wilderness
            free(current_view);
            return true;
        }
        previous_view = current_view;
        current_view = current_view->suivant;
    }

    log_msg("View with name '%s' not found\n", name);
    return false;  // View not found
}

// -------------------------- Save & Load --------------------------------

char* get_aquarium_path(const char* aquarium_name) {
    char* path = (char*)malloc(MAX_PATH_LEN * sizeof(char));
    snprintf(path, MAX_PATH_LEN, "aquariums/%s", aquarium_name);
    return path;  // When using this, don't forget to free the memory!
}

void save_aquarium(const char* aquarium_name) {  // Assumes the mutex is locked
    char* filename = get_aquarium_path(aquarium_name);
    if (current_aquarium == NULL) {
        log_msg("No aquarium is currently loaded.\n");
        free(filename);
        return;
    }

    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        log_msg("[ERROR] Error opening file %s\n", filename);
        free(filename);
        return;
    }

    // Write aquarium size
    fprintf(file, "%dx%d\n", current_aquarium->w, current_aquarium->h);

    // Write views
    Afficheur* current_view = current_aquarium->afficheurs;
    while (current_view != NULL) {
        fprintf(
            file, "%s %dx%d+%d+%d\n", 
            current_view->name, 
            current_view->x, current_view->y, 
            current_view->w, current_view->h
        );
        current_view = current_view->suivant;
    }

    fclose(file);
    log_msg("Saved aquarium '%s' to %s\n", aquarium_name, filename);
    free(filename);
}

void load_aquarium(const char* aquarium_name) {  // Assumes the mutex is locked
    char* filename = get_aquarium_path(aquarium_name);

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        log_msg("[ERROR] Error opening file %s\n", filename);
        log_msg("[ERROR] The file needs to be in the 'aquariums' directory.\n");
        free(filename);
    }

    // Destroy the current aquarium if it exists
    if (current_aquarium != NULL) {
        destroy_aquarium();
    }

    // Read aquarium size
    int w, h;
    fscanf(file, "%dx%d\n", &w, &h);

    // Create the aquarium
    create_aquarium(aquarium_name, w, h);

    // Read views and add them to the aquarium in inverse order
    // Create list first
    Afficheur* current_view = NULL;
    char name[MAX_NAME_LEN];
    int x, y, view_w, view_h;
    while (fscanf(
                    file, "%49s %dx%d+%d+%d\n",  // 49 = MAX_NAME_LEN - 1
                    name, &x, &y, &view_w, &view_h
                 ) == 5) {
        // TODO: Check if there is no error in the file
        // e.g. same ids, negative values
        Afficheur* view = (Afficheur*)malloc(sizeof(Afficheur));
        strncpy(view->name, name, MAX_NAME_LEN - 1);
        view->name[MAX_NAME_LEN - 1] = '\0';
        view->x = x;
        view->y = y;
        view->w = view_w;
        view->h = view_h;
        view->socket = -1;
        view->subscribed = false;
        view->last_update_time = 0;

        // Add to the beginning of the list
        view->suivant = current_view;
        current_view = view;
    }

    // Now add views to the aquarium in inverse order
    while (current_view != NULL) {
        Afficheur* next_view = current_view->suivant;
        add_view(
            current_view->name,
            current_view->x, current_view->y,
            current_view->w, current_view->h,
            current_view->socket
        );
        free(current_view);
        current_view = next_view;
    }

    fclose(file);
    free(filename);
}

double get_random_fish_speed_px_per_sec() {
    double max_px_per_sec = 50.0;
    double min_px_per_sec = 10.0;
    
    double random_fraction = (double)rand() / RAND_MAX;  // Random value between 0.0 and 1.0
    return min_px_per_sec + random_fraction * (max_px_per_sec - min_px_per_sec);
}

// Calculer le mesures de temps (en us) pour arriver à destination en utilisant la vitesse du poisson
microseconds_t get_duration_time_interval(struct Fish *p, int x_from, int y_from, int x_to, int y_to) {  // Assumes the mutex is locked
    microseconds_t duration_time_interval = (microseconds_t)(
        sqrt(pow((x_from - x_to), 2) + pow((y_from - y_to), 2)) / p->speed * 1000000);

    // Check if the duration is too small (less than 2 seconds)
    if (duration_time_interval < 5000000) {
        duration_time_interval = 5000000;  // 5 seconds
    }
    return duration_time_interval;
}

void add_n_fish_target_positions(struct Fish *p, int n) {  // Assumes the mutex is locked
    // Check if the aquarium is loaded
    if (current_aquarium == NULL) {
        log_msg("No aquarium loaded\n");
        return;
    }

    // Check if the fish is valid
    if (p == NULL) {
        log_msg("Invalid fish\n");
        return;
    }

    // Check if the number of positions is valid
    if (n <= 0) {
        log_msg("Invalid number of positions: %d\n", n);  // TODO test for n too big
        return;
    }

    // Initialize the future positions list if it doesn't exist
    if (p->future_positions == NULL) {
        p->future_positions = create_list();
    }

    // Get the absolute time when the fish needs a new destination
    microseconds_t current_time_us = get_time_usec();
    microseconds_t abs_arrival_time = current_time_us;
    microseconds_t swim_duration;

    // Get the latest position in the future positions list
    FishNextPos* latest_position = peek_back(p->future_positions);
    if (latest_position == NULL) {
        log_msg("No future positions available for fish %s\n", p->name);  // TODO test: This should never be the case
        return;
    }
    int x_from = latest_position->x;
    int y_from = latest_position->y;

    // Precalculate the next n positions using the move function
    for (int i = 0; i < n; i++) {
        // Debug prints
        log_msg("=========================\n");
        log_msg("Fish %s: Adding target position %d\n", p->name, i + 1);
        log_msg("Current position: (%d, %d)\n", x_from, y_from);
        log_msg("Current time: %lld\n", current_time_us);
        log_msg("With speed: %f px/s\n", p->speed);

        // Get random destination
        Tuple destination = p->move_function(p);  // E.g. RandomWayPoint(p)
        swim_duration = get_duration_time_interval(p, x_from, y_from, destination.x, destination.y);
        abs_arrival_time += swim_duration;

        // Get the next position
        FishNextPos next_pos;
        next_pos.x = destination.x;
        next_pos.y = destination.y;
        next_pos.arrival_time = abs_arrival_time;

        // Add the next position to the list
        insert_back(p->future_positions, next_pos);

        // Update the current position
        x_from = next_pos.x;
        y_from = next_pos.y;

        // Debug prints
        log_msg("Fish %s next position: (%d, %d) within %lld seconds\n",
            p->name, next_pos.x, next_pos.y, (long long)(swim_duration / 1000000));
        
        // long long time_diff = next_pos.arrival_time - current_time_us;
        // log_msg("Fish %s next position: (%d, %d) at time %lld (in %d seconds)\n",
        //     p->name, next_pos.x, next_pos.y, time_diff, (int)(time_diff / 1000000));
    }
}

void fill_up_fish_positions_list(struct Fish *p, int n)  // Assumes the mutex is locked
{
    // If there are no or not enough positions, create them
    if (p->future_positions == NULL) {
        add_n_fish_target_positions(p, n);
    } else if ((int)p->future_positions->size < n) {
        add_n_fish_target_positions(p, n - p->future_positions->size);
    }
}

// Create a random waypoint for the fish. The destination will take more than 2 seconds to reach
Tuple RandomWayPoint(struct Fish *p) {  // Assumes the mutex is locked
    const int min_x = 0;
    const int min_y = 0;
    const int max_x = current_aquarium->w - p->w;
    const int max_y = current_aquarium->h - p->h;

    int x = rand() % (max_x - min_x + 1) + min_x;
    int y = rand() % (max_y - min_y + 1) + min_y;

    return (Tuple){x, y};
}

int fonctionExiste(const char* nom) {  // Assumes the mutex is locked
    for (int i = 0; i < table_size; i++) {
        if (strcmp(table[i].nom, nom) == 0) {
            return i+1;
        }
    }
    return 0;
}
