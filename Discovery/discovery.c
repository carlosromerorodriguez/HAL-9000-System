#include "config.h"

// Definiciones de constantes
#define BACKLOG 128
#define DATA_MAX_SIZE 253

// Estructura para almacenar la configuración del servidor
typedef struct PooleServer {
    char *server_name;       // Nombre del servidor Poole
    char *ip_address;        // Dirección IP del servidor Poole
    int port;                // Puerto del servidor Poole
    int connected_users;     // Número de usuarios actualmente conectados a este servidor Poole
    char **usernames;        // Lista de nombres de usuario conectados a este servidor Poole
    struct PooleServer* next;// Puntero al siguiente servidor Poole en la lista
} PooleServer;

// Variables globales
PooleServer* poole_servers_head = NULL; // Cabeza de la lista enlazada de servidores Poole

DiscoveryConfig *discoverConfig;
int poole_fd, bowman_fd;
pthread_t poole_thread_id, bowman_thread_id;
volatile sig_atomic_t poole_thread_active = 0;
volatile sig_atomic_t bowman_thread_active = 0;
int poole_thread_created = 0;
int bowman_thread_created = 0;

char fecha[20];

// Prototipos de funciones
void* handlePooleConnection(void* arg);
void* handleBowmanConnection(void* arg);
void add_poole_server(char* server_name, char* ip_address, int port);
PooleServer* find_least_loaded_poole_server();
void logout_user( char* data, int bowman_socket);
void free_all_dynamic_memory(void);
void kctrlc(int signum);


