#ifndef SESSION_HANDLER_H
#define SESSION_HANDLER_H

#include "../global.h"
#include "utilities.h"
#include "connection_handler.h"
#include "download_manager.h"
#include "display_response_handler.h"

int logout(char *username);

unsigned char handle_bowman_command(char *command, unsigned char *connected, int *discovery_socket, char *username, char *discovery_ip, int discovery_port, int *psocket, char* folder_path); 

#endif