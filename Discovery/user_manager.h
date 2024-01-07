#ifndef USER_MANAGER_H
#define USER_MANAGER_H

// Librerias
#include "../global.h"

// Variables globales importadas de otros TADS
extern PooleServer* poole_servers_head;
extern char fecha[20];

/**
 * @brief Función para manejar la petición de logout de un usuario
 * 
 * @param data Datos de la trama recibida
 * @param bowman_socket Socket de Bowman
 * 
 * @return void
*/
void logout_user( char* data, int bowman_socket);

#endif 