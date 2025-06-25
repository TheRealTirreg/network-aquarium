#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "handle_client.h"
#include "aquarium.h"
#include "utils.h"
#include "log.h"

char buffer[BUFFER_SIZE];  // Copie locale du message pour strtok


bool aquarium_null_send(int job_socket, const char* send_msg) {
    pthread_mutex_lock(&mutex_aquarium);
    if (current_aquarium == NULL) {
        pthread_mutex_unlock(&mutex_aquarium);
        log_msg("No aquarium available\n");
        char response[BUFFER_SIZE];
        snprintf(response, BUFFER_SIZE, "%s (no aquarium available)\n", send_msg);
        send(job_socket, response, strlen(response), 0);
        return true;
    }
    pthread_mutex_unlock(&mutex_aquarium);
    return false;
}


bool view_null_send(Afficheur* view, int job_socket, const char* send_msg) {
    if (view == NULL) {
        log_msg("No view available\n");
        char response[BUFFER_SIZE];
        snprintf(response, BUFFER_SIZE, "%s (no view available)\n", send_msg);
        send(job_socket, response, strlen(response), 0);
        return true;
    }
    return false;
}


int wrong_msg_received_send_NOK(int job_socket, const char* msg, const char* expected_msg, const char* err_msg) {
    log_msg("Received '%s' instead of '%s'. %s\n", msg, expected_msg, err_msg);
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "NOK Received %s instead of '%s'. %s\n", msg, expected_msg, err_msg);
    send(job_socket, response, strlen(response), 0);
    return -1;
}


int send_NOK(int job_socket, const char* msg) {
    log_msg("Sending NOK: %s\n", msg);
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "NOK %s\n", msg);
    send(job_socket, response, strlen(response), 0);
    return -1;
}


int handle_hello_no_arg(int job_socket) {
    log_msg("Hello without 'in as'\n");

    // If no aquarium, send "no greeting"
    if (aquarium_null_send(job_socket, "no greeting")) {
        return -1;
    }

    pthread_mutex_lock(&mutex_aquarium);

    // Check if there is a free view
    Afficheur* free_view = find_free_view();

    // Handle free_view == NULL
    if (view_null_send(free_view, job_socket, "no greeting")) {
        pthread_mutex_unlock(&mutex_aquarium);
        return -1;
    }

    free_view->socket = job_socket;
    free_view->last_update_time = get_time_usec();
    free_view->subscribed = 0;  // Not subscribed yet
    log_msg("Found a free view: %s\n", free_view->name);

    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "greeting %s %dx%d+%d+%d", 
        free_view->name, 
        free_view->x, free_view->y, 
        free_view->w, free_view->h
    );
    strcat(response, "\n");
    send(job_socket, response, strlen(response), 0);
    log_msg("[hello] Sending 'greeting %s %dx%d+%d+%d'\n", 
        free_view->name, 
        free_view->x, free_view->y, 
        free_view->w, free_view->h
    );

    pthread_mutex_unlock(&mutex_aquarium);
    return 0;
}


