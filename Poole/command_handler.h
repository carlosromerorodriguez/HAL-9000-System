#ifndef POOLE_COMMAND_HANDLER_H
#define POOLE_COMMAND_HANDLER_H

#include "../global.h"

/**
 * @brief Maneja la conexion con Bowman y el envio de comandos
 * 
 * @param args Estructura con los argumentos necesarios para la conexion
 * 
 * @return NULL
*/
void *client_handler(void *socket);

#endif