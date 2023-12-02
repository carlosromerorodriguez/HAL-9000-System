#include "command_handler.h"

extern volatile sig_atomic_t server_sigint_received;

//LIST SONGS FUNCTIONS
void append_song(char **songs_list, char *song_name);
void get_mp3_files_recursive(char *directory, char **songs_list);
char* get_mp3_files(char *directory);
char* get_songs_list(thread_args *t_args);
void* list_songs_handler(void* args);

//LIST PLAYLISTS FUNCTIONS
void append_playlist(char **playlists_list, char *playlist_name, char separator);
void get_playlists_and_songs_files_recursive(char *directory, char **playlists_list);
char* get_playlists_and_songs(char *directory);
char *get_playlists_list(thread_args *t_args);
void* list_playlists_handler(void* args);

/**
 * @brief Maneja la conexion con Bowman y el envio de comandos
 * 
 * @param args Estructura con los argumentos necesarios para la conexion
 * 
 * @return NULL
*/
void *client_handler(void* args) {
    thread_args *t_args = (thread_args*)args;

    if (t_args == NULL) {
        printF(RED, "Error receiving thread arguments.\n");
        return NULL;
    }

    while (!server_sigint_received) {
        fd_set read_fds;
        struct timeval tv;

        FD_ZERO(&read_fds);
        FD_SET(t_args->client_socket, &read_fds);

        // Set timeout to 1 second
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int select_result = select(t_args->client_socket + 1, &read_fds, NULL, NULL, &tv);

        if (select_result < 0) {
            printF(RED, "Select error in client handler.\n");
            break;
        } else if (select_result == 0) {
            continue;
        }

        Frame request_frame;
        if (receive_frame(t_args->client_socket, &request_frame) <= 0) {
            printF(RED, "Error receiving frame from Bowman. Connection aborted.\n");
            return NULL;
        }
        // LIST_SONGS
        if (strncmp(request_frame.header_plus_data, "LIST_SONGS", request_frame.header_length) == 0) {
            printF(YELLOW,"\nNew request - %s requires the list of songs\n", t_args->username);
            printF(YELLOW,"Sending songs list to %s\n", t_args->username);

            //Lanzar thread que gestione la lista de canciones
            if (pthread_create(&t_args->list_songs_thread, NULL, list_songs_handler, (void *)t_args) != 0) {
                printF(RED, "Error creating thread to list songs.\n");
                return NULL;
            } else {
                pthread_join(t_args->list_songs_thread, NULL);
            }
        }
        // LIST PLAYLISTS
        else if(strncmp(request_frame.header_plus_data, "LIST_PLAYLISTS", request_frame.header_length) == 0) {
            printF(YELLOW,"\nNew request - %s requires the list of playlists\n", t_args->username);
            printF(YELLOW,"Sending playlists list to %s\n", t_args->username);

            //Lanzar thread que gestione la lista de playlists
            if (pthread_create(&t_args->list_playlists_thread, NULL, list_playlists_handler, (void *)t_args) != 0) {
                printF(RED, "Error creating thread to list playlists.\n");
                return NULL;
            } else {
                pthread_join(t_args->list_songs_thread, NULL);
            }
        }
        //USER LOGOUT
        else if (strncmp(request_frame.header_plus_data, "EXIT", request_frame.header_length) == 0) {
            Frame response_frame;

            if (strlen(request_frame.header_plus_data + request_frame.header_length) > 0) {
                printF(GREEN, "\nUser %s is disconnecting\n", request_frame.header_plus_data + request_frame.header_length);
                display_loading_spinner(GREEN, 3);
                response_frame = frame_creator(0x06, "LOGOUT_OK", "");
            } else {
                response_frame = frame_creator(0x06, "LOGOUT_KO", "");
            }

            if (send_frame(t_args->client_socket, &response_frame) < 0) {
                printF(RED, "Error sending response frame to Bowman.\n");
                return NULL;
            }

            return NULL;
        }
    }
    return NULL;
}

//LIST SONGS FUNCTIONS
void append_song(char **songs_list, char *song_name) {
    *songs_list = realloc(*songs_list, strlen(*songs_list) + strlen(song_name) + 2);
    if (*songs_list == NULL) {
        printF(RED, "Error reallocating memory for songs list.\n");
        return;
    }
    strcat(*songs_list, song_name);
    strcat(*songs_list, "&");
}

