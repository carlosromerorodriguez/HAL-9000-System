#include "command_handler.h"

// Variables globales
char *global_server_name = NULL; // Nombre del servidor asignado
char global_server_ip[INET_ADDRSTRLEN]; // Tamaño suficiente para direcciones IPv4
int global_server_port;
static int poole_socket;
extern pthread_t listen_poole_thread;
extern volatile sig_atomic_t bowman_sigint_received;
extern thread_receive_frames *pargs;
int msg_id;

// Declaración de funciones
void connect_to_server(int *psocket);
int connect_to_discovery(char *username, char *discovery_ip, int discovery_port);
void parse_and_store_server_info(const char* server_info);
void list_songs();
int logout(char *username);
int disconnect_notification_to_discovery(char *username, char *discovery_ip, int discovery_port);
void list_playlists();
void *receive_frames(void *args);
void download(char *name);
void* startSongDownload(void* args);

/**
 * @brief Parse the command to remove the extra spaces
 * @example "LIST             SONGS" -> "LIST SONGS
 * 
 * @param command Command to parse
 * @return char* Parsed command
*/
static char *parse_command(char *command) {
    char *opcode = strtok(command, " ");
    char *aux = strtok(NULL, " ");
    
    if (aux != NULL && aux[0] != '\0') {
        char *combined = (char *) malloc (strlen(opcode) + strlen(aux) + 2);
        sprintf(combined, "%s %s", opcode, aux);
        return combined;
    }

    return strdup(opcode);
}

/**
 * @brief Remove the extra spaces from the command
 * 
 * @param str Command to trim
 * 
 * @return char* Trimmed command
*/
static char *trim_whitespace(char* str) {
    char* end;

    // Trim leading spaces
    while(isspace((unsigned char)*str)) {
        str++;
    }

    // All spaces?
    if(*str == 0) { 
        return str;
    } 

    // Trim trailing spaces
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) {
        end--;
    }

    end[1] = '\0';

    return str;
}

