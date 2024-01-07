#include "../global.h"
#include "config.h"
#include "command_handler.h"
#include "monolit.h"

#define DATA_MAX_SIZE 253

/*
    Variables globales necesárias (pata liberar en caso de Ctrl+C)
*/
PooleConfig *poole_config; // configuración de Poole
int listen_socket;
char* username;
thread_args *args;
volatile sig_atomic_t server_sigint_received = 0;
pthread_t *thread_ids;
int num_threads = 0;
int global_write_pipe;
volatile sig_atomic_t poole_sigint_received = 0;
extern pthread_mutex_t send_frame_mutex;

/*
    Delaración de funciones
*/
void kctrlc(int signum);
void free_all_dynamic_memory(void);
void connect_to_discovery(const PooleConfig *config);
int setup_listen_socket(int port);
void startMonolitServer(void);
int disconnect_notification_to_discovery(const PooleConfig *config);

/**
 * @brief Función principal del programa Poole
 * 
 * @param argc Cantidad de argumentos recibidos
 * @param argv Arreglo de argumentos recibidos
 * 
 * @return EXIT_SUCCESS si el programa termina correctamente, EXIT_FAILURE en caso contrario
*/
int main(int argc, char** argv) {

    check_input_arguments(argc, 2);

    signal(SIGINT, kctrlc); //Activar signal Ctrl+C

    poole_config = (PooleConfig *) malloc (sizeof(PooleConfig));

    printF(YELLOW, "Reading configuration file...\n");
    display_loading_spinner(YELLOW, 3);

    if (fill_poole_config(poole_config, argv[1]) == 0) {
        printF(RED, "ERROR: Server connection unreachable. Please, check the server's filename and try again!\n");
        exit(EXIT_FAILURE);
    }

    printF(YELLOW, "Connecting %s Server to the system...\n", poole_config->username);

    connect_to_discovery(poole_config);    

    printF(WHITE, "Waiting for connections...\n");

    startMonolitServer();

    listen_socket = setup_listen_socket(poole_config->poole_port);
    if (listen_socket == EXIT_FAILURE) {
        free_all_dynamic_memory();
        exit(EXIT_FAILURE);
    }

    // Crear un arreglo de pthread_t para almacenar los IDs de los hilos
    thread_ids = malloc(num_threads * sizeof(pthread_t));
    if (thread_ids == NULL) {
        printF(RED, "Error allocating memory for thread IDs\n");
        free_all_dynamic_memory();
        exit(EXIT_FAILURE);
    }

    fd_set read_fds;
    while (!server_sigint_received) {
        FD_ZERO(&read_fds);
        FD_SET(listen_socket, &read_fds);

        struct timeval tv;
        tv.tv_sec = 1;  // 1 second
        tv.tv_usec = 0; // 0 microseconds

        if (select(listen_socket + 1, &read_fds, NULL, NULL, &tv) < 0) {
            printF(RED, "Select error\n");
            continue;
        }

        if (FD_ISSET(listen_socket, &read_fds)) {
            
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &addr_len);

            if (client_socket < 0) {
                printF(RED, "Error accepting connection\n");
                continue;
            }

            Frame response_frame;
            if (receive_frame(client_socket, &response_frame) <= 0) {
                response_frame = frame_creator(0x01, "CON_KO", "");
                printF(RED, "Error receiving frame from Bowman. Connection aborted.\n");
                continue;
            }

            username = strdup(response_frame.header_plus_data + response_frame.header_length);

            response_frame = frame_creator(0x01, "CON_OK", "");

            if (send_frame(client_socket, &response_frame) < 0) {
                printF(RED, "Error sending response frame to Bowman.\n");
            }

            printF(GREEN, "\nNew connection from %s\n", username);

            args = (thread_args *) malloc (sizeof(thread_args)); 

            args->client_socket = client_socket;

            args->username = strdup(username);

            args->server_directory = strdup(poole_config->folder_path);

            // Crear un nuevo hilo para manejar la conexión del cliente
            thread_ids = realloc(thread_ids, (num_threads + 1) * sizeof(pthread_t));
            if (thread_ids == NULL) {
                printF(RED, "Error allocating memory for thread IDs\n");
                free_all_dynamic_memory();
                exit(EXIT_FAILURE);
            }
            pthread_create(&thread_ids[num_threads], NULL, client_handler, (void *)args);
            num_threads++;
        }
    }

    close(listen_socket);
    free_all_dynamic_memory();
    
    return (EXIT_SUCCESS);
}

