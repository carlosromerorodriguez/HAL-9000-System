#ifndef SESSION_HANDLER_H
#define SESSION_HANDLER_H

#include "../global.h"
#include "utilities.h"
#include "connection_handler.h"
#include "download_manager.h"
#include "display_response_handler.h"

/**
 * @brief Handle the command received from the user
 * 
 * @param command Command to handle
 * @param connected Flag to know if the user wants to logout or not
 * @param discovery_socket Socket de conexión con el servidor Discovery
 * @param username Nombre de usuario
 * @param discovery_ip Dirección IP del servidor Discovery
 * @param discovery_port Puerto del servidor Discovery
 * @param psocket Socket de conexión con el servidor Poole
 * @param folder_path Ruta de la carpeta de descargas
*/
unsigned char handle_bowman_command(char *command, unsigned char *connected, int *discovery_socket, char *username, char *discovery_ip, int discovery_port, int *psocket, char* folder_path); 

/**
 * @brief Crea una trama EXIT y la envía al servidor Poole
 * 
 * @param username Nombre de usuario
 * 
 * @return EXIT_SUCCESS si el logout es exitoso, EXIT_FAILURE en caso contrario
*/
int logout(char *username);

#endif