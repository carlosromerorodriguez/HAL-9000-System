#ifndef USER_MANAGER_H
#define USER_MANAGER_H

// Librerias
#include "../global.h"

// Variables globales importadas de otros TADS
extern PooleServer* poole_servers_head;
extern char fecha[20];

void logout_user( char* data, int bowman_socket);

#endif 