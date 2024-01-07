#include "connection_handler.h"

// Variables globales importadas de otros TADS
extern thread_receive_frames *pargs;
extern pthread_t listen_poole_thread;
extern void *receive_frames(void *args);
extern int global_server_port;
extern char global_server_ip[INET_ADDRSTRLEN]; // Tamaño suficiente para direcciones IPv4
extern char *global_server_name;
extern int poole_socket_for_bowman;

/**
 * @brief Establece una conexión con un servidor Poole.
 *
 * Utiliza las variables globales global_server_ip y global_server_port para
 * configurar la dirección y puerto del servidor. Almacena el descriptor del
 * socket en la variable global poole_socket_for_bowman.
 *
 * @return Descriptor del socket conectado en caso de éxito, o -1 en caso de error.
 */
int connect_to_poole_server() {
    // Crear un socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    poole_socket_for_bowman = sock;
    if (poole_socket_for_bowman < 0) {
        perror("Error creating socket");
        return -1;
    }

    // Configurar la dirección del servidor
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(global_server_port);
    if (inet_pton(AF_INET, global_server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(poole_socket_for_bowman);
        return -1;
    }

    // Conectar al servidor
    if (connect(poole_socket_for_bowman, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        close(poole_socket_for_bowman);
        return -1;
    }

    printf("Connected to server %s at IP %s and port %d\n", global_server_name, global_server_ip, global_server_port);
    return poole_socket_for_bowman;
}

/**
 * @brief Crea un socket y se conecta al servidor Poole asignado
 * 
 * @param psocket Puntero al descriptor del socket
 * 
 * @return void 
*/
void connect_to_server(int *psocket, int global_server_port, char *global_server_ip, int poole_socket) {
    poole_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (poole_socket < 0) {
        printF(RED, "Socket creation failed\n");
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(global_server_port);
    if (inet_pton(AF_INET, global_server_ip, &server_addr.sin_addr) <= 0) {
        printF(RED, "Invalid address/ Address not supported\n");
        close(poole_socket);
        return;
    }

    if (connect(poole_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printF(RED, "Connection to Poole failed\n");
        close(poole_socket);
        return;
    }

    *psocket = poole_socket;
}

/**
 * @brief Establece una conexión con el servidor Discovery
 * 
 * @param config Configuración de Bowman que contiene la IP y el puerto del servidor Discovery
 * @return int Descriptor del socket si la conexión es exitosa, EXIT_FAILURE en caso de error
*/
int connect_to_discovery(char *username, char *discovery_ip, int discovery_port) {
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printF(RED, "Socket creation failed\n");
        return EXIT_FAILURE;
    }

    printF(GREEN, "\nConnecting to Discovery server at IP %s and port %d\n", discovery_ip, discovery_port);

    struct sockaddr_in discovery_addr;
    memset(&discovery_addr, 0, sizeof(discovery_addr));
    discovery_addr.sin_family = AF_INET;
    discovery_addr.sin_port = htons(discovery_port);
    inet_pton(AF_INET, discovery_ip, &discovery_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&discovery_addr, sizeof(discovery_addr)) < 0) {
        printF(RED, "Connection to Discovery failed\n");
        close(sock);
        return EXIT_FAILURE;
    }

    printF(GREEN, "Connected to Discovery server\n");

    // Construir y enviar trama NEW_BOWMAN
    Frame request_frame = frame_creator(0x01, "NEW_BOWMAN", username);

    if (send_frame(sock, &request_frame) < 0) {
        printF(RED, "Failed to send NEW_BOWMAN frame\n");
        close(sock);
        return EXIT_FAILURE;
    }

    // Recibir y procesar la respuesta de Discovery
    Frame response_frame;
    if (receive_frame(sock, &response_frame) <= 0) {
        printF(RED, "Error receiving response from Discovery\n");
        close(sock);
        return EXIT_FAILURE;
    }

    if (strncmp(response_frame.header_plus_data, "CON_OK", response_frame.header_length) == 0) {
        // Procesar y almacenar la información del servidor Poole
        char *data = response_frame.header_plus_data + response_frame.header_length;
        parse_and_store_server_info(data);
        close(sock);
    }else if(strncmp(response_frame.header_plus_data, "CON_KO", response_frame.header_length) == 0){
        close(sock);
        return EXIT_FAILURE;
    }
    else {
        printF(RED, "Connection to Discovery failed: %s\n", response_frame.header_plus_data);
        close(sock);
        return EXIT_FAILURE;
    }

    return sock;
}

/**
 * @brief Crea una trama EXIT y la envía al servidor Poole
 * 
 * @param username Nombre de usuario
 * @param discovery_ip Dirección IP del servidor Discovery
 * @param discovery_port Puerto del servidor Discovery
 * 
 * @return EXIT_SUCCESS si el logout es exitoso, EXIT_FAILURE en caso contrario
*/
int disconnect_notification_to_discovery(char *username, char *discovery_ip, int discovery_port, int global_server_port, char *global_server_ip) {
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printF(RED, "Socket creation failed\n");
        return EXIT_FAILURE;    
    }

    struct sockaddr_in discovery_addr;
    memset(&discovery_addr, 0, sizeof(discovery_addr));
    discovery_addr.sin_family = AF_INET;
    discovery_addr.sin_port = htons(discovery_port);
    inet_pton(AF_INET, discovery_ip, &discovery_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&discovery_addr, sizeof(discovery_addr)) < 0) {
        printF(RED, "Connection to Discovery failed\n");
        close(sock);
        return EXIT_FAILURE;
    }

    char logout_data[HEADER_MAX_SIZE];
    snprintf(logout_data, sizeof(logout_data), "%s&%s&%d", username, global_server_ip, global_server_port);
    Frame logout_frame = frame_creator(0x06, "EXIT", logout_data);

    if (send_frame(sock, &logout_frame) < 0) {
        printF(RED, "Error sending LOGOUT frame to Discovery server.\n");
        return EXIT_FAILURE;
    }

    Frame discovery_disconnect_frame;
    if (receive_frame(sock, &discovery_disconnect_frame) <= 0) {
        printF(RED, "Error receiving response from Poole server.\n");
        return EXIT_FAILURE;
    }

    close(sock);
    return EXIT_SUCCESS;
}