void startMonolitServer(void) {
    int fd[2];
    if (pipe(fd) == -1) {
        printF(RED, "ERROR: Error creating pipe\n");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == -1) {
        printF(RED, "ERROR: Error creating fork\n");
        close(fd[0]);
        close(fd[1]);
        exit(1);
    } else if (pid == 0) {
        printF(GREEN, "Starting Monolit Server...\n");
        close(fd[1]); // Cerrar el extremo de escritura del pipe
        throwMonolitServer(fd[0]); // Llamar a la función del Monòlit
    } else {
        // Proceso padre (Poole)
        close(fd[0]); // Cerrar el extremo de lectura del pipe
        global_write_pipe = fd[1]; // Guardar fd[1] en una variable global para pasarlo a las funciones que lo necesiten
    }
}

/**
 * @brief Libera toda la memoria dinámica reservada en el programa
 * 
 * @return void
*/
void free_all_dynamic_memory(void) {

    // Wait for threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(thread_ids[i], NULL);
    } if (thread_ids != NULL) {
        free(thread_ids);
        thread_ids = NULL;
    }

    //pthread_join(args->list_songs_thread, NULL);
    //pthread_join(args->list_playlists_thread, NULL);

    if (poole_config != NULL) {
        if (poole_config->username != NULL) {
            free(poole_config->username);
            poole_config->username = NULL;
        }

        if (poole_config->folder_path != NULL) {
            free(poole_config->folder_path);
            poole_config->folder_path = NULL;
        }

        if (poole_config->poole_ip != NULL) {
            free(poole_config->poole_ip);
            poole_config->poole_ip = NULL;
        }

        if (poole_config->discovery_ip != NULL) {
            free(poole_config->discovery_ip);
            poole_config->discovery_ip = NULL;
        }

        if (poole_config->discovery_port >= 0) {
            poole_config->discovery_port = -1;
        }

        free(poole_config);
        poole_config = NULL;
    }
    
    if (username != NULL) {
        free(username);
        username = NULL;
    }
    
    if (listen_socket >= 0) {
        close(listen_socket);
        listen_socket = -1;
    }
    
    if (args != NULL) {
        if (args->username != NULL) {
            free(args->username);
            args->username = NULL;
        }

        if (args->server_directory != NULL) {
            free(args->server_directory);
            args->server_directory = NULL;
        }

        free(args);
        args = NULL;
    }
}

/**
 * @brief Establece una conexión con el servidor Discovery
 * 
 * @param config Configuración de Poole que contiene la IP y el puerto del servidor Discovery
 * @return int Descriptor del socket si la conexión es exitosa, EXIT_FAILURE en caso de error
*/
void connect_to_discovery(const PooleConfig *config) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printF(RED, "Socket creation failed\n");
        return;
    }

    struct sockaddr_in discovery_addr;
    memset(&discovery_addr, 0, sizeof(discovery_addr));
    discovery_addr.sin_family = AF_INET;
    discovery_addr.sin_port = htons(config->discovery_port);
    inet_pton(AF_INET, config->discovery_ip, &discovery_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&discovery_addr, sizeof(discovery_addr)) < 0) {
        printF(RED, "Connection to Discovery failed\n");
        printF(RED, "Error: %s\n", strerror(errno));
        close(sock);
        free_all_dynamic_memory();
        exit(EXIT_FAILURE);
    }

    // Usar frame_creator para construir la trama
    char data[HEADER_MAX_SIZE];
    snprintf(data, sizeof(data), "%s&%s&%d", config->username, config->poole_ip, config->poole_port);
    Frame request_frame = frame_creator(0x01, "NEW_POOLE", data);

    if (send_frame(sock, &request_frame) < 0) {
        printF(RED, "Failed to send NEW_POOLE frame\n");
        close(sock);
        return;
    }

    Frame response_frame;
    if (receive_frame(sock, &response_frame) <= 0) {
        printF(RED, "Error receiving response from Discovery\n");
        close(sock);
        return;
    }

    if (strcmp(response_frame.header_plus_data, "CON_OK") == 0) {
        printF(GREEN, "Connected to HAL 9000 System, ready to listen to Bowmans petitions\n\n");
    } else {
        printF(RED, "Connection to Discovery failed: %s\n", response_frame.header_plus_data);
        close(sock);
        exit(EXIT_FAILURE);
    }

    close(sock);
}

