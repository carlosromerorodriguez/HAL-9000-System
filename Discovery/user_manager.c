#include "user_manager.h"

/**
 * @brief Función para manejar la petición de logout de un usuario
 * 
 * @param data Datos de la trama recibida
 * @param bowman_socket Socket de Bowman
 * 
 * @return void
*/
void logout_user(char* data, int bowman_socket) {
    data += 4; // Saltar "EXIT"

    char *username = strtok(data, "&");
    char *ip = strtok(NULL, "&");
    char *port_str = strtok(NULL, "&");

    if (username == NULL || ip == NULL || port_str == NULL) {
        printF(RED, "Error parsing logout data.\n");
    }

    int port = atoi(port_str);
    int user_found = 0;

    PooleServer* current = poole_servers_head;
    while (current != NULL) {
        if (strcmp(current->ip_address, ip) == 0 && current->port == port) {
            for (int i = 0; i < current->connected_users; i++) {
                if (strcmp(current->usernames[i], username) == 0) {
                    free(current->usernames[i]);
                    printF(YELLOW, "%s -> ", fecha);
                    printF(YELLOW, "User %s logged out\n", username);
                    for (int j = i; j < current->connected_users - 1; j++) {
                        current->usernames[j] = current->usernames[j + 1];
                    }
                    current->connected_users--;
                    current->usernames[current->connected_users] = NULL;
                    user_found = 1;
                    break;
                }
            }
            break;
        }
        current = current->next;
    }

    Frame disconnect_frame;
    if (user_found){
        disconnect_frame = frame_creator(0x06, "CONOK", "");
    }else{
        disconnect_frame = frame_creator(0x06, "CONKO", "");
    }

    send_frame(bowman_socket, &disconnect_frame);

}