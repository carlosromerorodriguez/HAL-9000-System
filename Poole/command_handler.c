#include "command_handler.h"

extern volatile sig_atomic_t server_sigint_received;
pthread_mutex_t send_frame_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_pipe_mutex = PTHREAD_MUTEX_INITIALIZER;
extern int global_write_pipe;
extern volatile sig_atomic_t poole_sigint_received;

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
char* searchFolder(const char *basePath, const char *folderName); 
long getFileSize(const char *filePath);
void* send_song(void * args);
void* send_list(void * args);
void empezar_envio(thread_args t_args, int id, char* path, long file_size);
void server_shutdown(int client_socket);

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
            }else{
                pthread_detach(t_args->list_songs_thread);
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
            }else{
                pthread_detach(t_args->list_playlists_thread);
            }  
        }
        //DOWNLOAD SONG
        else if(strncmp(request_frame.header_plus_data, "DOWNLOAD_SONG", request_frame.header_length) == 0) {
            printF(YELLOW,"\nNew request - %s wants to download %s\n", t_args->username, request_frame.header_plus_data + request_frame.header_length);
            printF(YELLOW,"Sending %s to %s\n",  request_frame.header_plus_data + request_frame.header_length, t_args->username);

            //Lanzar thread que gestione la descarga
            t_args->song_name = request_frame.header_plus_data + request_frame.header_length;
            t_args->is_song = 1;
            //Crear thread 
            if (pthread_create(&t_args->download_song_thread, NULL, send_song, (void *)t_args) != 0) {
                printF(RED, "Error creating thread to download song\n");
                return NULL;
            }
            else{
                pthread_detach(t_args->download_song_thread);
            }
        }
        else if(strncmp(request_frame.header_plus_data, "DOWNLOAD_LIST", request_frame.header_length) == 0) {
            printF(YELLOW,"\nNew request - %s wants to download the playlist %s\n", t_args->username, request_frame.header_plus_data + request_frame.header_length);
            printF(YELLOW,"Sending %s to %s\n", request_frame.header_plus_data + request_frame.header_length,  t_args->username);

            //Lanzar thread que gestione la descarga
            t_args->list_name = request_frame.header_plus_data + request_frame.header_length;
            t_args->is_song = 0;
            //Crear thread 
            if (pthread_create(&t_args->download_playlist_thread, NULL, send_list, (void *)t_args) != 0) {
                printF(RED, "Error creating thread to download playlist.\n");
                return NULL;
            }else{
                pthread_detach(t_args->download_playlist_thread);
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

            pthread_mutex_lock(&send_frame_mutex);
            if (send_frame(t_args->client_socket, &response_frame) < 0) {
                printF(RED, "Error sending response frame to Bowman.\n");
                pthread_mutex_unlock(&send_frame_mutex);
                return NULL;
            }
            pthread_mutex_unlock(&send_frame_mutex);

            return NULL;
        }
    }

    server_shutdown(t_args->client_socket);

    return NULL;
}

/**
 * @brief Añade una canción a la lista de canciones existente.
 * 
 * Realoca el espacio de memoria para incluir la nueva canción y un delimitador '&'.
 * En caso de error en la realocación, muestra un mensaje y termina la función.
 * 
 * @param songs_list Puntero doble a la lista de canciones.
 * @param song_name Nombre de la canción a añadir.
 * 
 * @return void
 */
void append_song(char **songs_list, char *song_name) {
    *songs_list = realloc(*songs_list, strlen(*songs_list) + strlen(song_name) + 2);
    if (*songs_list == NULL) {
        printF(RED, "Error reallocating memory for songs list.\n");
        return;
    }
    strcat(*songs_list, song_name);
    strcat(*songs_list, "&");
}


/**
 * @brief Recorre recursivamente un directorio para buscar y añadir archivos MP3 a la lista de canciones.
 * 
 * Esta función abre el directorio especificado y busca archivos MP3. Si encuentra un subdirectorio,
 * realiza una llamada recursiva para buscar archivos MP3 dentro de él. Los archivos MP3 encontrados
 * se añaden a la lista de canciones mediante la función 'append_song'.
 * 
 * @param directory El directorio inicial para comenzar la búsqueda.
 * @param songs_list Puntero doble a la lista de canciones donde se añadirán los nombres de los archivos MP3.
 * 
 * @return void
 */
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

