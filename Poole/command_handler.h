#ifndef POOLE_COMMAND_HANDLER_H
#define POOLE_COMMAND_HANDLER_H

#include "../global.h"

#define MAX_SONGS_THREADS 20

// Estructura para pasar los argumentos al thread
typedef struct {
    int client_socket;
    int is_song;
    char *playlist_name;
    char* username;
    char* server_directory;
    char* song_name;
    char* list_name;
    pthread_t list_songs_thread;
    pthread_t list_playlists_thread;
    pthread_t download_song_thread;
    pthread_t download_playlist_thread;
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