#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include "../global.h"
#include "utilities.h"

void connect_to_server(int *psocket, int global_server_port, char *global_server_ip, int poole_socket);

int connect_to_discovery(char *username, char *discovery_ip, int discovery_port);

int disconnect_notification_to_discovery(char *username, char *discovery_ip, int discovery_port, int global_server_port, char *global_server_ip);

int connect_to_another_server(char *username, char *discovery_ip, int port, int *psocket, int discovery_socket, int poole_socket, int msg_id, char* global_server_name, int global_server_port, char *global_server_ip);

int connect_to_poole_server();

#endif