/**
 * @brief Conecta a un servidor Poole alternativo en caso de fallo del servidor actual.
 *
 * Primero intenta establecer una conexión con el servidor Discovery usando las credenciales
 * proporcionadas y luego establece una conexión con un servidor Poole asignado por el servidor
 * Discovery. Gestiona la creación y envío de tramas para el inicio de sesión y la recepción de
 * tramas de respuesta. En caso de éxito, lanza un hilo para escuchar las tramas del servidor Poole.
 *
 * @param username Nombre de usuario para la conexión.
 * @param discovery_ip Dirección IP del servidor Discovery.
 * @param port Puerto del servidor Discovery.
 * @param psocket Puntero al descriptor del socket de Poole.
 * @param discovery_socket Descriptor del socket de Discovery (ya conectado).
 * @param poole_socket Descriptor del socket de Poole (ya conectado).
 * @param msg_id Identificador para la cola de mensajes.
 * @param global_server_name Nombre global del servidor Poole asignado.
 * @param global_server_port Puerto global del servidor Poole asignado.
 * @param global_server_ip Dirección IP global del servidor Poole asignado.
 * 
 * @return 1 en caso de éxito, 0 en caso de error.
 */
int connect_to_another_server(char *username, char *discovery_ip, int port, int *psocket, int discovery_socket, int poole_socket, int msg_id, char* global_server_name, int global_server_port, char *global_server_ip) {

    discovery_socket = connect_to_discovery(username, discovery_ip, port);
    if (discovery_socket == EXIT_FAILURE) {
        printF(RED, "No sockets available\n");
        return 0;
    }
    
    if (connect_to_poole_server() < 0) {
        printF(RED, "No sockets available\n");
        return 0;
    }

    Frame response_frame = frame_creator(0x01, "NEW_BOWMAN", username);
    if (send_frame(poole_socket_for_bowman, &response_frame) < 0) {
        printF(RED, "Failed to send NEW_BOWMAN frame\n");
        close(poole_socket);
        return 0;
    }

    Frame request_frame;
    if (receive_frame(poole_socket_for_bowman, &request_frame) <= 0) {
        printF(RED, "Error receiving frame from Bowman. Connection aborted.\n");
        close(poole_socket_for_bowman);
        return 0;
    }
    if (strncmp(request_frame.header_plus_data, "CON_KO", request_frame.header_length) == 0) {
        printF(RED, "Connection failed with Poole");
        return 0;
    }
    printF(GREEN, "%s connected to HAL 9000 system, welcome music lover!\n", username);
    printF(GREEN, "Connected to Poole server [%s] at IP %s and port %d\n", global_server_name, global_server_ip, global_server_port);
    //Crear la cola de mensajes
    msg_id = msgget(IPC_PRIVATE, 0600 | IPC_CREAT);
    if (msg_id == -1) {
        printF(RED, "Error creating message queue\n");
        return 0;
    }

    //Lanzar el thread que escucha las tramas del servidor Poole
    pargs->poole_socket = psocket;
    pargs->username = username;
    pargs->discovery_ip = discovery_ip;
    pargs->discovery_port = port;

    if (pthread_create(&listen_poole_thread, NULL, receive_frames, pargs) != 0) {
        printF(RED, "Error creating thread to receive frames.\n");
        return 0;
    }

    return 1;
}