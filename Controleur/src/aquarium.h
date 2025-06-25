// An Aquarium has a size, has fishes, holds a list of views, and can be saved to a file.
// Is this how classes work in c lol? -OOP main

#ifndef AQUARIUM_H
#define AQUARIUM_H

#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include "doubly_linked_list.h"
#include "utils.h"

#define MAX_NAME_LEN 50  // Warning: If change, update the strings that say %49s

#define MAX_FISH_SIZE 1000000

extern pthread_mutex_t mutex_aquarium;
extern int table_size;

// Fish list
typedef struct Fish {
    char name[MAX_NAME_LEN];
    bool started;
    bool arrived;
    bool to_delete;
    int w, h;  // Size
    Tuple (*move_function) (struct Fish*);  // Fonction de déplacement
    double speed;  // Speed in pixels per second
    DoublyLinkedList *future_positions;  // Contains the next positions (x, y, arrival_time) of the fish
    struct Fish *suivant;  // Liste chaînée
} Fish;

extern int fish_count;

// Map of function names to their corresponding functions
struct FunctionMapping {
    char* nom;
    Tuple (*fonction)(struct Fish*); // Déclaration directe du pointeur de fonction
};
extern struct FunctionMapping table[];

// View list
typedef struct Afficheur {
    char name[MAX_NAME_LEN];
    int x, y;  // Position
    int w, h;  // Size
    int socket;  // -1 if not connected
    microseconds_t last_update_time;  // Last time the view was updated
    bool subscribed;  // If the view is subscribed to getFishesContinuously

    struct Afficheur *suivant;  // Liste chaînée
} Afficheur;

// Aquarium list
typedef struct Aquarium {
    char name[MAX_NAME_LEN];
    int w, h;  // Size
    Fish *poissons;  // Fish list
    Afficheur *afficheurs;  // View list
} Aquarium;

// Currently selected aquarium
extern Aquarium* current_aquarium;

// Create a new empty aquarium
void create_aquarium(const char* name, int w, int h);

// Destroy an aquarium
void destroy_aquarium();

// Add a fish to the aquarium
bool add_fish(
    char* name,
    int x, int y,
    int w, int h,
    char move_function[MAX_NAME_LEN]
);

// Loops over all fishes and, if needed, gives them a a new target position
void update_fishes();

// logs out any views that have lost connection
void disconnect_views();

// Remove a fish from the aquarium
bool release_fish(const char* name);

// Add a view to the aquarium
bool add_view(
    char* name,
    int x, int y,
    int w, int h,
    int socket  // -1 if not connected
);

// Find a free view in the aquarium. NULL if no free view
Afficheur* find_free_view();

// Subscribe a view to getFishesContinuously by setting the subscribed flag
bool subscribe_view(Afficheur* view, bool unsubscribe);

// Get the aquarium coordinates for a view position
Tuple get_aquarium_coordinates(int xView, int yView, Afficheur* view);

// Get the view coordinates for an absolute aquarium position
Tuple get_view_coordinates(int x, int y, Afficheur* view);

// Remove a view from the aquarium
bool delete_view(const char* name);

// Get the path to the aquarium file, assuming
// the aquariums are stored in the "aquariums" directory
char* get_aquarium_path(const char* aquarium_name);

// Aquariums are stored in files in the following format:
// 
// Aquarium_width x Aquarium_height
// N1 view_x x view_y + view_w + view_h
// N2 view_x x view_y + view_w + view_h
// ...
// 
// Example:
// 800x600
// N1 0x0+500+500
// N2 500x0+500+500
// N3 0x500+500+500
// N4 500x500+500+500
void save_aquarium(const char* aquarium_name);

// Aquariums are stored in files in the following format:
// 
// Aquarium_width x Aquarium_height
// N1 view_x x view_y + view_w + view_h
// N2 view_x x view_y + view_w + view_h
// ...
// 
// Example:
// 800x600
// N1 0x0+500+500
// N2 500x0+500+500
// N3 0x500+500+500
// N4 500x500+500+500
void load_aquarium(const char* aquarium_name);

// Calculates a random position in the aquarium
Tuple RandomWayPoint(struct Fish *p);

// Fonction pour vérifier si une fonction est dans la table
int fonctionExiste(const char* nom);

// Calculates the next n future target positions for a fish (using it's move function)
// and appends them to the end of the future_positions list
void add_n_fish_target_positions(struct Fish *p, int n);

// Makes sure the fish has at least n target positions in the future_positions list
void fill_up_fish_positions_list(struct Fish *p, int n);

// Create string of the fish list. If mode_ls, don't remove the fish that have reached their target position
char* create_fish_list_string(microseconds_t curr_time_us, bool mode_ls, Afficheur* view);

// Calcule la vitesse d'un poisson en pixels par seconde
double get_random_fish_speed_px_per_sec();

#endif // AQUARIUM_H