void get_mp3_files_recursive(char *directory, char **songs_list) {
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(directory)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_DIR) {
                // Ignora los directorios '.' y '..' para ir directamente a los archivos mp3
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                    char path[1024];
                    snprintf(path, sizeof(path), "%s/%s", directory, ent->d_name);
                    get_mp3_files_recursive(path, songs_list);
                }
            } else if (strstr(ent->d_name, ".mp3") != NULL) {
                append_song(songs_list, ent->d_name);
            }
        }
        closedir(dir);
    } else {
        printF(RED, "Error opening directory.\n");
    }
}

char* get_mp3_files(char *directory) {
    char *songs_list = malloc(1);
    if (songs_list == NULL) {
        printF(RED, "Error allocating memory for songs list.\n");
        return NULL;
    }
    songs_list[0] = '\0';

    get_mp3_files_recursive(directory, &songs_list);

    return songs_list;
}

char* get_songs_list(thread_args *t_args) {
    char *new_directory = malloc(strlen(t_args->server_directory) + 3);
    if (new_directory == NULL) {
        printF(RED, "Error allocating memory for new directory.\n");
        return NULL;
    }

    snprintf(new_directory, strlen(t_args->server_directory) + 3, "..%s", t_args->server_directory);
    
    char *songs_list = get_mp3_files(new_directory);
    
    free(new_directory);
    return songs_list;
}

void* list_songs_handler(void* args) {
    thread_args *t_args = (thread_args*)args;

    if (t_args == NULL) {
        printF(RED, "Error receiving thread arguments.\n");
        return NULL;
    }

    char *songs_list = get_songs_list(t_args);
    if (songs_list == NULL) {
        printF(RED, "Error getting songs list.\n");
        return NULL;
    }

    // Asegurar suficiente espacio para la concatenación
    int extra_data_length = strlen("HSGSAGSAHSAGSAGSASAHASGHSAGHSAGHSAHGASHGHASGASGHJASGHJASGHAGHAGHSGHASGHASASGHAGHSAHSGASHAHSGASGHASHGASGHSAGHSAGHAHSGSHAGDHGHAGHGDHGASASHGASHJGSAHJAGSHJSAGJASGHSAGASGASGSAHSGAGSASAGHSAGSAGHSAHSGAS") + 1;
    songs_list = realloc(songs_list, strlen(songs_list) + extra_data_length);
    if (songs_list == NULL) {
        printF(RED, "Error reallocating memory for songs list.\n");
        return NULL;
    }
    strcat(songs_list, "HSGSAGSAHSAGSAGSASAHASGHSAGHSAGHSAHGASHGHASGASGHJASGHJASGHAGHAGHSGHASGHASASGHAGHSAHSGASHAHSGASGHASHGASGHSAGHSAGHAHSGSHAGDHGHAGHGDHGASASHGASHJGSAHJAGSHJSAGJASGHSAGASGASGSAHSGAGSASAGHSAGSAGHSAHSGAS");

    int list_length = strlen(songs_list);
    int offset = 0;
    char data_segment[HEADER_MAX_SIZE]; // 253 bytes

    while (offset < list_length) {
        int data_length = (list_length - offset > HEADER_MAX_SIZE - 1) ? HEADER_MAX_SIZE - 1 : list_length - offset;
        memset(data_segment, 0, HEADER_MAX_SIZE);
        strncpy(data_segment, songs_list + offset, data_length);
        data_segment[data_length] = '\0'; // Asegura que el segmento de datos esté correctamente terminado

        Frame response_frame = frame_creator(0x02, "SONGS_RESPONSE", data_segment);
        if (send_frame(t_args->client_socket, &response_frame) < 0) {
            printF(RED, "Error sending response frame to Bowman.\n");
            free(songs_list);
            return NULL;
        }
        offset += data_length;
    }

    free(songs_list);
    pthread_exit(NULL);
}

//LIST PLAYLISTS FUNCTIONS
void append_playlist(char **playlists_list, char *playlist_name, char separator) {
    size_t new_size = strlen(*playlists_list) + strlen(playlist_name) + 2; // 1 para el separador, 1 para el '\0'
    char *temp = realloc(*playlists_list, new_size);
    if (temp == NULL) {
        printF(RED, "Error reallocating memory for playlists list.\n");
        return;
    }
    *playlists_list = temp;
    strcat(*playlists_list, playlist_name);
    strncat(*playlists_list, &separator, 1);
}

