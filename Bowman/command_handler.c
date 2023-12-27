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
Song_Downloading *songsDownloading = NULL;
int num_songs_downloading = 0;
int discovery_socket;
char *username;
char *discovery_ip;
int *psocket;
int bowman_is_connected = 0;

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
//Handle List songs
void handleListSongsSize(char* data);
void handleSongsResponse(char *data);
int countSongs(const char *str);
//Handle List playlists
void handleListPlaylistsSize(char *data);
void handlePlaylistsResponse(char *data);
int countPlaylists(const char *str);
void print_playlists(char *to_print);
void printSongsInPlaylists(char *playlist, char playlistIndex);
//Handle downloads
void handleNewFile(char* data);
void handleFileData(char* data);
void clearDownloadedSongs();
void parseSongInfo(const char *str, Song *song);
void printAllSongsDownloading() ;
int connect_to_another_server(char *username, char *discovery_ip, int port, int *psocket);

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
    discovery_socket = discovery_socket;
    username = username;
    discovery_ip = discovery_ip;
    psocket = psocket;

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

    // Leer el tamaño del mensaje a mostrar de la cola de mensajes
    Message_buffer *msg = malloc(sizeof(Message_buffer));
    if (!msg) {
        printF(RED, "Error allocating memory for message buffer.\n");
        return;
    }
    memset(msg, 0, sizeof(Message_buffer));
    msg->msg_type = 1000;
    msg_queue_reader(msg_id, msg);
    int size_fichero = atoi(msg->msg_text);
    free(msg); 

    // APLICAR EXCLUSIÓN MUTUA
    // Leer de la cola de mensajes la respuesta del servidor Poole
    usleep(200000);
    char *to_print = malloc(sizeof(char));  // Se inicializa con un tamaño de 1 para el carácter nulo
    if (!to_print) {
        printF(RED, "Error allocating memory for to_print.\n");
        return;
    }
    to_print[0] = '\0';  // Asegurar cadena vacía

    while(size_fichero > 0){
        msg = malloc(sizeof(Message_buffer));
        if (!msg) {
            printF(RED, "Error allocating memory for message buffer.\n");
            free(to_print);
            return;
        }
        memset(msg, 0, sizeof(Message_buffer));
        msg->msg_type = 1000;
        msg_queue_reader(msg_id, msg);

        size_t old_size = strlen(to_print);
        size_t new_size = old_size + strlen(msg->msg_text) + 1;
        char *temp = realloc(to_print, new_size);
        if (!temp) {
            printF(RED, "Error reallocating memory for to_print.\n");
            free(msg);
            free(to_print);
            return;
        }
        to_print = temp;
        strcat(to_print, msg->msg_text);
        
        size_fichero -= strlen(msg->msg_text) + strlen("SONGS_RESPONSE");        
        free(msg);
    }

    // Imprimir el número total de canciones
    printF(GREEN, "There are %d songs available for download:\n", countSongs(to_print));

    // Presentar la lista de canciones
    char *token = strtok(to_print, "&");
    int index = 1;
    while (token != NULL) {
        printF(GREEN, "%d. %s\n", index, token);
        token = strtok(NULL, "&");
        index++;
    }

    free(to_print);
}

int countSongs(const char *str) {
    int count = 0;
    const char *temp = str;
    while (*temp) {
        if (*temp == '&') count++;
        temp++;
    }
    return count + 1;
}

