#ifndef POOLE_SERVER_MANAGER_H
#define POOLE_SERVER_MANAGER_H

// Librerias
#include "../global.h"

// Variables globales importadas de otros TADS
extern char fecha[20];

/**
 * @brief Función para añadir un servidor Poole a la lista enlazada
 * 
 * @param server_name Nombre del servidor Poole
 * @param ip_address Dirección IP del servidor Poole
 * @param port Puerto del servidor Poole
 * 
 * @return void
*/
void add_poole_server(char* server_name, char* ip_address, int port);

/**
 * @brief Función para encontrar el servidor Poole con menos usuarios conectados
 * 
 * @return Puntero al servidor Poole con menos usuarios conectados
*/
PooleServer* find_least_loaded_poole_server();

/**
 * @brief Función para eliminar un servidor Poole de la lista de servidores disponibles
 * 
 * @param data Nombre del servidor Poole
 * 
 * @return void
*/
void delete_poole_from_list(char* server_name);

#endif 