// If "hello" without "in as", create a new view and respond with "Greeting <new ID>"
// If "hello in as ID", check if the view ID is valid && not used yet
// In valid case, respond with "Greeting <ID> <X>x<Y>+<w>+<h>"
// Else, either we create a new view and respond with "Greeting <new ID>"
// or we respond with "no greeting" if the aquarium is full
int handle_Hello(int job_socket, const char* message) {
    log_msg("Message reçu (Hello) : '%s'\n", message);

    char buffer[BUFFER_SIZE];
    strncpy(buffer, message, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char* tok = strtok(buffer, " ");  // hello
    tok = strtok(NULL, " ");          // in

    // hello without "in" (viable command according to the specification)
    if (tok == NULL) {
        // Handle hello call and send back response
        return handle_hello_no_arg(job_socket);  // Has its own mutex locking logic

    } else if (!(strncmp(tok, "in", 2) == 0)) {
        // Next token is not "in"
        return wrong_msg_received_send_NOK(job_socket, tok, "in", "Did you mean 'hello' or 'hello in as <view name>'?");
    }

    tok = strtok(NULL, " ");          // as
    if (tok == NULL) {
        // No "as"
        return wrong_msg_received_send_NOK(job_socket, tok, "as", "Did you mean 'hello' or 'hello in as <view name>'?");
    }

    tok = strtok(NULL, " ");          // view name
    if (tok == NULL) {
        // No view name
        return wrong_msg_received_send_NOK(job_socket, tok, "<view name>", "Did you mean 'hello' or 'hello in as <view name>'?");
    }

    // If no aquarium, say "no greeting"
    if (aquarium_null_send(job_socket, "no greeting")) {
        return -1;
    }
    
    pthread_mutex_lock(&mutex_aquarium);

    // Check if there is a view without connection with the same ID.
    // If the requested view name does not exist or is already occupied by another client, 
    // just give the next free view. If there is none, send "no greeting"
    Afficheur* current_view = current_aquarium->afficheurs;
    while (current_view != NULL) {
        if (strcmp(current_view->name, tok) == 0) {  // If view found
            if (current_view->socket != -1) {
                log_msg("[hello] View '%s' already connected\n", current_view->name);
            } else {
                // View not connected, connect it
                current_view->socket = job_socket;
                current_view->last_update_time = get_time_usec();
                current_view->subscribed = 0;  // Not subscribed yet
                log_msg("[hello] Connected view '%s'\n", current_view->name);

                char response[BUFFER_SIZE];
                snprintf(response, BUFFER_SIZE, "greeting %s %dx%d+%d+%d", 
                    current_view->name, 
                    current_view->x, current_view->y, 
                    current_view->w, current_view->h
                );
                strcat(response, "\n");
                send(job_socket, response, strlen(response), 0);
                log_msg("[hello] Sending 'greeting %s %dx%d+%d+%d'\n", 
                    current_view->name, 
                    current_view->x, current_view->y, 
                    current_view->w, current_view->h
                );
                pthread_mutex_unlock(&mutex_aquarium);
                return 0;
            }
        }
        current_view = current_view->suivant;
    }

    // If we reach here, the requested view name does not exist or is already occupied by another client
    // Check if there is a free view
    current_view = current_aquarium->afficheurs;
    while (current_view != NULL) {
        if (current_view->socket == -1) {  // If view not connected
            // Found a free view
            current_view->socket = job_socket;
            current_view->last_update_time = get_time_usec();
            current_view->subscribed = 0;  // Not subscribed yet
            log_msg("[hello] Found a free view: %s\n", current_view->name);

            char response[BUFFER_SIZE];
            snprintf(response, BUFFER_SIZE, "greeting %s %dx%d+%d+%d", 
                current_view->name, 
                current_view->x, current_view->y, 
                current_view->w, current_view->h
            );
            strcat(response, "\n");
            send(job_socket, response, strlen(response), 0);
            log_msg("[hello] Sending 'greeting %s %dx%d+%d+%d'\n", 
                current_view->name, 
                current_view->x, current_view->y, 
                current_view->w, current_view->h
            );
            pthread_mutex_unlock(&mutex_aquarium);
            return 0;
        }
        current_view = current_view->suivant;
    }
    // No free view found
    pthread_mutex_unlock(&mutex_aquarium);
    log_msg("[hello] No free view found\n");
    char response[] = "no greeting (No free view)\n";
    send(job_socket, response, strlen(response), 0);
    return -1;
}


// send the list of fishes in the aquarium, e.g.:
// list ["fish1.name" at "fish_destination_position","fish_size","time_to_reach_destination_seconds"] ["fish2.name" at ...] 
// Warning: the fish_destination_position is a percentage of the view size (e.g. 50x70 meaning 50% of the width and 70% of the height)
// Fish size example: 50x40 (meaning 50 pixels width and 40 pixels height)
// Time to reach destination example: 5 (s)
int handle_getFishes(int job_socket, const char* message) {
    log_msg("Message reçu (getFishes) : %s\n", message);
    
    // If no aquarium, say "no greeting"
    if (aquarium_null_send(job_socket, "no greeting")) {
        return -1;
    }
    char response[BUFFER_SIZE];
    strcpy(response, "list ");

    pthread_mutex_lock(&mutex_aquarium);

    Fish* current_fish = current_aquarium->poissons;
    while (current_fish != NULL) {
        // Find if the client is connected to a view
        Afficheur* current_view = current_aquarium->afficheurs;
        while (current_view != NULL) {
            if (current_view->socket == job_socket) {
                break;  // Found the view
            }
            current_view = current_view->suivant;
        }
        if (current_view == NULL) {
            pthread_mutex_unlock(&mutex_aquarium);
            return wrong_msg_received_send_NOK(job_socket, message, "view", "Client is not connected to a view");
        }

        // Get fish information. It may be the case that the update_fish-thread
        // has not yet updated the fish's target position. Skip in this case.
        FishNextPos* next_position = peek_front(current_fish->future_positions);
        if (next_position == NULL) {
            log_msg("[getFishes] Fish %s has no target position\n", current_fish->name);
            current_fish = current_fish->suivant;
            continue;  // Skip this fish
        }

        // Get view coordinates for the fish.
        // Only return the coordinates of the first target position
        Tuple view_coords = get_view_coordinates(
            next_position->x,
            next_position->y,
            current_view
        );

        // TODO For now, we don't send fishes whose target position is outside the view
        if (view_coords.x < 0 || view_coords.y < 0) {
            log_msg("[getFishes] Fish %s is outside the view\n", current_fish->name);
            current_fish = current_fish->suivant;
            continue;  // Skip this fish
        }

        int seconds_to_reach = (next_position->arrival_time - get_time_usec()) / 1000000;
        if (seconds_to_reach < 0) {
            seconds_to_reach = 0;  // No time left
        }

        char fish_info[BUFFER_SIZE];
        snprintf(fish_info, BUFFER_SIZE, "[\"%s\" at %dx%d,%dx%d,%d]", 
            current_fish->name,
            view_coords.x, view_coords.y,
            current_fish->w, current_fish->h,
            seconds_to_reach
        );
        strcat(response, fish_info);
        current_fish = current_fish->suivant;
        if (current_fish != NULL) {
            strcat(response, " ");
        }
    }

    pthread_mutex_unlock(&mutex_aquarium);

    strcat(response, "\n");
    send(job_socket, response, strlen(response), 0);
    log_msg("[getFishes] Sending '%s'\n", response);

    return 0;
}

// 
int handle_Continuous(int job_socket, const char* message) {
    log_msg("Message reçu (Continuous) : %s\n", message);
    
    pthread_mutex_lock(&mutex_aquarium);
    
    if (current_aquarium == NULL) {
        pthread_mutex_unlock(&mutex_aquarium);
        char response[] = "NOK No aquarium\n";
        send(job_socket, response, strlen(response), 0);
        return -1;
    }

    // Find view by socket
    Afficheur* current_view = current_aquarium->afficheurs;
    while (current_view != NULL) {
        if (current_view->socket == job_socket) {
            current_view->subscribed = true;  // Subscribe to continuous updates
            break;  // Found the view
        }
        current_view = current_view->suivant;
    }
    
    pthread_mutex_unlock(&mutex_aquarium);

    char response[] = "OK Subscribed to getFishesContinuously\n";
    send(job_socket, response, strlen(response), 0);
    return 0;
}

// ls [<n>]
// Precalculates the next n (default 3) positions of the fishes and sends them to the client
int handle_ls(int job_socket, const char* message) {
    struct timeval start, end;
    gettimeofday(&start, NULL);

    log_msg("Message reçu (ls) : %s\n", message);

    // Check if the message is "ls" or "ls <n>"
    int n = 3;
    char buffer[BUFFER_SIZE];
    strncpy(buffer, message, BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';  // Sécurisation de la fin de chaîne

    char* key = strtok(buffer, " ");
    key = strtok(NULL, " ");  // Get "<n>"
    if (key != NULL) {
        n = atoi(key);
        if (n <= 0) {
            return wrong_msg_received_send_NOK(
                job_socket, key, "<n>", 
                "Invalid value for <n> (needs to be an integer). Did you mean 'ls [<n>]'?"
            );
        }
    }
    
    pthread_mutex_lock(&mutex_aquarium);

    if (current_aquarium == NULL) {
        pthread_mutex_unlock(&mutex_aquarium);
        char response[] = "NOK No aquarium\n";
        send(job_socket, response, strlen(response), 0);
        return -1;
    }

    // Loop through all fishes and make sure they have at least n target positions
    Fish* current_fish = current_aquarium->poissons;
    while (current_fish != NULL) {
        if (current_fish->future_positions == NULL) {
            current_fish->future_positions = create_list();
        }

        if (current_fish->future_positions->size == 0) {
            add_n_fish_target_positions(current_fish, n);
        } else {
            fill_up_fish_positions_list(current_fish, n);
        }

        current_fish = current_fish->suivant;
    }

    // Loop through all fishes n times and get their target positions.
    for (int i = 0; i < n; i++) {
        char response[BUFFER_SIZE];
        strcpy(response, "list");

        current_fish = current_aquarium->poissons;
        while (current_fish != NULL) {
            FishNextPos* next_position = peek_at_index(current_fish->future_positions, i);
            if (next_position == NULL) {
                current_fish = current_fish->suivant;
                continue;
            }

            Tuple view_coords = get_view_coordinates(
                next_position->x,
                next_position->y,
                current_aquarium->afficheurs
            );

            int seconds_to_reach = (next_position->arrival_time - get_time_usec()) / 1000000;
            if (seconds_to_reach < 0) seconds_to_reach = 0;

            char fish_info[BUFFER_SIZE];
            snprintf(fish_info, BUFFER_SIZE, " [\"%s\" at %dx%d,%dx%d,%d]",
                current_fish->name,
                view_coords.x, view_coords.y,
                current_fish->w, current_fish->h,
                seconds_to_reach
            );
            strcat(response, fish_info);

            current_fish = current_fish->suivant;
        }
        
        // Send the response to the client
        strcat(response, "\n");
        send(job_socket, response, strlen(response), 0);
        log_msg("[ls] Sending '%s'\n", response);
    }
    
    pthread_mutex_unlock(&mutex_aquarium);

    gettimeofday(&end, NULL);
    long seconds = end.tv_sec - start.tv_sec;
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
    double elapsed = seconds * 1000.0 + micros / 1000.0;
    log_msg("[handle_ls] Execution time: %.3f ms\n", elapsed);
    return 0;
}

int handle_ping(int job_socket, const char* message) {
    // log_msg("Message reçu (ping) : %s\n", message);
    strncpy(buffer, message, BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';  // Sécurisation de la fin de chaîne

    char *key = strtok(buffer, " ");
    key = strtok(NULL, " ");

    if (aquarium_null_send(job_socket, "no greeting")) {
        return -1;
    }
    
    pthread_mutex_lock(&mutex_aquarium);

    // Find connected view and reset its last update time
    Afficheur* current_view = current_aquarium->afficheurs;
    while (current_view != NULL) {
        if (current_view->socket == job_socket) {
            current_view->last_update_time = get_time_usec();
            log_msg("[ping] View %s is still connected on socket %d\n", current_view->name, job_socket);
            break;  // Found the view
        }
        current_view = current_view->suivant;
    }
    
    pthread_mutex_unlock(&mutex_aquarium);

    char response[BUFFER_SIZE];  // Déclarer un buffer vide
    strcpy(response, "pong ");   // Copier "pong" au début
    strcat(response, key);       // Ajouter `key` à la fin
    strcat(response, "\n");
    send(job_socket, response, strlen(response), 0);
    return 0;
}


// Le client envoie "addFish <name> at <x>x<y>, <w>x<h>, <move_function>"
int handle_addFish(int job_socket, const char* message) {
    log_msg("Message reçu (addFish) : %s\n", message);
    
    char buffer[BUFFER_SIZE];
    strncpy(buffer, message, BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';  // Sécurisation de la fin de chaîne

    char *key = strtok(buffer, " ");

    key = strtok(NULL, " ");

    char name[MAX_NAME_LEN];
    strncpy(name, key, sizeof(name));  // Copier le nom du fish
    name[sizeof(name) - 1] = '\0';  // Sécuriser

    key = strtok(NULL, " ");  // Sauter un token inutile ("at")

    // Handle "<x>x<y>"
    int x = 0, y = 0, w = 0, h = 0;
    key = strtok(NULL, ", ");  // Aller à la partie dimensions
    if (key != NULL) {
        sscanf(key, "%dx%d", &x, &y);  // Lire les coordonnées
    } else {
        return wrong_msg_received_send_NOK(job_socket, key, "<x>x<y>", "No target coordinates provided. Did you mean 'addFish <name> at <x>x<y>, <w>x<h>, <move_function>'?");
    }

    // Handle "<w>x<h>"
    key = strtok(NULL, ", ");  // Aller à la partie dimensions
    if (key != NULL) {
        sscanf(key, "%dx%d", &w, &h);  // Lire les dimensions
    } else {
        return wrong_msg_received_send_NOK(job_socket, key, "<w>x<h>", "No dimensions provided. Did you mean 'addFish <name> at <x>x<y>, <w>x<h>, <move_function>'?");
    }

    key = strtok(NULL, " ");  // Aller au nom de la fonction de déplacement
    char move_function[MAX_NAME_LEN];
    if (key != NULL) {
        strncpy(move_function, key, sizeof(move_function));
        move_function[sizeof(move_function) - 1] = '\0';  // Sécurisation
    } else {
        strcpy(move_function, "RandomWayPoint");  // Valeur par défaut
    }

    pthread_mutex_lock(&mutex_aquarium);

    if (current_aquarium == NULL) {
        pthread_mutex_unlock(&mutex_aquarium);
        return wrong_msg_received_send_NOK(job_socket, message, "loading an aquarium", "No aquarium available in addFish");
    }

    char response[BUFFER_SIZE];
    if (add_fish(name, x, y, w, h, move_function)) {
        strcpy(response, "OK Fish added\n");
    } else {
        strcpy(response, "NOK Fish could not be added\n");
    }
    
    pthread_mutex_unlock(&mutex_aquarium);

    log_msg("[addFish] Response: %s", response);
    send(job_socket, response, strlen(response), 0);
    return 0;
}


int handle_delFish(int job_socket, const char* message) {
    log_msg("Message reçu (delFish) : %s\n", message);

    char buffer[BUFFER_SIZE];
    strncpy(buffer, message, BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';  // Sécurisation de la fin de chaîne

    char* tok = strtok(buffer, " ");  // delFish
    tok = strtok(NULL, " ");          // fish name

    if (tok == NULL) {
        return wrong_msg_received_send_NOK(job_socket, tok, "<fish name>", "No fish name provided in delFish");
    }
    
    pthread_mutex_lock(&mutex_aquarium);

    // Send "NOK" if no aquarium
    if (current_aquarium == NULL) {
        pthread_mutex_unlock(&mutex_aquarium);
        return wrong_msg_received_send_NOK(job_socket, message, "loading an aquarium", "No aquarium available in delFish");
    }
    
    if(release_fish(tok)) {
        pthread_mutex_unlock(&mutex_aquarium);
        // If fish is released, send "OK"
        char response[] = "OK Fish released\n";
        send(job_socket, response, strlen(response), 0);
        return 0;
    }

    // If no (fish not in aquarium), send "NOK"
    pthread_mutex_unlock(&mutex_aquarium);
    return wrong_msg_received_send_NOK(job_socket, tok, "<fish name>", "Fish not found in delFish");
}

// Handle "startFish <FishName>" command
int handle_startFish(int job_socket, const char* message) {
    log_msg("Message reçu (start fish) : %s\n", message);

    char buffer[BUFFER_SIZE];
    strncpy(buffer, message, BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';  // Sécurisation de la fin de chaîne

    char* tok = strtok(buffer, " ");  // startFish
    tok = strtok(NULL, " ");          // fish name
    if (tok == NULL) {
        return wrong_msg_received_send_NOK(job_socket, tok, "<fish name>", "No fish name provided in startFish");
    }
    
    pthread_mutex_lock(&mutex_aquarium);

    // If no aquarium, send "NOK"
    if (current_aquarium == NULL) {
        pthread_mutex_unlock(&mutex_aquarium);
        return wrong_msg_received_send_NOK(job_socket, message, "loading an aquarium", "No aquarium available in startFish");
    }
    
    // Check if the fish is in the aquarium
    Fish* current_fish = current_aquarium->poissons;
    while (current_fish != NULL) {
        if (strcmp(current_fish->name, tok) == 0) {
            break;  // Found the fish
        }
        current_fish = current_fish->suivant;
    }

    char response[BUFFER_SIZE];

    if (current_fish == NULL) {
        pthread_mutex_unlock(&mutex_aquarium);
        snprintf(response, BUFFER_SIZE, "[startFish] Fish %s not found in aquarium\n", tok);
        return send_NOK(job_socket, buffer);
    }

    // Check if the fish is already moving
    if (current_fish->started) {
        pthread_mutex_unlock(&mutex_aquarium);
        snprintf(response, BUFFER_SIZE, "OK [startFish] Fish %s is already moving\n", tok);
        send(job_socket, response, strlen(response), 0);
        return 0;
    }
    
    // Start the fish
    current_fish->started = true;
    
    pthread_mutex_unlock(&mutex_aquarium);
    snprintf(response, BUFFER_SIZE, "OK [startFish] Fish %s started\n", tok);
    send(job_socket, response, strlen(response), 0);
    return 0;
}

int handle_logOut(int job_socket, const char* message) {
    log_msg("Message reçu (logOut) : %s\n", message);
    
    pthread_mutex_lock(&mutex_aquarium);

    // Si aucun aquarium n'existe
    if (current_aquarium == NULL) {
        pthread_mutex_unlock(&mutex_aquarium);
        char response[] = "bye\n";
        send(job_socket, response, strlen(response), 0);
        return 0;
    }

    // Supprimer l'afficheur correspondant au socket
    Afficheur* current_view = current_aquarium->afficheurs;
    while (current_view != NULL) {
        if (current_view->socket == job_socket) {
            log_msg("[logOut] Déconnexion de la vue : %s\n", current_view->name);
            current_view->socket = -1;
            current_view->subscribed = false;  // Unsubscribe the view
            break;
        }
        current_view = current_view->suivant;
    }

    if (current_view == NULL) {
        log_msg("[logOut] Aucune vue trouvée pour le socket %d\n", job_socket);
    } else {
        log_msg("[logOut] Vue déconnectée : %s\n", current_view->name);
    }

    pthread_mutex_unlock(&mutex_aquarium);
    char response[] = "bye\n";
    send(job_socket, response, strlen(response), 0);
    return 0;
}


int handle_Unknown(int job_socket, const char* message) {
    log_msg("Message reçu (Unknown) : '%s'\n", message);
    
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "Commande inconnue : %s\n", message);
    send(job_socket, response, strlen(response), 0);
    
    return 0;
}


int first_word(int job_socket, char* message) {
    // strip message
    trim(message);
    log_msg("=====================================\n");

    if (strncmp(message, "hello", 5) == 0) 
        return handle_Hello(job_socket, message);
    else if (strncmp(message, "getFishesContinuously", 21) == 0) 
        return handle_Continuous(job_socket, message);
    else if (strncmp(message, "getFishes", 9) == 0) 
        return handle_getFishes(job_socket, message);
    else if (strncmp(message, "ls", 2) == 0)
        return handle_ls(job_socket, message);
    else if (strncmp(message, "ping ", 5) == 0) 
        return handle_ping(job_socket, message);
    else if (strncmp(message, "addFish ", 8) == 0) 
        return handle_addFish(job_socket, message);
    else if (strncmp(message, "delFish ", 8) == 0) 
        return handle_delFish(job_socket, message);
    else if (strncmp(message, "startFish ", 10) == 0) 
        return handle_startFish(job_socket, message);
    else if (strncmp(message, "log out", 7) == 0)
        return handle_logOut(job_socket, message);
    else
        return handle_Unknown(job_socket, message);
}

void handle_message(int job_socket, char* buffer) {
    first_word(job_socket, buffer);
}