/**
 * @brief Obtiene la lista de canciones del servidor.
 * 
 * Esta función crea una nueva ruta para el directorio de canciones y llama a la función
 * 'get_mp3_files_recursive' para obtener la lista de canciones.
 * 
 * @param directory El directorio inicial para comenzar la búsqueda.
 * 
 * @return char* La lista de canciones.
 */
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

/**
 * @brief Obtiene la lista de canciones del servidor.
 * 
 * Esta función crea una nueva ruta para el directorio de canciones y llama a la función
 * 'get_mp3_files_recursive' para obtener la lista de canciones.
 * 
 * @param t_args Estructura con los argumentos necesarios para la conexion
 * 
 * @return char* La lista de canciones.
 */
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

/**
 * @brief Thread que maneja la petición de listado de canciones.
 * 
 * Esta función obtiene la lista de canciones del servidor y la envía a Bowman.
 * 
 * @param args Estructura con los argumentos necesarios para la conexion
 * 
 * @return void
 */
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

    int list_length = strlen(songs_list);
    int offset = 0;
    char data_segment[HEADER_MAX_SIZE]; // 253 bytes

    char list_length_str[HEADER_MAX_SIZE - strlen("LIST_SONGS_SIZE")];
    memset(list_length_str, 0, HEADER_MAX_SIZE - strlen("LIST_SONGS_SIZE"));
    snprintf(list_length_str, sizeof(list_length_str), "%d", list_length);

    Frame list_length_frame = frame_creator(0x02, "LIST_SONGS_SIZE", list_length_str);

    pthread_mutex_lock(&send_frame_mutex);
    if (send_frame(t_args->client_socket, &list_length_frame) < 0) {
        printF(RED, "Error sending list length frame to Bowman.\n");
        free(songs_list);
        pthread_mutex_unlock(&send_frame_mutex);
        return NULL;
    }
    pthread_mutex_unlock(&send_frame_mutex);

    //Empezar a mandar la lista de canciones (HEADER = SONGS_RESPONSE)
    while (offset < list_length) {
        int data_length = (list_length - offset > HEADER_MAX_SIZE - 1) ? HEADER_MAX_SIZE - 1 : list_length - offset;
        memset(data_segment, 0, HEADER_MAX_SIZE);
        strncpy(data_segment, songs_list + offset, data_length);
        data_segment[data_length] = '\0'; // Asegura que el segmento de datos esté correctamente terminado

        Frame response_frame = frame_creator(0x02, "SONGS_RESPONSE", data_segment);
        //printF(YELLOW, "Sending frame: %s\n", response_frame.header_plus_data);
        pthread_mutex_lock(&send_frame_mutex);
        if (send_frame(t_args->client_socket, &response_frame) < 0) {
            printF(RED, "Error sending response frame to Bowman.\n");
            free(songs_list);
            pthread_mutex_unlock(&send_frame_mutex);
            return NULL;
        }
        pthread_mutex_unlock(&send_frame_mutex);
        offset += data_length;
    }

    free(songs_list);
    pthread_exit(NULL);
}

/**
 * @brief Añade un nombre de playlist a una lista de playlists, separando los nombres con un carácter específico.
 * 
 * Realoca memoria para la lista de playlists para acomodar el nuevo nombre, incluyendo espacio para el separador
 * y el carácter nulo final. Añade el separador y luego el nombre de la nueva playlist a la lista.
 * 
 * @param playlists_list Puntero doble a la lista de playlists existente.
 * @param playlist_name Nombre de la playlist a añadir.
 * @param separator Carácter utilizado para separar los nombres de las playlists en la lista.
 * 
 */
void append_playlist(char **playlists_list, char *playlist_name, char separator) {
    size_t new_size = strlen(*playlists_list) + strlen(playlist_name) + 2; // 1 para el separador, 1 para el '\0'
    char *temp = realloc(*playlists_list, new_size);
    if (temp == NULL) {
        printF(RED, "Error reallocating memory for playlists list.\n");
        return;
    }
    *playlists_list = temp;
    strncat(*playlists_list, &separator, 1);
    strcat(*playlists_list, playlist_name);
}

