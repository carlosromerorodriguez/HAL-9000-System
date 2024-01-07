#include "poole_server_manager.h"

PooleServer* poole_servers_head = NULL; // Cabeza de la lista enlazada de servidores Poole

/**
 * @brief Función para añadir un servidor Poole a la lista enlazada
 * 
 * @param server_name Nombre del servidor Poole
 * @param ip_address Dirección IP del servidor Poole
 * @param port Puerto del servidor Poole
 * 
 * @return void
*/
void add_poole_server(char* server_name, char* ip_address, int port) {
    PooleServer* new_server = (PooleServer*)malloc(sizeof(PooleServer));
    new_server->server_name = malloc(strlen(server_name) + 1); // Asignar memoria para el nombre del servidor
    strcpy(new_server->server_name, server_name);
    new_server->server_name[strlen(server_name)] = '\0';

    new_server->ip_address = malloc(strlen(ip_address) + 1);   // Asignar memoria para la dirección IP
    strcpy(new_server->ip_address, ip_address);
    new_server->ip_address[strlen(ip_address)] = '\0';
    
    new_server->port = port;                                   // Asignar el puerto
    new_server->connected_users = 0;                           // Inicializar usuarios conectados
    new_server->usernames = NULL;                              // Inicializar lista de nombres de usuario

    new_server->next = poole_servers_head;                     // Insertar al inicio de la lista
    poole_servers_head = new_server;
}

/**
 * @brief Función para encontrar el servidor Poole con menos usuarios conectados
 * 
 * @return Puntero al servidor Poole con menos usuarios conectados
*/
PooleServer* find_least_loaded_poole_server() {
    PooleServer* current = poole_servers_head;
    PooleServer* least_loaded = NULL;
    int min_users = INT_MAX;

    while (current != NULL) {
        if (current->connected_users < min_users) {
            min_users = current->connected_users;
            least_loaded = current;
        }
        current = current->next;
    }

    return least_loaded;
}


/**
 * @brief Función para eliminar un servidor Poole de la lista de servidores disponibles
 * 
 * @param data Nombre del servidor Poole
 * 
 * @return void
*/
void delete_poole_from_list(char* data) {

    char *server_name = strtok(data, "&");

    PooleServer* current = poole_servers_head;
    PooleServer* prev = NULL;

    while (current != NULL) {
        if (strcmp(current->server_name, server_name) == 0) {
            if (prev == NULL) {
                poole_servers_head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current->server_name);
            free(current->ip_address);
            for (int i = 0; i < current->connected_users; i++) {
                free(current->usernames[i]);
            }
            free(current->usernames);
            free(current);
            break;
        }
        prev = current;
        current = current->next;
    }

    //lista de pooles printada:

    current = poole_servers_head;

    printF(YELLOW, "%s -> ", fecha);
    printF(RED, "%s has been deleted from the list of Poole servers\n", server_name);

    printF(GREEN, "Servers available:\n");
    while (current != NULL) {
        printF(GREEN, "    %s\n", current->server_name);
        current = current->next;
    }
}