void get_playlists_and_songs_files_recursive(char *directory, char **playlists_list) {
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(directory)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_DIR) {
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                    char path[1024];
                    snprintf(path, sizeof(path), "%s/%s", directory, ent->d_name);
                    append_playlist(playlists_list, ent->d_name, '#'); // Añadir el separador entre playlists '#'
                    get_playlists_and_songs_files_recursive(path, playlists_list);
                }
            } else if (strstr(ent->d_name, ".mp3") != NULL) {
                append_playlist(playlists_list, ent->d_name, '&'); // Añadir el separador entre canciones '&'
            }
        }
        closedir(dir);
    } else {
        printF(RED, "Error opening directory.\n");
    }
}

char* get_playlists_and_songs(char *directory) {
    char *playlist_list = malloc(1);
    if (playlist_list == NULL) {
        printF(RED, "Error allocating memory for playlist list.\n");
        return NULL;
    }
    playlist_list[0] = '\0';

    get_playlists_and_songs_files_recursive(directory, &playlist_list);

    size_t list_len = strlen(playlist_list);
    if (list_len > 0 && (playlist_list[list_len - 1] == '&' || playlist_list[list_len - 1] == '#')) {
        playlist_list[list_len - 1] = '\0';
    }

    return playlist_list;
}

char *get_playlists_list(thread_args *t_args) {
    char *new_directory = malloc(strlen(t_args->server_directory) + 3);
    if (new_directory == NULL) {
        printF(RED, "Error allocating memory for new directory.\n");
        return NULL;
    }

    snprintf(new_directory, strlen(t_args->server_directory) + 3, "..%s", t_args->server_directory);

    char *playlists_list = get_playlists_and_songs(new_directory);

    free(new_directory);
    return playlists_list;
}

void* list_playlists_handler(void* args) {
    thread_args *t_args = (thread_args*)args;

    if (t_args == NULL) {
        printF(RED, "Error receiving thread arguments.\n");
        return NULL;
    }

    char *playlist_list = get_playlists_list(t_args);
    if (playlist_list == NULL) {
        printF(RED, "Error getting songs list.\n");
        return NULL;
    }

    // Asegurar suficiente espacio para la concatenación
    int extra_data_length = strlen("HSGSAGSAHSAGSAGSASAHASGHSAGHSAGHSAHGASHGHASGASGHJASGHJASGHAGHAGHSGHASGHASASGHAGHSAHSGASHAHSGASGHASHGASGHSAGHSAGHAHSGSHAGDHGHAGHGDHGASASHGASHJGSAHJAGSHJSAGJASGHSAGASGASGSAHSGAGSASAGHSAGSAGHSAHSGAS") + 1;
    playlist_list = realloc(playlist_list, strlen(playlist_list) + extra_data_length);
    if (playlist_list == NULL) {
        printF(RED, "Error reallocating memory for songs list.\n");
        return NULL;
    }
    strcat(playlist_list, "HSGSAGSAHSAGSAGSASAHASGHSAGHSAGHSAHGASHGHASGASGHJASGHJASGHAGHAGHSGHASGHASASGHAGHSAHSGASHAHSGASGHASHGASGHSAGHSAGHAHSGSHAGDHGHAGHGDHGASASHGASHJGSAHJAGSHJSAGJASGHSAGASGASGSAHSGAGSASAGHSAGSAGHSAHSGAS");

    int list_length = strlen(playlist_list);
    int offset = 0;
    char data_segment[HEADER_MAX_SIZE]; // 253 bytes

    while (offset < list_length) {
        int data_length = (list_length - offset > HEADER_MAX_SIZE - 1) ? HEADER_MAX_SIZE - 1 : list_length - offset;
        memset(data_segment, 0, HEADER_MAX_SIZE);
        strncpy(data_segment, playlist_list + offset, data_length);
        data_segment[data_length] = '\0'; // Asegura que el segmento de datos esté correctamente terminado

        Frame response_frame = frame_creator(0x02, "PLAYLISTS_RESPONSE", data_segment);
        if (send_frame(t_args->client_socket, &response_frame) < 0) {
            printF(RED, "Error sending response frame to Bowman.\n");
            free(playlist_list);
            return NULL;
        }
        offset += data_length;
    }

    free(playlist_list);
    pthread_exit(NULL);
}