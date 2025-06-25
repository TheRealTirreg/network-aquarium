#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "log.h"
#include "handle_client.h"
#include "cli.h"
#include "aquarium.h"
#include "read_cfg.h"

#define MAX_JOBS 100
#define NB_THREADS 10
#define MAX_CLIENTS 10
#define FISH_UPDATE_INTERVAL 10000  // en microseconds, 10ms atm

int jobs_socket[MAX_JOBS];
int index_current_job = 0;

pthread_mutex_t mutex_jobs = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_jobs = PTHREAD_COND_INITIALIZER;

void enqueue_job(int socket)
{
    pthread_mutex_lock(&mutex_jobs);
    while (index_current_job >= MAX_JOBS)
    {
        pthread_cond_wait(&cond_jobs, &mutex_jobs);
    }
    jobs_socket[index_current_job++] = socket;
    pthread_cond_signal(&cond_jobs);
    pthread_mutex_unlock(&mutex_jobs);
}

int dequeue_job()
{
    pthread_mutex_lock(&mutex_jobs);
    while (index_current_job <= 0)
    {
        pthread_cond_wait(&cond_jobs, &mutex_jobs);
    }
    int job_socket = jobs_socket[--index_current_job];
    pthread_cond_signal(&cond_jobs);
    pthread_mutex_unlock(&mutex_jobs);
    return job_socket;
}

void *prompt_thread(void *arg){
    usleep(10000); // Wait 10ms to let the aquarium load
    log_msg("[WARNING] Load an aquarium before connecting clients!\n");
    CliContext* ctx = (CliContext*)arg;
    cli(ctx->input_win, ctx->output_win);
    return NULL;
}

void *getFishesContinuously_thread(void *arg)
{
    while (1)
    {
        // Call update_fishes() every FISH_UPDATE_INTERVAL micro seconds
        usleep(FISH_UPDATE_INTERVAL);
        
        pthread_mutex_lock(&mutex_aquarium);
        update_fishes();
        disconnect_views();
        pthread_mutex_unlock(&mutex_aquarium);
    }
    return NULL;
}

void *fct_thread(void *arg)
{
    while (1)
    {
        int job_socket = dequeue_job(); // Récupérer un socket client
        if (job_socket < 0) {
            log_msg("Erreur: job_socket < 0\n");
            continue; // Sécurité : éviter les erreurs si aucun job disponible
        }

        char buffer[BUFFER_SIZE];
        
        int bytesRead = read(job_socket, buffer, BUFFER_SIZE - 1);
        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';  // Assurer la fin de chaîne
            
            if (strncmp(buffer, "log", 3) == 0 && strlen(buffer) == 4)
            {
                char reponse[4] = "bye";
                log_msg("Fermeture du socket client.\n");
                send(job_socket, reponse, strlen(reponse), 0);
                close(job_socket);  // Fermer proprement la connexion
            }
            else
            {
                handle_message(job_socket, buffer);
                
                // 🔄 Remettre le job dans la file pour qu'un autre thread le prenne
                enqueue_job(job_socket);
            }
        }
        else
        {
            log_msg("Client déconnecté.\n");
            close(job_socket);  // Fermer si le client coupe la connexion
        }
    }
    return NULL;
}

CliContext* init_ncurses()
{
    initscr();
    cbreak();
    keypad(stdscr, TRUE);

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    CliContext* ctx = malloc(sizeof(CliContext));
    if (!ctx) {
        fprintf(stderr, "Erreur d'allocation mémoire pour le contexte CLI.\n");
        exit(EXIT_FAILURE);
    }

    ctx->output_win = newwin(max_y - 1, max_x, 0, 0);
    scrollok(ctx->output_win, TRUE);

    ctx->input_win = newwin(1, max_x, max_y - 1, 0);
    wattron(ctx->input_win, A_BOLD);
    mvwprintw(ctx->input_win, 0, 0, "Entrez une commande > ");
    wattroff(ctx->input_win, A_BOLD);
    wrefresh(ctx->input_win);

    init_logger(ctx->output_win);  // shared output window for log_msg()

    return ctx;
}

int main()
{
    // Initialisation de ncurses
    CliContext* cli_ctx = init_ncurses();

    // Lire config
    if (!read_cfg("controller.cfg"))
    {
        log_msg("Erreur de lecture du fichier de configuration.\n");
        return EXIT_FAILURE;
    }

    // Création du socket
    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }

    // Définition de l'adresse du serveur
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(CONTROLLER_PORT);

    // Options du socket (réutilisation de l'adresse)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind le socket à l'adresse
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Erreur bind");
        exit(EXIT_FAILURE);
    }

    // Écoute
    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("Erreur listen");
        exit(EXIT_FAILURE);
    }

    // Création des threads
    pthread_t threads[NB_THREADS];
    for (int i = 0; i < NB_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, fct_thread, NULL);
    }

    // Création du thread pour le prompt
    pthread_t prompt;
    pthread_create(&prompt, NULL, prompt_thread, (void*)cli_ctx);

    // Création du thread pour le getFishesContinuously
    pthread_t getFishesContinuously;
    pthread_create(&getFishesContinuously, NULL, getFishesContinuously_thread, NULL);

    // Accepter les connexions
    log_msg("[INFO] Serveur en attente de connexions sur le port %d...\n", CONTROLLER_PORT);
    while (1)
    {
        // Accepter un client
        int new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0)
        {
            perror("Erreur accept");
            exit(EXIT_FAILURE);
        }
        log_msg("Nouvelle connexion acceptée\n");
        // Ajouter le job à la file

        enqueue_job(new_socket);
    }

    return 0;
}
