#ifndef UTILITIES_2_H
#define UTILITIES_2_H

// Librerias
#include "../global.h"

// Variables globales importadas de otros TADS
extern DiscoveryConfig *discoverConfig;
extern int poole_fd, bowman_fd;
extern pthread_t poole_thread_id, bowman_thread_id;
extern volatile sig_atomic_t poole_thread_active;
extern volatile sig_atomic_t bowman_thread_active;
extern int poole_thread_created;
extern int bowman_thread_created;
extern PooleServer* poole_servers_head;

/**
 * @brief Libera toda la memoria dinámica reservada en el programa
 * 
 * @return void
*/
void free_all_dynamic_memory(void);

/**
 * @brief Función que se ejecuta cuando se pulsa Ctrl+C
 * 
 * @param signum Señal que se ha recibido
 * @return void
*/
void kctrlc(int signum);

#endif // !UTILITIES_2_H