/**
 * @brief Recorre recursivamente un directorio para añadir nombres de playlists y archivos MP3 a una lista.
 * 
 * Abre un directorio especificado y busca tanto subdirectorios (playlists) como archivos MP3. 
 * Añade los nombres de los subdirectorios y archivos MP3 a la lista de playlists, utilizando separadores 
 * específicos ('#' para playlists y '&' para canciones). Realiza llamadas recursivas para buscar en 
 * subdirectorios.
 * 
 * @param directory El directorio inicial para comenzar la búsqueda.
 * @param playlists_list Puntero doble a la lista donde se añadirán los nombres de las playlists y canciones.
 * 
 * @return void
 */
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

/**
 * @brief Crea y devuelve una lista de nombres de playlists y canciones encontradas en un directorio.
 * 
 * Inicializa una lista de playlists y realiza una búsqueda recursiva en el directorio especificado
 * para añadir los nombres de playlists y archivos MP3 a esta lista. Elimina el último separador 
 * de la lista (si existe) antes de devolverla.
 * 
 * @param directory El directorio desde el cual comenzar la búsqueda de playlists y canciones.
 * @return Puntero a la lista de playlists y canciones, o NULL si falla la asignación de memoria.
 * 
 */
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

/**
 * @brief Obtiene una lista de playlists y canciones de un directorio relacionado con el servidor.
 * 
 * Esta función construye una nueva ruta de directorio a partir del directorio del servidor proporcionado
 * en los argumentos del hilo, y luego llama a 'get_playlists_and_songs' para obtener la lista de playlists 
 * y canciones de ese directorio. La nueva ruta de directorio se crea añadiendo ".." al inicio del directorio 
 * del servidor para referenciar el directorio padre.
 * 
 * @param t_args Argumentos del hilo que contienen el directorio del servidor.
 * @return Puntero a la lista de playlists y canciones, o NULL si falla la asignación de memoria.
 * 
 * @note La memoria para el directorio nuevo se libera antes de retornar la lista de playlists.
 */
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

/**
 * @brief Maneja la solicitud de listado de playlists y envía la lista a través de un socket.
 * 
 * Esta función se encarga de procesar los argumentos del hilo para obtener la lista de playlists 
 * y canciones. Luego, envía esta lista en segmentos a través de un socket al cliente Bowman. 
 * La lista se envía en múltiples marcos ('frames'), comenzando con el tamaño total de la lista y 
 * seguido por los segmentos de la lista.
 * 
 * @param args Argumentos del hilo, que incluyen información del socket del cliente y el directorio del servidor.
 * @return NULL siempre, para cumplir con la firma de una función de hilo.
 * 
 * @note La función utiliza un mutex para sincronizar el envío de datos a través del socket.
 *       Si se produce algún error durante el proceso, se libera la memoria asignada y se envía 
 *       un mensaje de error antes de salir.
 */

