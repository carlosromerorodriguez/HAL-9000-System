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
char* searchSong(const char *basePath, const char *songName);   
long getFileSize(const char *filePath);
void* send_song(void * args);
void empezar_envio(thread_args t_args,int id, char* path, long file_size);

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
        //DOWNLOAD SONG
        else if(strncmp(request_frame.header_plus_data, "DOWNLOAD_SONG", request_frame.header_length) == 0) {
            printF(YELLOW,"\nNew request - %s wants to download %s\n", t_args->username, request_frame.header_plus_data + request_frame.header_length);
            printF(YELLOW,"Sending %s to %s\n", t_args->username, request_frame.header_plus_data + request_frame.header_length);

            //Lanzar thread que gestione la descarga
            t_args->song_name = request_frame.header_plus_data + request_frame.header_length;
            if (pthread_create(&t_args->download_song_thread, NULL, send_song, (void *)t_args) != 0) {
                printF(RED, "Error creating thread to list playlists.\n");
                return NULL;
            } else {
                pthread_join(t_args->download_song_thread, NULL);
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

    //MANDAR EL TAMAÑO DE LA LISTA DE CANCIONES AL CLIENTE
    printF(YELLOW, "List length: %d\n", list_length);

    char list_length_str[HEADER_MAX_SIZE - strlen("LIST_SONGS_SIZE")];
    memset(list_length_str, 0, HEADER_MAX_SIZE - strlen("LIST_SONGS_SIZE"));
    snprintf(list_length_str, sizeof(list_length_str), "%d", list_length);

    Frame list_length_frame = frame_creator(0x02, "LIST_SONGS_SIZE", list_length_str);
    if (send_frame(t_args->client_socket, &list_length_frame) < 0) {
        printF(RED, "Error sending list length frame to Bowman.\n");
        free(songs_list);
        return NULL;
    }

    //Empezar a mandar la lista de canciones (HEADER = SONGS_RESPONSE)
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

    //MANDAR EL TAMAÑO DE LA LISTA DE CANCIONES AL CLIENTE
    printF(YELLOW, "Playlist length: %d\n", list_length);

    char list_length_str[HEADER_MAX_SIZE - strlen("PLAYLISTS_SONGS_SIZE")];
    memset(list_length_str, 0, HEADER_MAX_SIZE - strlen("PLAYLISTS_SONGS_SIZE"));
    snprintf(list_length_str, sizeof(list_length_str), "%d", list_length);

    Frame list_length_frame = frame_creator(0x02, "PLAYLISTS_SONGS_SIZE", list_length_str);
    if (send_frame(t_args->client_socket, &list_length_frame) < 0) {
        printF(RED, "Error sending list length frame to Bowman.\n");
        free(playlist_list);
        return NULL;
    }

    usleep(100000); // 100ms

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

char* searchSong(const char *basePath, const char *songName) {
    struct dirent *dp;
    DIR *dir = opendir(basePath);
    char *path = NULL;

    // Unable to open directory
    if (!dir) {
        return NULL;
    }

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            char newPath[1000];
            struct stat statbuf;

            // Construct new path from our base path
            snprintf(newPath, sizeof(newPath), "%s/%s", basePath, dp->d_name);
            stat(newPath, &statbuf);

            if (S_ISDIR(statbuf.st_mode)) {
                // If entry is a directory, recursively search it
                path = searchSong(newPath, songName);
            } else {
                // If entry is a file, check if it's the song
                if (strcmp(dp->d_name, songName) == 0) {
                    path = strdup(newPath);
                    break;
                }
            }
        }

        if (path) break;
    }

    closedir(dir);
    return path;
}

char* getFileMD5(char *filePath) {
    int pipefd[2];
    pid_t pid;
    char *md5sum = malloc(33);

    if (md5sum == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    if (pipe(pipefd) == -1) {
        perror("pipe");
        free(md5sum);
        return NULL;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        free(md5sum);
        return NULL;
    }

    if (pid == 0) { 
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]); 

        execlp("md5sum", "md5sum", filePath, NULL);
        _exit(EXIT_FAILURE);
    } else {
        close(pipefd[1]);

        read(pipefd[0], md5sum, 32);
        md5sum[32] = '\0';

        close(pipefd[0]); 
        wait(NULL);
    }

    return md5sum;
}

long getFileSize(const char *filePath) {
    struct stat fileStat;

    // Intenta obtenir informació sobre el fitxer
    if (stat(filePath, &fileStat) == 0) {
        // Retorna la mida del fitxer
        return (long) fileStat.st_size;
    } else {
        // En cas d'error, retorna -1 (pots canviar a una altra gestió d'errors segons les teves necessitats)
        perror("Error obtenint informació del fitxer");
        return -1;
    }
}

void* send_song(void * args){

    thread_args *t_args = (thread_args *)args;

    printF(GREEN, "Thread created -> sending %s\n", t_args->song_name);

    int newPathLength = strlen("..") + strlen(t_args->server_directory) + 1;
    char *newPath = malloc(newPathLength);

    strcpy(newPath, "..");
    strcat(newPath, t_args->server_directory);

    char *path = searchSong(newPath, t_args->song_name);
    if (path) {
        printF(GREEN,"Found: %s\n", path);
    } else {
        printF(RED,"Song not found.\n");
        return NULL;
    }

    char *md5sum = getFileMD5(path);
    long file_size = getFileSize(path);
    char *file_name = t_args->song_name;
    int id = ftok(path, 1);

    int length = snprintf(NULL, 0, "%s&%lu&%s&%d", file_name, file_size, md5sum, id);
    char *data = malloc(length + 1);
    sprintf(data, "%s&%lu&%s&%d", file_name, file_size, md5sum, id);

    Frame new_song = frame_creator(0x04, "NEW_FILE", data);

    if (send_frame(t_args->client_socket, &new_song) < 0) {
        printF(RED, "Error sending NEW_FILE frame to %s.\n", t_args->username);
        return NULL;
    } 

    empezar_envio(*t_args, id, path, file_size);    

    return NULL;
}

void empezar_envio(thread_args t_args, int id, char* path, long file_size) {
    char *id_char = intToStr(id);
    int id_length = strlen(id_char);
    int max_buffer_size = 253 - 9 - 1 - id_length; 

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printF(RED, "Error opening file\n");
        return;
    }

    long total_bytes_sent = 0;

    while (total_bytes_sent < file_size) {
        long current_buffer_size = (file_size - total_bytes_sent > max_buffer_size) ? max_buffer_size : (file_size - total_bytes_sent);
        char *buffer = malloc(sizeof(char) * current_buffer_size);

        int bytes_read = read(fd, buffer, current_buffer_size);
        if (bytes_read < 0) {
            printF(RED, "Error reading file\n");
            free(buffer);
            close(fd);
            return;
        }

        char *data = malloc(sizeof(char) * (id_length + sizeof(char) + bytes_read));
        memcpy(data, id_char, id_length);
        data[id_length] = '&';
        memcpy(data + id_length + 1, buffer, bytes_read);

        Frame file_data = frame_creator(0x04, "FILE_DATA", data); 

        if (send_frame(t_args.client_socket, &file_data) < 0) {
            printF(RED, "Error sending FILE_DATA frame to %s.\n", t_args.username);
            free(data);
            free(buffer);
            close(fd);
            return;
        }

        total_bytes_sent += bytes_read;
        printF(RED, "%ld / %ld\n", total_bytes_sent, file_size);
        free(data);
        free(buffer);
    }

    printF(GREEN, "File completed!\n");
    close(fd);
}