/**
 * @brief Función principal del programa
 * 
 * @param argc Número de argumentos de la línea de comandos
 * @param argv Argumentos de la línea de comandos
 * 
 * @return int 0 si todo ha ido bien, 1 si ha habido algún error
*/
int main(int argc, char* argv[]) {

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", tm);

    check_input_arguments(argc, 2);

    signal(SIGINT, kctrlc); //Activar signal Ctrl+C

    discoverConfig = (DiscoveryConfig *) malloc (sizeof(DiscoveryConfig));

    if (readDiscoveryConfig(argv[1], discoverConfig) == 0) {
        printF(RED, "Error reading discovery file\n");
        exit(EXIT_FAILURE);
    }

    poole_fd = socket(AF_INET, SOCK_STREAM, 0);
    bowman_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (poole_fd < 0 || bowman_fd < 0) {
        printF(RED, "Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    // Enlazar y escuchar en las direcciones de Poole y Bowman
    if (bind(poole_fd, (struct sockaddr *)&discoverConfig->poole_addr, sizeof(discoverConfig->poole_addr)) < 0 ||
        bind(bowman_fd, (struct sockaddr *)&discoverConfig->bowman_addr, sizeof(discoverConfig->bowman_addr)) < 0) {
        printF(RED, "Bind failed\n");
        free_all_dynamic_memory();
        exit(EXIT_FAILURE);
    }

    listen(poole_fd, BACKLOG);
    listen(bowman_fd, BACKLOG);

    // Configurar los sockets para que sean no bloqueantes
    fcntl(poole_fd, F_SETFL, O_NONBLOCK);
    fcntl(bowman_fd, F_SETFL, O_NONBLOCK);

    // Aceptar las conexiones y crear hilos para manejar cada una de ellas
    printF(YELLOW, "%s -> ", fecha);
    printF(GREEN, "Discovery server initiated\n");
    printF(YELLOW, "Listening for incoming connections...\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int* client_fd_ptr;

        // Aceptar conexiones de Poole
        int client_fd = accept(poole_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd >= 0) {
            client_fd_ptr = malloc(sizeof(int));
            *client_fd_ptr = client_fd;
            if (pthread_create(&poole_thread_id, NULL, handlePooleConnection, client_fd_ptr) == 0) {
                poole_thread_created = 1;
            }
        }

        // Aceptar conexiones de Bowman
        client_fd = accept(bowman_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd >= 0) {
            client_fd_ptr = malloc(sizeof(int));
            *client_fd_ptr = client_fd;
            if (pthread_create(&bowman_thread_id, NULL, handleBowmanConnection, client_fd_ptr) == 0) {
                bowman_thread_created = 1;
            }
        }

        usleep(10000); // Esperar 10ms (pequeño retardo para reducir el uso de CPU)
    }

    free_all_dynamic_memory();

    return EXIT_SUCCESS;
}

/**
 * @brief Función que maneja las conexiones entrantes de Poole
 * 
 * @param arg Puntero al socket de Poole
 * 
 * @return void
*/
void* handlePooleConnection(void* arg) {
    int poole_socket = *(int*)arg;
    free(arg);

    if (poole_thread_active) {
        //Liberar toda la memoria dinámica o recursos que se hayan podido reservar
        close(poole_socket);
        pthread_exit(NULL);
    }

    Frame request_frame;
    if (receive_frame(poole_socket, &request_frame) <= 0) {
        printF(RED, "Error receiving frame from Poole.\n");
        close(poole_socket);
        pthread_exit(NULL);
    }

    // printF(YELLOW, "Received frame: %s\n", request_frame.header_plus_data);

    // Verificar si la trama es NEW_POOLE
    if (strncmp(request_frame.header_plus_data, "NEW_POOLE", request_frame.header_length) == 0) {

        // Extraer el nombre, IP y puerto del servidor Poole de la trama
        char *data = request_frame.header_plus_data + request_frame.header_length;
        char *token = strtok(data, "&");
        char *server_name = token != NULL ? strdup(token) : NULL;
        token = strtok(NULL, "&");
        char *ip_address = token != NULL ? strdup(token) : NULL;
        token = strtok(NULL, "&");
        int port = token != NULL ? atoi(token) : -1;

        if (server_name != NULL && ip_address != NULL && port != -1) {
            // Añadir el servidor Poole a la lista
            add_poole_server(server_name, ip_address, port);
            free(server_name);
            free(ip_address);

            // Enviar trama CON_OK a Poole
            Frame response_frame = frame_creator(0x01, "CON_OK", "");
            send_frame(poole_socket, &response_frame);
        } else {
            // Enviar trama CON_KO a Poole si hay algún error
            Frame response_frame = frame_creator(0x01, "CON_KO", "");
            send_frame(poole_socket, &response_frame);
        }
    } else {
        printF(RED, "Received unexpected frame header from Poole.\n");
    }

    printF(YELLOW, "%s ->", fecha);
    printF(GREEN, " New connection from Poole\n");

    close(poole_socket);
    pthread_exit(NULL);
}

/**
 * @brief Función que maneja las conexiones entrantes de Bowman
 * 
 * @param arg Puntero al socket de Bowman
 * 
 * @return void
*/
void* handleBowmanConnection(void* arg) {
    int bowman_socket = *(int*)arg;
    free(arg);

    if (bowman_thread_active) {
        //Liberar toda la memoria dinámica o recursos que se hayan podido reservar
        close(bowman_socket);
        pthread_exit(NULL);
    }

    Frame request_frame;
    if (receive_frame(bowman_socket, &request_frame) <= 0) {
        printF(RED, "Error receiving frame from Bowman. Connection aborted.\n");
        close(bowman_socket);
        pthread_exit(NULL);
    }

    // Verificar si la trama es NEW_BOWMAN
    if (strncmp(request_frame.header_plus_data, "NEW_BOWMAN", request_frame.header_length) == 0) {
        PooleServer* least_loaded_poole = find_least_loaded_poole_server();
        Frame response_frame;

        if (least_loaded_poole != NULL) {
            least_loaded_poole->usernames = realloc(least_loaded_poole->usernames, (least_loaded_poole->connected_users + 1) * sizeof(char*));
            least_loaded_poole->usernames[least_loaded_poole->connected_users] = strdup(request_frame.header_plus_data + request_frame.header_length);
            if (least_loaded_poole->usernames[least_loaded_poole->connected_users] == NULL) {
                printF(RED, "Error allocating memory for username.\n");
                close(bowman_socket);
                pthread_exit(NULL);
            }

            // Incrementar el contador de usuarios conectados
            least_loaded_poole->connected_users++;

            printF(YELLOW, "%s -> ", fecha);
            printF(GREEN, "New connection from Bowman: %s\n", least_loaded_poole->usernames[least_loaded_poole->connected_users - 1]);

            char data[HEADER_MAX_SIZE];
            snprintf(data, sizeof(data), "%s&%s&%d", least_loaded_poole->server_name, least_loaded_poole->ip_address, least_loaded_poole->port);
            response_frame = frame_creator(0x01, "CON_OK", data);
        } else {    
            printF(RED, "No Poole servers available. Sending CON_KO to Bowman.\n");
            response_frame = frame_creator(0x01, "CON_KO", "");
        }

        // Enviar la trama de respuesta a Bowman
        if (send_frame(bowman_socket, &response_frame) < 0) {
            printF(RED, "Error sending response frame to Bowman.\n");
        }
    } else if (strncmp(request_frame.header_plus_data, "EXIT", request_frame.header_length) == 0) {
        logout_user(request_frame.header_plus_data, bowman_socket);
    } else {
        printF(RED, "Received an unexpected frame header from Bowman: %s\n", request_frame.header_plus_data);
    }

    close(bowman_socket);
    pthread_exit(NULL);
}

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

/**
 * @brief Libera toda la memoria dinámica reservada en el programa
 * 
 * @return void
*/
void free_all_dynamic_memory(void) {
    if (discoverConfig) {
        free(discoverConfig);
        discoverConfig = NULL;
    } if (poole_fd) {
        close(poole_fd);
    } if (bowman_fd) {
        close(bowman_fd);
    } if (poole_servers_head) {
        PooleServer* current = poole_servers_head;
        while (current != NULL) {
            PooleServer* next = current->next;
            free(current->server_name);
            free(current->ip_address);
            for (int i = 0; i < current->connected_users; i++) {
                free(current->usernames[i]);
            }
            free(current->usernames);
            free(current);
            current = next;
        }
        poole_servers_head = NULL;
    }
}

/**
 * @brief Función que se ejecuta cuando se pulsa Ctrl+C
 * 
 * @param signum Señal que se ha recibido
 * @return void
*/
void kctrlc(int signum) {
    if (signum == SIGINT) {
        poole_thread_active = 1;
        bowman_thread_active = 1;

        // Esperar a que los hilos terminen si fueron creados
        if (poole_thread_created) {
            pthread_join(poole_thread_id, NULL);
        }
        if (bowman_thread_created) {
            pthread_join(bowman_thread_id, NULL);
        }

        free_all_dynamic_memory();
    }
    exit(EXIT_SUCCESS);
}