void* list_playlists_handler(void* args) {
    thread_args *t_args = (thread_args*)args;

    if (t_args == NULL) {
        printF(RED, "Error receiving thread arguments.\n");
        return NULL;
    }

    char *playlists_list = get_playlists_list(t_args);
    if (playlists_list == NULL) {
        printF(RED, "Error getting songs list.\n");
        return NULL;
    }
    int list_length = strlen(playlists_list);
    int offset = 0;
    char data_segment[HEADER_MAX_SIZE]; // 253 bytes

    char list_length_str[HEADER_MAX_SIZE - strlen("LIST_PLAYLISTS_SIZE")];
    memset(list_length_str, 0, HEADER_MAX_SIZE - strlen("LIST_PLAYLISTS_SIZE"));
    snprintf(list_length_str, sizeof(list_length_str), "%d", list_length);

    Frame list_length_frame = frame_creator(0x02, "LIST_PLAYLISTS_SIZE", list_length_str);
    pthread_mutex_lock(&send_frame_mutex);
    if (send_frame(t_args->client_socket, &list_length_frame) < 0) {
        printF(RED, "Error sending list length frame to Bowman.\n");
        free(playlists_list);
        pthread_mutex_unlock(&send_frame_mutex);
        return NULL;
    }
    pthread_mutex_unlock(&send_frame_mutex);

    //Empezar a mandar la lista de canciones (HEADER = PLAYLISTS_RESPONSE)
    while (offset < list_length) {
        int data_length = (list_length - offset > HEADER_MAX_SIZE - 1) ? HEADER_MAX_SIZE - 1 : list_length - offset;
        memset(data_segment, 0, HEADER_MAX_SIZE);
        strncpy(data_segment, playlists_list + offset, data_length);
        data_segment[data_length] = '\0'; // Asegura que el segmento de datos esté correctamente terminado

        Frame response_frame = frame_creator(0x02, "PLAYLISTS_RESPONSE", data_segment);
        //printF(YELLOW, "Sending frame: %s\n", response_frame.header_plus_data);
        pthread_mutex_lock(&send_frame_mutex);
        if (send_frame(t_args->client_socket, &response_frame) < 0) {
            printF(RED, "Error sending response frame to Bowman.\n");
            free(playlists_list);
            pthread_mutex_unlock(&send_frame_mutex);
            return NULL;
        }
        pthread_mutex_unlock(&send_frame_mutex);
        offset += data_length;
    }

    free(playlists_list);
    pthread_exit(NULL);
}

/**
 * @brief Busca un archivo de canción por nombre en un directorio y sus subdirectorios de manera recursiva.
 * 
 * Abre el directorio especificado y busca el archivo de canción por nombre. Si encuentra un subdirectorio,
 * realiza una búsqueda recursiva dentro de él. Si encuentra el archivo de canción, devuelve la ruta completa
 * a este archivo.
 * 
 * @param basePath La ruta del directorio base donde comenzar la búsqueda.
 * @param songName El nombre del archivo de la canción que se busca.
 * @return Ruta completa al archivo de la canción si se encuentra, de lo contrario NULL.
 * 
 * @note Devuelve la primera ocurrencia del archivo de canción que coincide con el nombre dado.
 *       La memoria para la ruta del archivo devuelto debe ser liberada por el llamador.
 */

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

/**
 * @brief Busca un directorio por nombre en un directorio base y sus subdirectorios de manera recursiva.
 * 
 * Abre el directorio especificado y busca el directorio por nombre. Si encuentra un subdirectorio,
 * realiza una búsqueda recursiva dentro de él. Si encuentra el directorio objetivo, devuelve la ruta completa
 * a este directorio.
 * 
 * @param basePath La ruta del directorio base donde comenzar la búsqueda.
 * @param folderName El nombre del directorio que se busca.
 * @return Ruta completa al directorio si se encuentra, de lo contrario NULL.
 * 
 * @note Devuelve la primera ocurrencia del directorio que coincide con el nombre dado.
 *       La memoria para la ruta del directorio devuelto debe ser liberada por el llamador.
 */

char* searchFolder(const char *basePath, const char *folderName) {
    struct dirent *dp;
    DIR *dir = opendir(basePath);
    char *path = NULL;

    // Verificar si se pudo abrir el directorio
    if (!dir) {
        return NULL;
    }

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            char newPath[PATH_MAX]; // Usar PATH_MAX para rutas
            struct stat statbuf;

            snprintf(newPath, sizeof(newPath), "%s/%s", basePath, dp->d_name);

            // Verificar si stat tiene éxito
            if (stat(newPath, &statbuf) == -1) {
                perror("stat");
                continue;
            }

            if (S_ISDIR(statbuf.st_mode)) {
                if (strcmp(dp->d_name, folderName) == 0) {
                    path = strdup(newPath);
                    break;
                } else {
                    char *subPath = searchFolder(newPath, folderName);
                    if (subPath) {
                        path = subPath;
                        break;
                    }
                }
            }
        }

        if (path) {
            break;
        }
    }

    closedir(dir);
    return path;
}

