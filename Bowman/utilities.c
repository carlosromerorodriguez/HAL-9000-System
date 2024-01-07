#include "utilities.h"

// Variables globales importadas de otros TADS
extern char *global_server_name;
extern char global_server_ip[INET_ADDRSTRLEN]; // Tamaño suficiente para direcciones IPv4
extern int global_server_port;
extern Song_Downloading *songsDownloading;
extern int num_songs_downloading;

/**
 * @brief Parse the command to remove the extra spaces
 * @example "LIST             SONGS" -> "LIST SONGS
 * 
 * @param command Command to parse
 * @return char* Parsed command
*/
char *parse_command(char *command) {
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
char *trim_whitespace(char* str) {
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
 * @brief Cuenta la cantidad de canciones en una cadena de texto
 * 
 * @param str Cadena de texto
 * 
 * @return int Cantidad de canciones
*/
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
 * @brief Cuenta la cantidad de listas de reproducción en una cadena de texto
 * 
 * @param str Cadena de texto
 * 
 * @return int Cantidad de listas de reproducción
*/
int countPlaylists(const char *str) {
    int count = 0;
    const char *temp = str;
    while (*temp) {
        if (*temp == '#') count++;
        temp++;
    }
    return count;
}

/**
 * @brief Parsea la información de una canción y la almacena en una estructura Song
 * 
 * @param str Cadena de texto con la información de la canción
 * @param song Estructura Song donde se almacenará la información
 * 
 * @return void
*/
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
        global_server_name = malloc(strlen(token) + 1); // Nombre del servidor
        strcpy(global_server_name, token);
        global_server_name[strlen(token)] = '\0';
        
        token = strtok(NULL, "&");
    }
    if (token != NULL) {
        strcpy(global_server_ip, token); // Dirección IP
        token = strtok(NULL, "&");
    }
    if (token != NULL) {
        global_server_port = atoi(token); // Puerto
    }
}

/**
 * @brief Verifica si el nombre de un archivo corresponde a una lista de reproducción
 * 
 * @param name Nombre del archivo
 * 
 * @return int 0 si es una canción, 1 si es una lista de reproducción
*/
int check_if_playlist(char *name) {

    int name_length = strlen(name);
    
    if (name_length >= 4 && strcmp(name + name_length - 4, ".mp3") == 0) {
        return 0;
    } else {
        return 1; 
    }
}

/**
 * @brief Añade una canción al arreglo de canciones descargándose
 * 
 * @param newSong Canción a añadir
 * @param thread_id ID del hilo que descarga la canción
 * 
 * @return void
*/
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

/**
 * @brief Elimina una canción del arreglo de canciones descargándose
 * 
 * @param song Canción a eliminar
 * 
 * @return void
*/
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

/**
 * @brief Libera la memoria dinámica del arreglo de canciones descargándose
 * 
 * @return void
*/
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