/**
 * @brief Handle the command received from the user
 * 
 * @param command Command to handle
 * @param connected Flag to know if the user wants to logout or not
*/
unsigned char handle_bowman_command(char *command, unsigned char *connected, int *discovery_socket, char *username, char *discovery_ip, int discovery_port, int *psocket) {

    command = trim_whitespace(command);

    if (command == NULL || command[0] == '\0') {
        printF(RED, "ERROR: Please input a valid command.\n");
        return EXIT_SUCCESS;
    }

    char *opcode = parse_command(command);

    if (!*connected) {
        if (!strcasecmp(opcode, "CONNECT")) {

            *discovery_socket = connect_to_discovery(username, discovery_ip, discovery_port);
            if (*discovery_socket == EXIT_FAILURE) {
                printF(RED, "Failed to connect to Discovery server\n");
                free(opcode);
                return (EXIT_FAILURE);
            }

            // Crear un socket y conectarse al servidor Poole asignado
            connect_to_server(psocket);
            if (*psocket == EXIT_FAILURE) {
                printF(RED, "Failed to connect to Poole server\n");
                free(opcode);
                return (EXIT_FAILURE);
            }

            Frame response_frame = frame_creator(0x01, "NEW_BOWMAN", username);
            if (send_frame(poole_socket, &response_frame) < 0) {
                printF(RED, "Failed to send NEW_BOWMAN frame\n");
                free(opcode);
                close(poole_socket);
                return EXIT_FAILURE;
            }

            Frame request_frame;
            if (receive_frame(poole_socket, &request_frame) <= 0) {
                printF(RED, "Error receiving frame from Bowman. Connection aborted.\n");
                free(opcode);
                close(poole_socket);
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

            //Crear la cola de mensajes
            key_t key = ftok("bowman.c", 12);
            if (key == (key_t)-1) {
                printF(RED, "Error creating key for message queue\n");
                free(opcode);
                return EXIT_FAILURE;
            }

            msg_id = msgget(key, 0600 | IPC_CREAT);
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

        } else if (strcasecmp(opcode, "LOGOUT") == 0) { //PREGUNTAR SI SE PUEDE SALIR SIN ESTAR CONECTADO
            printF(GREEN, "Thanks for using HAL 9000, see you soon, music lover!\n");
            free(opcode);
            return EXIT_FAILURE;
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
            return EXIT_FAILURE;
        } else if (!strncasecmp(opcode, "DOWNLOAD", 8)) {

            download(command + strlen("DOWNLOAD "));
          
        } else if (!strcasecmp(opcode, "CHECK DOWNLOADS")) {
            /* 
                Handle the CHECK DOWNLOADS
            */
           printF(WHITE, "Check downloads\n");

        } else if (!strcasecmp(opcode, "LIST SONGS")) {
            
            list_songs();

        } else if (!strcasecmp(opcode, "LIST PLAYLISTS")) {

            list_playlists();

        } else {

            printF(RED, "ERROR: Please input a valid command.\n");
        }
    }
    
    free(opcode);
    return EXIT_SUCCESS;
}

/**
 * @brief Crea un socket y se conecta al servidor Poole asignado
 * 
 * @param psocket Puntero al descriptor del socket
 * 
 * @return void 
*/
void connect_to_server(int *psocket) {
    poole_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (poole_socket < 0) {
        perror("Error creating socket");
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(global_server_port);
    if (inet_pton(AF_INET, global_server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        close(poole_socket);
        return;
    }

    if (connect(poole_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
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
    } else {
        printF(RED, "Connection to Discovery failed: %s\n", response_frame.header_plus_data);
        close(sock);
        return EXIT_FAILURE;
    }

    return sock;
}

/**
 * @brief Parsea la información del servidor Poole y la almacena en variables globales
 * 
 * @param server_info Información del servidor Poole
 * 
 * @return void
*/
void parse_and_store_server_info(const char* server_info) {
    char info_copy[strlen(server_info) + 1];
    strcpy(info_copy, server_info);

    char *token = strtok(info_copy, "&");
    if (token != NULL) {
        global_server_name = malloc(strlen(token) + 1);
        strcpy(global_server_name, token);
        global_server_name[strlen(token)] = '\0';

        token = strtok(NULL, "&");
    } if (token != NULL) {
        strcpy(global_server_ip, token); // Dirección IP
        token = strtok(NULL, "&");
    } if (token != NULL) {
        global_server_port = atoi(token); // Puerto
    }
}

/**
 * @brief Crea una trama LIST_SONGS y la envía al servidor Poole
 * 
 * @return void
*/
void list_songs() {
    Frame list_songs_frame = frame_creator(0x02, "LIST_SONGS", "");

    // Enviar la trama al servidor Poole
    if (send_frame(poole_socket, &list_songs_frame) < 0) {
        printF(RED, "Error sending LIST_SONGS frame to Poole server.\n");
        return;
    } 

    //Leer el tamaño del mensaje a mostrar de la cola de mensajes
    Message_buffer *msg = malloc(sizeof(Message_buffer));
    memset(msg, 0, sizeof(Message_buffer));
    msg->msg_type = 1;
    msg_queue_reader(msg_id, msg);
    printF(GREEN, "Tamaño del fichero: %d\n", atoi(msg->msg_text));
    int size_fichero = atoi(msg->msg_text);
    //APLICAR EXCLUSIÓN MUTUA
    //Leer de la cola de mensajes la respuesta del servidor Poole
    while(size_fichero > 0){
        msg = malloc(sizeof(Message_buffer));
        memset(msg, 0, sizeof(Message_buffer));
        msg->msg_type = 1;
        msg_queue_reader(msg_id, msg);
        printF(GREEN, "Message received 340: %s\n", msg->msg_text);
        size_fichero -= strlen(msg->msg_text);
    }
}

/**
 * @brief Crea una trama LIST_PLAYLISTS y la envía al servidor Poole
 * 
 * @return void
*/
void list_playlists() {
    Frame playlists_frame = frame_creator(0x02, "LIST_PLAYLISTS", "");

    if (send_frame(poole_socket, &playlists_frame) < 0) {
        printF(RED, "Error sending LIST_PLAYLISTS frame to Poole server.\n");
        return;
    } 
    //Leer de la cola de mensajes la respuesta del servidor Poole
    Message_buffer *msg = malloc(sizeof(Message_buffer));
    msg->msg_type = 2;

    //TODO: BUCLE ESPERANDO A COMPLETAR LOS BYTES TOTALES QUE ENVÍA POOLE
    msg_queue_reader(msg_id, msg);
    printF(GREEN, "Message received 359: %s\n", msg->msg_text);

    free(msg);
}

void *receive_frames(void *args) {
    thread_receive_frames trf = *(thread_receive_frames *)args;
    Frame response_frame;

    while (!bowman_sigint_received) {
        if (receive_frame(*trf.poole_socket, &response_frame) <= 0) {
            continue;   
        }
        //RECIBIR EL TAMAÑO DE LIST_SONGS A MOSTRAR
        if (!strncasecmp(response_frame.header_plus_data , "LIST_SONGS_SIZE", response_frame.header_length)) {
            //printf("\nList Songs Size: %s\n", response_frame.header_plus_data + response_frame.header_length);
            //printf("Size: %d\n", atoi(response_frame.header_plus_data + response_frame.header_length));
            Message_buffer *msg = malloc(sizeof(Message_buffer));
            memset(msg, 0, sizeof(Message_buffer));
            msg->msg_type = 1;
            strncpy(msg->msg_text, response_frame.header_plus_data + response_frame.header_length, HEADER_MAX_SIZE);
            msg->msg_text[HEADER_MAX_SIZE] = '\0';
            printF(GREEN, "Message received 398: %s\n", msg->msg_text);
            msg_queue_writer(msg_id, msg);
        }
        else if (!strncasecmp(response_frame.header_plus_data , "SONGS_RESPONSE", response_frame.header_length)) {
            //MATAR DOS PAJAROS DE UN TIRO :)
            //printF(GREEN, "\nSongs Response: %s\n", response_frame.header_plus_data);
            //printF(GREEN, "\nTamaño de la trama: %d\n", strlen(response_frame.header_plus_data));
            Message_buffer *msg = malloc(sizeof(Message_buffer));
            memset(msg, 0, sizeof(Message_buffer));
            msg->msg_type = 1;
            memset(msg->msg_text, 0, sizeof(msg->msg_text));
            strcpy(msg->msg_text, response_frame.header_plus_data);
            msg_queue_writer(msg_id, msg);
        }
        else if (!strncasecmp(response_frame.header_plus_data , "PLAYLISTS_RESPONSE", response_frame.header_length)) {
            //printF(GREEN, "\nPlaylists Response: %s\n", response_frame.header_plus_data);
            //printF(GREEN, "\nTamaño de la trama: %d\n", strlen(response_frame.header_plus_data));
            Message_buffer *msg = malloc(sizeof(Message_buffer));
            memset(msg, 0, sizeof(Message_buffer));
            msg->msg_type = 4;
            memset(msg->msg_text, 0, sizeof(msg->msg_text));
            strcpy(msg->msg_text, response_frame.header_plus_data);
            msg_queue_writer(msg_id, msg);
        }
        else if (!strncasecmp(response_frame.header_plus_data, "NEW_FILE", response_frame.header_length)) {
            
            int data_length = strlen(response_frame.header_plus_data + response_frame.header_length) + 1;
            char* data_copy = malloc(data_length);
            if (data_copy == NULL) {
                printF(RED, "Malloc\n");
                continue; 
            }
            strcpy(data_copy, response_frame.header_plus_data + response_frame.header_length);

            pthread_t new_file;
            if (pthread_create(&new_file, NULL, startSongDownload, data_copy) != 0) {
                printF(RED, "Pthread_create\n");
                free(data_copy); 
                continue; 
            }
            //pthread_detach(new_file); 
        }
        else if (!strncasecmp(response_frame.header_plus_data, "FILE_DATA", response_frame.header_length)) {

            int data_length = strlen(response_frame.header_plus_data + response_frame.header_length) + 1;
            char* data_copy = malloc(data_length);
            if (data_copy == NULL) {
                perror("malloc");
                continue; 
            }
            strcpy(data_copy, response_frame.header_plus_data + response_frame.header_length);            
            char* datos = strdup(data_copy);
            if (datos != NULL) {
                Message_buffer msg;
                long id;
                char* ampersand = strchr(datos, '&');
                ampersand = ampersand + 1;
                if (ampersand != NULL) {
                    strcpy(msg.msg_text, ampersand); 
                }
                id = atoi(datos);
                msg.msg_type = id;

                msg_queue_writer(msg_id, &msg);

                free(datos);
            }
            
            free(data_copy);
        }

        else if (!strncasecmp(response_frame.header_plus_data , "LOGOUT_OK", response_frame.header_length)) {
            printF(GREEN, "Logout successful\n");
            disconnect_notification_to_discovery(trf.username, trf.discovery_ip, trf.discovery_port);
            pthread_exit(NULL);
        }
        else if (!strncasecmp(response_frame.header_plus_data , "LOGOUT_KO", response_frame.header_length)) {
            printF(RED, "Logout failed\n");
            pthread_exit(NULL);
        }
        else if (!strncasecmp(response_frame.header_plus_data , "UNKNOWN", response_frame.header_length)) {
 
        }
    }

    pthread_exit(NULL);
}

//LOGOUT FUNCTIONS
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

    if (send_frame(poole_socket, &logout_frame) < 0) {
        printF(RED, "Error sending LOGOUT frame to Poole server.\n");
        return EXIT_FAILURE;
    } 

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
int disconnect_notification_to_discovery(char *username, char *discovery_ip, int discovery_port){
    
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

void download(char *name){

    Frame download_frame = frame_creator(0x03, "DOWNLOAD_SONG", name);

    if (send_frame(poole_socket, &download_frame) < 0) {
        printF(RED, "Error sending DOWNLOAD_SONG frame to Poole server.\n");
        return ;
    } 

}

void parseSongInfo(const char *str, Song *song) {
    char *token;
    char *tempStr;
    size_t length;

    // Copiar la cadena original para no modificarla durante el uso de strtok
    tempStr = strdup(str);

    // Extraer el nombre del archivo
    token = strtok(tempStr, "&");
    length = strlen(token) + 1;
    song->fileName = malloc(length);
    strncpy(song->fileName, token, length);

    // Extraer el tamaño del archivo
    token = strtok(NULL, "&");
    song->fileSize = strtol(token, NULL, 10);

    // Extraer el MD5SUM
    token = strtok(NULL, "&");
    length = strlen(token) + 1;
    song->md5sum = malloc(length);
    strncpy(song->md5sum, token, length);

    // Extraer el ID
    token = strtok(NULL, "&");
    song->id = atoi(token);

    // Liberar la memoria temporal
    free(tempStr);
}

void* startSongDownload(void* args) {
    char* str = (char*)args; 

    Song newSong;
    parseSongInfo(str, &newSong); 

    long fileSize = newSong.fileSize;

    int fd = open(newSong.fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Error opening file");
        return NULL;
    }

    Message_buffer *msg = malloc(sizeof(Message_buffer));
    msg->msg_type = 2;

    long total_bytes_written = 0;
    
    while (total_bytes_written < fileSize) {

        msg_queue_reader(msg_id, msg);

        printF(RED, "Escribimos -> %s\n", msg->msg_text);

        int written = write(fd, msg->msg_text, sizeof(msg->msg_text));
        if (written < 0) {
            perror("Error writing to file");
            break;
        }
        total_bytes_written += written;
        //printF(RED ,"%ld / %ld", total_bytes_written , fileSize);
    }

    printF(GREEN, "\nDescarrega completada\n");
    close(fd);
    free(msg);

    return NULL;
}