/**
 * @brief Obtiene el tamaño de un archivo dado su ruta.
 * 
 * Utiliza la función 'stat' para obtener información del archivo en la ruta especificada,
 * incluyendo su tamaño. Si la función 'stat' falla, muestra un mensaje de error.
 * 
 * @param filePath La ruta del archivo cuyo tamaño se desea obtener.
 * @return El tamaño del archivo en bytes, o -1 si hay un error al obtener la información del archivo.
 */
off_t getFileSize(const char *filePath) {
    struct stat fileStat;

    if (stat(filePath, &fileStat) == 0) {
        return fileStat.st_size;
    } else {
        printF(RED, "Error obtenint informació del fitxer\n");
        return -1;
    }
}

/**
 * @brief Maneja el envío de una canción o playlist a un cliente.
 * 
 * Esta función se encarga de buscar una canción o playlist específica en el servidor,
 * calcular su tamaño y suma de verificación MD5, y enviar estos detalles al cliente.
 * Utiliza mutex para sincronizar el acceso a recursos compartidos como sockets y pipes.
 * Genera un identificador único para cada archivo enviado.
 * 
 * @param args Argumentos del hilo, incluyendo detalles de la canción o playlist y conexión del cliente.
 * @return NULL siempre, para cumplir con la firma de una función de hilo.
 * 
 * @note La función libera la memoria asignada durante su ejecución antes de finalizar.
 *       Si ocurre un error en cualquier punto, envía un mensaje de error y termina la ejecución del hilo.
 */

void* send_song(void * args){
    thread_args *t_args = (thread_args *)args;

    //printF(GREEN, "Thread created -> sending %s\n", t_args->song_name);

    int newPathLength = strlen("..") + strlen(t_args->server_directory) + 1;
    char *newPath = malloc(newPathLength);

    strcpy(newPath, "..");
    strcat(newPath, t_args->server_directory);

    char *path = searchSong(newPath, t_args->song_name);
    if (!path) {
        printF(RED,"Song not found.\n");
        free(path);
        free(newPath);
        pthread_exit(NULL);
    }

    char *md5sum = getFileMD5(path);
    off_t file_size = getFileSize(path);
    if (file_size < 0) {
        printF(RED, "Error getting file size.\n");
        free(path);
        free(md5sum);
        free(newPath);
        pthread_exit(NULL);
    }

    // Mandar el nombre de la canción a monolit
    pthread_mutex_lock(&write_pipe_mutex);
    write(global_write_pipe, t_args->song_name, strlen(t_args->song_name));
    sleep(0.05);
    pthread_mutex_unlock(&write_pipe_mutex);

    char *file_name = NULL;

    if (t_args->is_song) {
        printF(GREEN, "filename is songname: %s\n", t_args->song_name);
        file_name = t_args->song_name;
    } else {
        printF(GREEN, "filename is playlistname: %s\n", t_args->playlist_name);
        file_name = t_args->playlist_name;
    }

    int id = rand() % 900 + 100; // Genera un número aleatorio entre 100 y 999

    int length = snprintf(NULL, 0, "%s&%lu&%s&%d", file_name, file_size, md5sum, id);
    char *data = malloc(length + 1);
    sprintf(data, "%s&%lu&%s&%d", file_name, file_size, md5sum, id);

    Frame new_song = frame_creator(0x04, "NEW_FILE", data);
    pthread_mutex_lock(&send_frame_mutex);
    if (send_frame(t_args->client_socket, &new_song) < 0) {
        printF(RED, "Error sending NEW_FILE frame to %s.\n", t_args->username);
        free(path);
        free(md5sum);
        free(newPath);
        free(data);
        pthread_mutex_unlock(&send_frame_mutex);
        pthread_exit(NULL);
    } 
    pthread_mutex_unlock(&send_frame_mutex);
    empezar_envio(*t_args, id, path, file_size);   

    free(path);
    free(md5sum);
    free(newPath); 
    free(data);

    if (!t_args->is_song) {
        free(t_args->song_name);
        free(t_args->playlist_name);
        free(t_args);
    }

    pthread_exit(NULL);
}

