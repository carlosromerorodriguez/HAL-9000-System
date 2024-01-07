#include "display_response_handler.h"

// Variables globales

// Variables globales importadas de otros TADS
extern int poole_socket_for_bowman;
extern int msg_id;

// Funciones importadas de otros TADS


/**
 * @brief Crea una trama LIST_SONGS y la envía al servidor Poole
 * 
 * @return void
*/
void list_songs() {
    Frame list_songs_frame = frame_creator(0x02, "LIST_SONGS", "");

    // Enviar la trama al servidor Poole
    if (send_frame(poole_socket_for_bowman, &list_songs_frame) < 0) {
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

/**
 * @brief Crea una trama LIST_PLAYLISTS y la envía al servidor Poole
 * 
 * @return void
*/
void list_playlists() {
    Frame playlists_frame = frame_creator(0x02, "LIST_PLAYLISTS", "");

    // Enviar la trama al servidor Poole
    if (send_frame(poole_socket_for_bowman, &playlists_frame) < 0) {
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

void printSongDownloading(Song_Downloading songDownloading) {
    // conseguir porcentaje
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