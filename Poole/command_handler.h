#ifndef POOLE_COMMAND_HANDLER_H
#define POOLE_COMMAND_HANDLER_H

#include "../global.h"

// Estructura para pasar los argumentos al thread
typedef struct {
    int client_socket;
    char* username;
    char* server_directory;
    pthread_t list_songs_thread;
    pthread_t list_playlists_thread;
} thread_args;

/**
 * @brief Maneja la conexion con Bowman y el envio de comandos
 * 
 * @param args Estructura con los argumentos necesarios para la conexion
 * 
 * @return NULL
*/
void *client_handler(void *socket);

#endif