#include "session_handler.h"

// Variables globales
char *bowman_folder_path = NULL;
int bowman_is_connected = 0;
int msg_id;
int poole_socket_for_bowman;
int global_server_port;
char global_server_ip[INET_ADDRSTRLEN]; // Tamaño suficiente para direcciones IPv4
char *global_server_name = NULL;
char *username;
extern int discovery_socket;
int disc_port;
char *discovery_ip; 
int *psocket;

// Variables globales importadas de otros TADS
extern thread_receive_frames *pargs;
extern pthread_t listen_poole_thread;

// Funciones importadas de otros TADS
extern void *receive_frames(void *args);

/**
 * @brief Handle the command received from the user
 * 
 * @param command Command to handle
 * @param connected Flag to know if the user wants to logout or not
*/
unsigned char handle_bowman_command(char *command, unsigned char *connected, int *discovery_socket, char *username, char *discovery_ip, int discovery_port, int *psocket, char* folder_path) {
    bowman_folder_path = folder_path;
    command = trim_whitespace(command);
    discovery_socket = discovery_socket;
    username = username;
    discovery_ip = discovery_ip;
    disc_port = discovery_port;
    psocket = psocket;
    poole_socket_for_bowman = *psocket;

    if (command == NULL || command[0] == '\0') {
        printF(RED, "ERROR: Please input a valid command.\n");
        return EXIT_SUCCESS;
    }

    char *opcode = parse_command(command);

    if (!*connected || bowman_is_connected == 0) {
        
        if (!strcasecmp(opcode, "CONNECT")) {

            *discovery_socket = connect_to_discovery(username, discovery_ip, discovery_port);
            if (*discovery_socket == EXIT_FAILURE) {
                printF(RED, "Failed to connect to Discovery server\n");
                free(opcode);
                return 0;
            }

            // Crear un socket y conectarse al servidor Poole asignado
            connect_to_server(psocket, global_server_port, global_server_ip, poole_socket_for_bowman);
            if (*psocket == EXIT_FAILURE) {
                printF(RED, "Failed to connect to Poole server\n");
                free(opcode);
                return 0;
            }

            poole_socket_for_bowman = *psocket;

            Frame response_frame = frame_creator(0x01, "NEW_BOWMAN", username);
            if (send_frame(poole_socket_for_bowman, &response_frame) < 0) {
                printF(RED, "Failed to send NEW_BOWMAN frame\n");
                free(opcode);
                close(poole_socket_for_bowman);
                return EXIT_FAILURE;
            }

            Frame request_frame;
            if (receive_frame(poole_socket_for_bowman, &request_frame) <= 0) {
                printF(RED, "Error receiving frame from Poole. Connection aborted.\n");
                free(opcode);
                close(poole_socket_for_bowman);
                return EXIT_FAILURE;
            }
            // Checkear si la conexión se ha realizado correctamente
            if (strncmp(request_frame.header_plus_data, "CON_KO", request_frame.header_length) == 0) {
                printF(RED, "Connection failed with Poole");
                free(opcode);
                return EXIT_FAILURE;
            }
            printF(GREEN, "%s connected to HAL 9000 system, welcome music lover!\n", username);
            printF(GREEN, "Connected to Poole server [%s] at IP %s and port %d\n", global_server_name, global_server_ip, global_server_port);
            *connected = 1; 
            bowman_is_connected = 1;

            //Crear la cola de mensajes
            msg_id = msgget(IPC_PRIVATE, 0600 | IPC_CREAT);
            if (msg_id == -1) {
                printF(RED, "Error creating message queue\n");
                free(opcode);
                return EXIT_FAILURE;
            }

            //Lanzar el thread que escucha las tramas del servidor Poole
            pargs = malloc (sizeof(thread_receive_frames));
            pargs->poole_socket = psocket;
            pargs->username = username;
            pargs->discovery_ip = discovery_ip;
            pargs->discovery_port = discovery_port;

            if (pthread_create(&listen_poole_thread, NULL, receive_frames, pargs) != 0) {
                printF(RED, "Error creating thread to receive frames.\n");
                free(opcode);
                return EXIT_FAILURE;
            }

        } else if (strcasecmp(opcode, "LOGOUT") == 0) { 
            printF(GREEN, "Thanks for using HAL 9000, see you soon, music lover!\n");
            free(opcode);
            free(global_server_name);
            return 1;
        } else {
            printF(RED, "Cannot execute command, you are not connected to HAL 9000\n");
        }

    } else {
        if (!strcasecmp(opcode, "LOGOUT")) {
            if (logout(username)) {
                printF(RED, "\nLogout failed\n");
            } else {
                printF(GREEN, "\nThanks for using HAL 9000, see you soon, music lover!\n");
            }

            free(opcode);
            return 1;
        } else if (!strncasecmp(opcode, "DOWNLOAD", 8)) {

            download(command + strlen("DOWNLOAD "));
          
        } else if (!strcasecmp(opcode, "CHECK DOWNLOADS")) {
            
            printAllSongsDownloading();            

        } else if (!strcasecmp(opcode, "LIST SONGS")) {
            
            list_songs();

        } else if (!strcasecmp(opcode, "LIST PLAYLISTS")) {

            list_playlists();

        } 
        else if (!strcasecmp(opcode, "CLEAR DOWNLOADS")) {

            clearDownloadedSongs();

        } else {

            printF(RED, "ERROR: Please input a valid command.\n");
        }
    }
    
    

    free(opcode);
    return EXIT_SUCCESS;
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
int logout(char *username) {
    
    Frame logout_frame = frame_creator(0x06, "EXIT", username);

    if (send_frame(poole_socket_for_bowman, &logout_frame) < 0) {
        return EXIT_FAILURE;
    } 

    return EXIT_SUCCESS;
}