/**
 * @brief Envía todas las canciones de una lista de reproducción (playlist) a un cliente.
 * 
 * Esta función busca una lista de reproducción por nombre, y para cada canción en la lista,
 * crea un nuevo hilo para manejar el envío de la canción al cliente. Utiliza 'pthread_create'
 * para lanzar estos hilos y 'pthread_detach' para asegurar que los recursos de cada hilo se liberen
 * automáticamente al finalizar.
 * 
 * @param args Argumentos del hilo, incluyendo detalles de la lista de reproducción y conexión del cliente.
 * @return NULL siempre, para cumplir con la firma de una función de hilo.
 * 
 * @note En caso de error en la búsqueda de la lista de reproducción o la apertura del directorio,
 *       la función termina antes de intentar enviar canciones.
 *       Cada hilo creado es responsable de liberar su propia memoria asignada.
 */
void* send_list(void * args) {
    thread_args *t_args = (thread_args *)args;

    int newPathLength = strlen("..") + strlen(t_args->server_directory) + 1;
    char *newPath = malloc(newPathLength);
    if (!newPath) {
        printF(RED, "Failed to allocate memory for newPath.\n");
        pthread_exit(NULL);
    }

    strcpy(newPath, "..");
    strcat(newPath, t_args->server_directory);

    char *path = searchFolder(newPath, t_args->list_name);
    if (!path) {
        printF(RED, "Playlist not found.\n");
        free(newPath);
        pthread_exit(NULL);
    }

    DIR *dir = opendir(path);
    if (!dir) {
        printF(RED, "Failed to open directory: %s\n", path);
        free(path);
        pthread_exit(NULL);
    }

    struct dirent *dp;
    while ((dp = readdir(dir)) != NULL) {

        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }

        thread_args *new_t_args = malloc(sizeof(thread_args));
        if (!new_t_args) {
            printF(RED, "Error allocating memory for new thread arguments.\n");
            continue;
        }

        *new_t_args = *t_args; // Copy the existing data

        new_t_args->song_name = strdup(dp->d_name);
        if (!new_t_args->song_name) {
            free(new_t_args);
            continue;
        }

        int size = strlen(t_args->list_name) + strlen(dp->d_name) + strlen(" - ") + 1;
        new_t_args->playlist_name = malloc(size);
        if (!new_t_args->playlist_name) {
            free(new_t_args->song_name);
            free(new_t_args);
            continue;
        }
        snprintf(new_t_args->playlist_name, size, "%s - %s", t_args->list_name, dp->d_name);

        if (pthread_create(&new_t_args->download_song_thread, NULL, send_song, (void *)new_t_args) != 0) {
            printF(RED, "Error creating thread to download song\n");
            free(new_t_args->song_name);
            free(new_t_args->playlist_name);
            free(new_t_args);
        } else {
            // It's assumed that send_song will take care of freeing new_t_args and its contents
            pthread_detach(new_t_args->download_song_thread);
        }

    }

    closedir(dir);
    free(newPath);
    free(path);
    pthread_exit(NULL);
}

/**
 * @brief Envía la canción mp3 en segmentos a un cliente a través de un socket.
 * 
 * Esta función abre el archivo especificado y lo envía en segmentos a través
 * del socket del cliente. Utiliza un identificador único para cada segmento del archivo.
 * La función se asegura de que cada segmento del archivo se envíe correctamente y maneja
 * los errores de lectura y envío.
 * 
 * @param t_args Argumentos del hilo, incluyendo el socket del cliente y detalles del usuario.
 * @param id Identificador único para el archivo que se envía.
 * @param path Ruta del archivo a enviar.
 * @param file_size Tamaño total del archivo a enviar.
 * 
 * @note La función se detiene si recibe una señal de interrupción o si encuentra un error
 *       durante la lectura o el envío del archivo. Utiliza mutex para sincronizar el envío
 *       a través del socket.
 */

