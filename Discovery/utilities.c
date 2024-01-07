#include "utilities.h"

/**
 * @brief Libera toda la memoria dinámica reservada en el programa
 * 
 * @return void
*/
void free_all_dynamic_memory(void) {
    if (discoverConfig) {
        free(discoverConfig);
        discoverConfig = NULL;
    } if (poole_fd) {
        close(poole_fd);
    } if (bowman_fd) {
        close(bowman_fd);
    } if (poole_servers_head) {
        PooleServer* current = poole_servers_head;
        while (current != NULL) {
            PooleServer* next = current->next;
            free(current->server_name);
            free(current->ip_address);
            for (int i = 0; i < current->connected_users; i++) {
                free(current->usernames[i]);
            }
            free(current->usernames);
            free(current);
            current = next;
        }
        poole_servers_head = NULL;
    }
}



/**
 * @brief Función que se ejecuta cuando se pulsa Ctrl+C
 * 
 * @param signum Señal que se ha recibido
 * @return void
*/
void kctrlc(int signum) {
    if (signum == SIGINT) {
        poole_thread_active = 1;
        bowman_thread_active = 1;

        // Esperar a que los hilos terminen si fueron creados
        if (poole_thread_created) {
            pthread_join(poole_thread_id, NULL);
        }
        if (bowman_thread_created) {
            pthread_join(bowman_thread_id, NULL);
        }

        free_all_dynamic_memory();
    }
    exit(EXIT_SUCCESS);
}