/**
 * @brief Crea una trama LIST_PLAYLISTS y la envía al servidor Poole
 * 
 * @return void
*/
void list_playlists() {
    Frame playlists_frame = frame_creator(0x02, "LIST_PLAYLISTS", "");

    // Enviar la trama al servidor Poole
    if (send_frame(poole_socket, &playlists_frame) < 0) {
        printF(RED, "Error sending LIST_SONGS frame to Poole server.\n");
        return;
    } 

    // Leer el tamaño del mensaje a mostrar de la cola de mensajes
    Message_buffer *msg = malloc(sizeof(Message_buffer));
    if (!msg) {
        printF(RED, "Error allocating memory for message buffer.\n");
        return;
    }
    memset(msg, 0, sizeof(Message_buffer));
    msg->msg_type = 1001;
    msg_queue_reader(msg_id, msg);
    int size_fichero = atoi(msg->msg_text);
    free(msg); 

    // APLICAR EXCLUSIÓN MUTUA
    // Leer de la cola de mensajes la respuesta del servidor Poole
    usleep(200000);
    char *to_print = malloc(sizeof(char));  // Se inicializa con un tamaño de 1 para el carácter nulo
    if (!to_print) {
        printF(RED, "Error allocating memory for to_print.\n");
        return;
    }
    to_print[0] = '\0';  // Asegurar cadena vacía

    while(size_fichero > 0){
        msg = malloc(sizeof(Message_buffer));
        if (!msg) {
            printF(RED, "Error allocating memory for message buffer.\n");
            free(to_print);
            return;
        }
        memset(msg, 0, sizeof(Message_buffer));
        msg->msg_type = 1001;
        msg_queue_reader(msg_id, msg);

        size_t old_size = strlen(to_print);
        size_t new_size = old_size + strlen(msg->msg_text) + 1;
        char *temp = realloc(to_print, new_size);
        if (!temp) {
            printF(RED, "Error reallocating memory for to_print.\n");
            free(msg);
            free(to_print);
            return;
        }
        to_print = temp;
        strcat(to_print, msg->msg_text);
        
        size_fichero -= strlen(msg->msg_text) + strlen("PLAYLISTS_RESPONSE");        
        free(msg);
    }
    
    printF(GREEN, "There are %d playlists available for download:\n", countPlaylists(to_print));

    char *temp = strdup(to_print + 1);
    print_playlists(temp);

    free(temp);
    free(to_print);
}

int countPlaylists(const char *str) {
    int count = 0;
    const char *temp = str;
    while (*temp) {
        if (*temp == '#') count++;
        temp++;
    }
    return count;
}

void printSongsInPlaylists(char *playlist, char playlistIndex) {
    char *saveptr1;
    char *name = strtok_r(playlist, "&", &saveptr1);
    
    if (name != NULL) {
        printF(GREEN, "%d. %s\n", playlistIndex, name);
        // Procesar las canciones.
        char *song;
        char songIndex = 'a';
        while ((song = strtok_r(NULL, "&", &saveptr1)) != NULL) {
            printF(GREEN, "\t%c. %s\n", songIndex, song);
            songIndex++;
        }
    }
}

void print_playlists(char *to_print) {
    char *saveptr2;
    char *playlist = strtok_r(to_print, "#", &saveptr2);
    int playlistIndex = 1;

    while (playlist != NULL) {
        printSongsInPlaylists(playlist, playlistIndex);
        playlist = strtok_r(NULL, "#", &saveptr2);
        playlistIndex++;
    }
}

int check_if_playlist(char *name){

    int name_length = strlen(name);
    
    if (name_length >= 4 && strcmp(name + name_length - 4, ".mp3") == 0) {
        return 0;
    } else {
        return 1; 
    }

}

//DOWNLOAD SONG FUNCTION 
void download(char *name){

    if(check_if_playlist(name)){
        Frame download_frame = frame_creator(0x03, "DOWNLOAD_LIST", name);

        if (send_frame(poole_socket, &download_frame) < 0) {
        printF(RED, "Error sending DOWNLOAD_LIST frame to Poole server.\n");
        return;
    }
    }else{
        Frame download_frame = frame_creator(0x03, "DOWNLOAD_SONG", name);

        if (send_frame(poole_socket, &download_frame) < 0) {
            printF(RED, "Error sending DOWNLOAD_SONG frame to Poole server.\n");
            return;
        } 
    }

    printF(GREEN, "Download started!\n");
}

