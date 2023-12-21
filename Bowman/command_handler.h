#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "../global.h"

typedef struct {
    int *poole_socket;
    char *username;
    char *discovery_ip;
    int discovery_port;
} thread_receive_frames;

typedef struct {
    char *fileName;
    long fileSize;
    char *md5sum;
    int id;
} Song;

typedef struct {
    Song song;
    long downloaded_bytes;
    long song_size;
    pthread_t thread_id;
} Song_Downloading;

/**
 * @brief Handle the command received from the user
 * 
 * @param command Command to handle
 * @param connected Flag to know if the user wants to logout or not
*/
unsigned char handle_bowman_command(char *command, unsigned char *connected, int *discovery_socket, char *username, char *discovery_ip, int discovery_port, int *poole_socket);

/**
 * @brief Crea una trama EXIT y la envía al servidor Poole
 * 
 * @param username Nombre de usuario
 * @param discovery_ip Dirección IP del servidor Discovery
 * @param discovery_port Puerto del servidor Discovery
 * 
 * @return EXIT_SUCCESS si el logout es exitoso, EXIT_FAILURE en caso contrario
*/
int logout(char *username);

void freeSongDownloadingArray();

#endif