/**
 * @brief Crea un socket de escucha en el puerto especificado
 * 
 * @param port Puerto en el que se escucharán las conexiones
 * 
 * @return int Descriptor del socket si la conexión es exitosa, EXIT_FAILURE en caso de error
*/
int setup_listen_socket(int port) {
    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket < 0) {
        printF(RED, "Error creating listening socket.\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Escuchar en todas las interfaces
    server_addr.sin_port = htons(port);

    if (bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printF(RED, "Error binding listening socket.\n");
        close(listen_socket);
        return EXIT_FAILURE;
    }

    if (listen(listen_socket, 128) < 0) { // Ajustar el backlog (preguntar becario)
        printF(RED, "Error listening on socket.\n");
        close(listen_socket);
        return EXIT_FAILURE;
    }

    return listen_socket;
}

/**
 * @brief Función que se ejecuta cuando se pulsa Ctrl+C
 * 
 * @param signum Señal que se ha recibido
 * @return void
*/
void kctrlc(int signum) {
    if (signum == SIGINT) {
        disconnect_notification_to_discovery(poole_config);
        poole_sigint_received = 1;
        server_sigint_received = 1;
        free_all_dynamic_memory();
    } 

    exit(EXIT_SUCCESS);
}

int disconnect_notification_to_discovery(const PooleConfig *config){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printF(RED, "Socket creation failed\n");
        return EXIT_FAILURE;    
    }

    struct sockaddr_in discovery_addr;
    memset(&discovery_addr, 0, sizeof(discovery_addr));
    discovery_addr.sin_family = AF_INET;
    discovery_addr.sin_port = htons(config->discovery_port);
    inet_pton(AF_INET, config->discovery_ip, &discovery_addr.sin_addr);

    pthread_mutex_lock(&send_frame_mutex);
    if (connect(sock, (struct sockaddr *)&discovery_addr, sizeof(discovery_addr)) < 0) {
        printF(RED, "Connection to Discovery failed\n");
        close(sock);
        return EXIT_FAILURE;
    }
    pthread_mutex_unlock(&send_frame_mutex);
    
    char logout_data[HEADER_MAX_SIZE];
    snprintf(logout_data, HEADER_MAX_SIZE, "%s&%s&%d", config->username, config->discovery_ip, config->discovery_port);
    Frame logout_frame = frame_creator(0x06, "POOLE_SHUTDOWN", logout_data);

    pthread_mutex_lock(&send_frame_mutex);
    if (send_frame(sock, &logout_frame) < 0) {
        printF(RED, "Error sending SHUTDOWN frame to Discovery server.\n");
        return EXIT_FAILURE;
    }
    pthread_mutex_unlock(&send_frame_mutex);

    Frame discovery_disconnect_frame;
    if (receive_frame(sock, &discovery_disconnect_frame) <= 0) {
        printF(RED, "Error receiving response from Discovery server.\n");
        return EXIT_FAILURE;
    }

    close(sock);
    printF(GREEN, "\n\nDisconnected from Discovery server.\n");
    return EXIT_SUCCESS;
}