void empezar_envio(thread_args t_args, int id, char* path, long file_size) {
    char *id_char = intToStr(id);
    int id_length = strlen(id_char);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printF(RED, "Error opening file\n");
        free(id_char);
        return;
    }

    long total_bytes_sent = 0;
    ssize_t max_bytes_to_send = HEADER_MAX_SIZE - strlen("FILE_DATA") - id_length - 1;
    
    while (total_bytes_sent <= file_size && !poole_sigint_received) {
        // Calcula el tamaño del buffer para esta iteración
        size_t buffer_size = ((total_bytes_sent + max_bytes_to_send) <= (file_size)) ? max_bytes_to_send : (file_size - total_bytes_sent);

        char *buffer = malloc(buffer_size);
        if (buffer == NULL) {
            printF(RED, "Error allocating memory for buffer\n");
            close(fd);
            free(id_char);
            return;
        }
        memset(buffer, 0, buffer_size);

        int bytes_read = read(fd, buffer, buffer_size);
        if (bytes_read <= 0) {
            printF(RED, "Error reading file\n");
            free(buffer);
            close(fd);
            free(id_char);
            return;
        }

        char *data = malloc(id_length + 1 + bytes_read);
        if (data == NULL) {
            printF(RED, "Error allocating memory for data\n");
            free(buffer);
            close(fd);
            free(id_char);
            return;
        }
        memset(data, 0, id_length + 1 + bytes_read);

        memcpy(data, id_char, id_length);
        data[id_length] = '&';
        memcpy(data + id_length + 1, buffer, bytes_read);

        Frame file_data;
        // Preparar la estructura de la trama
        memset(&file_data, 0, sizeof(Frame));
        file_data.type = 0x04;
        file_data.header_length = strlen("FILE_DATA");
        memcpy(file_data.header_plus_data, "FILE_DATA", file_data.header_length);

        // Calcular el tamaño de los datos a copiar
        size_t total_data_size = id_length + 1 + bytes_read; // Tamaño total de ID, '&' y datos
        size_t max_data_to_copy = HEADER_MAX_SIZE - file_data.header_length;
        size_t actual_data_size = total_data_size < max_data_to_copy ? total_data_size : max_data_to_copy;

        // Copiar los datos
        memcpy(file_data.header_plus_data + file_data.header_length, data, actual_data_size);

        pthread_mutex_lock(&send_frame_mutex);
        if (send_frame(t_args.client_socket, &file_data) < 0) {
            printF(RED, "Error sending FILE_DATA frame to %s.\n", t_args.username);
            free(data);
            free(buffer);
            pthread_mutex_unlock(&send_frame_mutex);
            break;
        }
        pthread_mutex_unlock(&send_frame_mutex);

        total_bytes_sent += bytes_read;

        free(data);
        free(buffer);

        sleep(0.05);
        if (total_bytes_sent == file_size) {
            break;
        }
    }

    close(fd);
    free(id_char);
}

/**
 * @brief Envía una señal de apagado al cliente y espera su confirmación antes de cerrar la conexión.
 * 
 * Esta función crea y envía un frame de apagado al cliente a través del socket especificado. Luego,
 * espera recibir una respuesta de confirmación del cliente. Si la respuesta es satisfactoria,
 * procede a cerrar el socket. En caso de error en el envío o recepción del frame, muestra un mensaje
 * de error y maneja el cierre del socket adecuadamente.
 * 
 * @param client_socket El socket del cliente al que se le envía la señal de apagado.
 * 
 * @note Utiliza un mutex para sincronizar el envío del frame de apagado.
 *       Si la respuesta del cliente no es la esperada, termina el proceso con un error.
 */
void server_shutdown(int client_socket) {
    
    Frame shutdown_frame = frame_creator(0x06, "POOLE_SHUTDOWN", "");

    pthread_mutex_lock(&send_frame_mutex);
    if (send_frame(client_socket, &shutdown_frame) < 0) {
        printF(RED, "Error sending response frame to Bowman.\n");
        return;
    }
    pthread_mutex_unlock(&send_frame_mutex);

    Frame response_frame;
    if (receive_frame(client_socket, &response_frame) <= 0) {
        printF(RED, "Error receiving response from Bowman\n");
        close(client_socket);
        return;
    }

    if (strcmp(response_frame.header_plus_data, "SHUTDOWN_OK") == 0) {
        printF(GREEN, "Bowman notified\n");
    } else {
        printF(RED, "Connection to Poole failed: %s\n", response_frame.header_plus_data);
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    close(client_socket);

}