void *receive_frames(void *args) {
    thread_receive_frames trf = *(thread_receive_frames *)args;

    while (!bowman_sigint_received) {
        Frame response_frame;
        if (receive_frame(*trf.poole_socket, &response_frame) <= 0) {
            continue;   
        }
        //RECIBIR EL TAMAÑO DE LIST_SONGS A MOSTRAR
        if (!strncasecmp(response_frame.header_plus_data , "LIST_SONGS_SIZE", response_frame.header_length)) {
            handleListSongsSize(response_frame.header_plus_data + response_frame.header_length);
        }
        else if (!strncasecmp(response_frame.header_plus_data , "SONGS_RESPONSE", response_frame.header_length)) {
            handleSongsResponse(response_frame.header_plus_data + response_frame.header_length);            
        }
        else if (!strncasecmp(response_frame.header_plus_data , "LIST_PLAYLISTS_SIZE", response_frame.header_length)) {
            handleListPlaylistsSize(response_frame.header_plus_data + response_frame.header_length);
        }
        else if (!strncasecmp(response_frame.header_plus_data , "PLAYLISTS_RESPONSE", response_frame.header_length)) {
            handlePlaylistsResponse(response_frame.header_plus_data + response_frame.header_length);
        }
        else if (!strncasecmp(response_frame.header_plus_data, "NEW_FILE", response_frame.header_length)) {
            handleNewFile(response_frame.header_plus_data + response_frame.header_length);
        }
        else if (!strncasecmp(response_frame.header_plus_data, "FILE_DATA", response_frame.header_length)) {
            handleFileData(response_frame.header_plus_data + response_frame.header_length);
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
        else if (!strncasecmp(response_frame.header_plus_data , "POOLE_SHUTDOWN", response_frame.header_length)) {
            
            printF(RED, "\nPoole server has shutdown. Reconnecting to Discovery...\n");

            Frame logout_frame = frame_creator(0x06, "SHUTDOWN_OK", "");

            if (send_frame(poole_socket, &logout_frame) < 0) {
                printF(RED, "Error sending SHUTDOWN_OK frame to Poole server.\n");
                pthread_exit(NULL);
            } 

            close(*trf.poole_socket);
            if(!connect_to_another_server(trf.username, trf.discovery_ip, trf.discovery_port, trf.poole_socket)){
                printF(RED, "Error connecting to another server. No Pooles available\n");
                bowman_is_connected = 0;
                printF(WHITE, "$ ");
                pthread_exit(NULL);
            }
            printF(WHITE, "$ ");
        }
        else if (!strncasecmp(response_frame.header_plus_data , "UNKNOWN", response_frame.header_length)) {
 
        }
    }

    pthread_exit(NULL);
}

void handleListSongsSize(char* data) {
    int data_length = strlen(data) + 1;
    char* data_copy = malloc(data_length);
    if (data_copy == NULL) {
        printF(RED, "Malloc\n");
        return; 
    }
    strcpy(data_copy, data);

    Message_buffer *msg = malloc(sizeof(Message_buffer));
    memset(msg, 0, sizeof(Message_buffer));
    msg->msg_type = 1000;
    strcpy(msg->msg_text, data_copy);
    msg_queue_writer(msg_id, msg);  
    free(data_copy);  
    free(msg);
}

void handleSongsResponse(char *data) {
    int data_length = strlen(data) + 1;
    char* data_copy = malloc(data_length);
    if (data_copy == NULL) {
        printF(RED, "Malloc\n");
        return; 
    }
    strcpy(data_copy, data);

    Message_buffer *msg = malloc(sizeof(Message_buffer));
    memset(msg, 0, sizeof(Message_buffer));
    msg->msg_type = 1000;
    memset(msg->msg_text, 0, sizeof(msg->msg_text));
    strcpy(msg->msg_text, data_copy);
    msg_queue_writer(msg_id, msg);
    free(data_copy);
    free(msg);
}

void handleListPlaylistsSize(char *data) {
    int data_length = strlen(data) + 1;
    char* data_copy = malloc(data_length);
    if (data_copy == NULL) {
        printF(RED, "Malloc\n");
        return; 
    }
    strcpy(data_copy, data);

    Message_buffer *msg = malloc(sizeof(Message_buffer));
    memset(msg, 0, sizeof(Message_buffer));
    msg->msg_type = 1001;
    strcpy(msg->msg_text, data_copy);
    msg_queue_writer(msg_id, msg); 
    free(data_copy);
    free(msg);
}

void handlePlaylistsResponse(char *data) {
    int data_length = strlen(data) + 1;
    char* data_copy = malloc(data_length);
    if (data_copy == NULL) {
        printF(RED, "Malloc\n");
        return; 
    }
    strcpy(data_copy, data);

    Message_buffer *msg = malloc(sizeof(Message_buffer));
    memset(msg, 0, sizeof(Message_buffer));
    msg->msg_type = 1001;
    memset(msg->msg_text, 0, sizeof(msg->msg_text));
    strcpy(msg->msg_text, data_copy);
    msg_queue_writer(msg_id, msg);
    free(data_copy);
    free(msg);
}

void handleNewFile(char* data) {
    int data_length = strlen(data) + 1;
    char* data_copy = malloc(data_length);
    if (data_copy == NULL) {
        printF(RED, "Malloc\n");
        return; 
    }
    strcpy(data_copy, data);

    //printF(RED, "New file -> %s\n", data_copy);

    pthread_t new_file;
    if (pthread_create(&new_file, NULL, startSongDownload, data_copy) != 0) {
        printF(RED, "Pthread_create\n");
        free(data_copy); 
        return; 
    } else {
        if (pthread_detach(new_file) != 0) {
            printF(RED, "Pthread_detach\n");
            free(data_copy);
        }
        return;
    }

    free(data_copy);
}

void handleFileData(char* data) {
    // ID (3 bytes [0-999]) + '&' + '\0'
    char id_str[4];
    memcpy(id_str, data, 3);
    id_str[3] = '\0';

    long id = strtol(id_str, NULL, 10);

    Message_buffer msg;
    memset(&msg, 0, sizeof(Message_buffer));
    msg.msg_type = id;


    size_t data_length = HEADER_MAX_SIZE - strlen("FILE_DATA") - 3 - 1; // 3 bytes para el ID + 1 byte para el '&'
    size_t msg_text_length = (data_length < sizeof(msg.msg_text)) ? data_length : sizeof(msg.msg_text);
    memcpy(msg.msg_text, data + 4, msg_text_length);

    //printF(RED, "File data [AFTER] -> %s\n", msg.msg_text);
    msg_queue_writer(msg_id, &msg);
}

void addSongToArray(Song newSong, pthread_t thread_id) {
    Song_Downloading songDownloading;
    songDownloading.song = newSong;
    songDownloading.song_size = newSong.fileSize;
    songDownloading.downloaded_bytes = 0;
    songDownloading.thread_id = thread_id;

    if (songsDownloading == NULL) {
        songsDownloading = malloc(sizeof(Song_Downloading));
        if (songsDownloading == NULL) {
            printF(RED, "Error al asignar memoria\n");
            return;
        }
        num_songs_downloading = 1;
    } else {
        Song_Downloading *temp = realloc(songsDownloading, sizeof(Song_Downloading) * (num_songs_downloading + 1));
        if (temp == NULL) {
            printF(RED, "Error al realocar memoria\n");
            return;
        }
        songsDownloading = temp;
        num_songs_downloading++;
    }
    songsDownloading[num_songs_downloading - 1] = songDownloading;
}

void deleteSongFromArray(Song song) {
    int index = -1;
    for (int i = 0; i < num_songs_downloading; i++) {
        if (songsDownloading[i].song.id == song.id) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        printF(RED, "Error al eliminar la canción del array\n");
        return;
    }

    // Liberar la memoria dinámica de la canción que se va a eliminar
    free(songsDownloading[index].song.fileName);
    free(songsDownloading[index].song.md5sum);

    // Reorganizar el arreglo
    for (int i = index; i < num_songs_downloading - 1; i++) {
        songsDownloading[i] = songsDownloading[i + 1];
    }

    num_songs_downloading--;

    if (num_songs_downloading == 0) {
        free(songsDownloading);
        songsDownloading = NULL;
    } else {
        Song_Downloading *temp = realloc(songsDownloading, sizeof(Song_Downloading) * num_songs_downloading);
        if (temp == NULL) {
            printF(RED, "Error al realocar memoria\n");
            return;
        }
        songsDownloading = temp;
    }
}

void freeSongDownloadingArray() {
    if (songsDownloading != NULL) {
        for (int i = 0; i < num_songs_downloading; i++) {
            free(songsDownloading[i].song.fileName);
            free(songsDownloading[i].song.md5sum);
            if (songsDownloading[i].downloaded_bytes < songsDownloading[i].song_size) {
                pthread_join(songsDownloading[i].thread_id, NULL);
            }
        }
        free(songsDownloading);
        songsDownloading = NULL;
        num_songs_downloading = 0;
    }
}

void clearDownloadedSongs(){
    printF(GREEN, "Limpiando canciones descargadas\n");
    for (int i = 0; i < num_songs_downloading; i++) {
        if (songsDownloading[i].downloaded_bytes == songsDownloading[i].song_size) {
            printF(GREEN, "\t -%s eliminada de la lista\n", songsDownloading[i].song.fileName);
            deleteSongFromArray(songsDownloading[i].song);
            i--;
        }
    }
}

void* startSongDownload(void* args) {
    char* str = (char*)args; 

    Song newSong;
    parseSongInfo(str, &newSong); 

    int fd = open(newSong.fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        printF(RED, "Error opening file\n");
        pthread_exit(NULL);
    }

    Message_buffer *msg = malloc(sizeof(Message_buffer));

    msg->msg_type = (long)newSong.id;
    long total_bytes_written = 0;
    ssize_t max_write_size = HEADER_MAX_SIZE - strlen("FILE_DATA") - 3 - 1; // 3 bytes para el ID + 1 byte para el '&'

    addSongToArray(newSong, pthread_self());
    
    while (total_bytes_written < newSong.fileSize && !bowman_sigint_received) {
        msg_queue_reader(msg_id, msg);

        // Calcular cuántos bytes se deben escribir en esta iteración
        size_t bytes_to_write = (total_bytes_written + max_write_size <= newSong.fileSize) ? max_write_size : (newSong.fileSize - total_bytes_written);
        //size_t bytes_to_write = sizeof(msg->msg_text);

        int written = write(fd, msg->msg_text, bytes_to_write);
        if (written < 0) {
            printF(RED, "Error writing to file\n");
            break;
        }

        total_bytes_written += written;

        // Actualizar el número de bytes descargados
        for (int i = 0; i < num_songs_downloading; i++) {
            if (songsDownloading[i].song.id == newSong.id) {
                songsDownloading[i].downloaded_bytes = total_bytes_written;
                break;
            }
        }
        
    }

    //printF(GREEN, "\nDescarga completada\n");
    close(fd);
    free(msg);
    free(str);

    pthread_exit(NULL);
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

void printSongDownloading(Song_Downloading songDownloading) {

    //todo conseguir porcentaje
    float percentage = (float)songDownloading.downloaded_bytes / (float)songDownloading.song_size * 100;
    //crear barra de progreso
    char bar[101];
    memset(bar, 0, sizeof(bar));
    int i;
    for (i = 0; i < percentage; i++) {
        bar[i] = '=';
    }
    bar[i++] = '%';
    for (; i < 100; i++) {
        bar[i] = ' ';
    }
    bar[100] = '\0';

    printF(GREEN, "%s [%s] %.2f%%\n", songDownloading.song.fileName, bar, percentage);

}

void printAllSongsDownloading() {

    if (songsDownloading == NULL || num_songs_downloading == 0) {
        printF(RED,"No hay canciones descargándose actualmente.\n");
        return;
    }

    printF(GREEN,"Canciones descargándose:\n");
    for (int i = 0; i < num_songs_downloading; i++) {
        printSongDownloading(songsDownloading[i]);
    }
}

int connect_to_another_server(char *username, char *discovery_ip, int port, int *psocket){

    discovery_socket = connect_to_discovery(username, discovery_ip, port);
    if (discovery_socket == EXIT_FAILURE) {
        printF(RED, "No sockets availabe\n");
        return 0;
    }
    
    connect_to_server(psocket);
    if (*psocket == EXIT_FAILURE) {
        printF(RED, "Failed to connect to Poole server\n");
        return 0;
    }

    Frame response_frame = frame_creator(0x01, "NEW_BOWMAN", username);
    if (send_frame(poole_socket, &response_frame) < 0) {
        printF(RED, "Failed to send NEW_BOWMAN frame\n");
        close(poole_socket);
        return 0;
    }

    Frame request_frame;
    if (receive_frame(poole_socket, &request_frame) <= 0) {
        printF(RED, "Error receiving frame from Bowman. Connection aborted.\n");
        close(poole_socket);
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
    pargs = malloc (sizeof(thread_receive_frames));
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