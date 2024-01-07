#ifndef POOLE_SERVER_MANAGER_H
#define POOLE_SERVER_MANAGER_H

// Librerias
#include "../global.h"

// Variables globales importadas de otros TADS
extern char fecha[20];

void add_poole_server(char* server_name, char* ip_address, int port);

PooleServer* find_least_loaded_poole_server();

void delete_poole_from_list(char* server_name);

#endif 