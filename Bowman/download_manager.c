#include "download_manager.h"

// Variables globales
Song_Downloading *songsDownloading = NULL;
int num_songs_downloading = 0;


// Variables globales importadas de otros TADS
extern int poole_socket_for_bowman;
extern char *bowman_folder_path;
extern int bowman_sigint_received;
extern int msg_id;

/**
 * @brief Inicia la descarga de una canción o una lista de reproducción.
 *
 * Envía una trama al servidor Poole para solicitar la descarga de una canción o una lista de reproducción.
 * 
 * @param name Nombre de la canción o lista de reproducción a descargar.
 */
void download(char *name) {

    if(check_if_playlist(name)){
        Frame download_frame = frame_creator(0x03, "DOWNLOAD_LIST", name);

        if (send_frame(poole_socket_for_bowman, &download_frame) < 0) {
        printF(RED, "Error sending DOWNLOAD_LIST frame to Poole server.\n");
        return;
    }
    }else{
        Frame download_frame = frame_creator(0x03, "DOWNLOAD_SONG", name);

        if (send_frame(poole_socket_for_bowman, &download_frame) < 0) {
            printF(RED, "Error sending DOWNLOAD_SONG frame to Poole server.\n");
            return;
        } 
    }

    printF(GREEN, "Download started!\n");
}

/**
 * @brief Limpia las canciones completamente descargadas de la lista de descargas.
 *
 * Revisa la lista de canciones descargándose y elimina aquellas que han sido completamente descargadas.
 */
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

/**
 * @brief Función ejecutada en un hilo para manejar la descarga de una canción.
 *
 * Procesa la información de la canción, crea un archivo y escribe los datos recibidos en él.
 * 
 * @param args Argumentos pasados al hilo, incluyendo la información de la canción.
 * @return void* Puntero a NULL al finalizar la ejecución del hilo.
 */
void* startSongDownload(void* args) {
    char* str = (char*)args; 

    Song newSong;
    parseSongInfo(str, &newSong);

    // Crear la ruta completa para el directorio dentro de Bowman
    size_t directoryPathLength = strlen("../Bowman/") + strlen(bowman_folder_path) + 1; // +1 para el '\0'
    char *directoryPath = malloc(directoryPathLength);

    if (directoryPath == NULL) {
        perror("Error allocating memory for directory path");
        free(str);
        pthread_exit(NULL);
    }

    snprintf(directoryPath, directoryPathLength, "../Bowman/%s", bowman_folder_path);

    // Crear el directorio si no existe
    struct stat st = {0};
    if (stat(directoryPath, &st) == -1) {
        if (mkdir(directoryPath, 0777) != 0) {
            printF(RED, "Error creating directory\n");
            free(directoryPath);
            free(str);
            pthread_exit(NULL);
        }
    } else if (!S_ISDIR(st.st_mode)) {
        printF(RED, "Error creating directory\n");
        free(directoryPath);
        free(str);
        pthread_exit(NULL);
    }

    // Construir la ruta completa del archivo dentro de este directorio
    size_t fullPathLength = strlen(directoryPath) + 1 + strlen(newSong.fileName) + 1;
    char *fullPath = malloc(fullPathLength);

    if (fullPath == NULL) {
        perror("Error allocating memory for file path");
        free(directoryPath);
        free(str);
        pthread_exit(NULL);
    }

    snprintf(fullPath, fullPathLength, "%s/%s", directoryPath, newSong.fileName);

    int fd = open(fullPath, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd < 0) {
        perror("Error opening file");
        free(fullPath);
        free(str);
        pthread_exit(NULL);
    }

    Message_buffer *msg = malloc(sizeof(Message_buffer));
    // Asegúrate de que msg no sea NULL antes de continuar
    if (msg == NULL) {
        perror("Error allocating memory for message buffer");
        close(fd);
        free(fullPath);
        free(str);
        free(directoryPath);
        pthread_exit(NULL);
    }

    msg->msg_type = (long)newSong.id;
    long total_bytes_written = 0;
    ssize_t max_write_size = HEADER_MAX_SIZE - strlen("FILE_DATA") - 3 - 1;

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

    //Comparar el md5sum de la canción descargada con el md5sum de la canción original
    char *newSongMD5 = getFileMD5(fullPath);
    printF(RED, "MD5SUM Calculado: %s\n", newSongMD5);
    printF(RED, "MD5SUM Que Deberia dar: %s\n", newSong.md5sum);
    if (newSongMD5 != NULL && strcmp(newSongMD5, newSong.md5sum) == 0) {
        //printF(GREEN, "\nDescarga completada: %s\n", newSong.fileName);
        free(newSongMD5);
        //printF(WHITE, "$ ");
    } else {
        printF(RED, "\nError downloading song: %s\n", newSong.fileName);
        deleteSongFromArray(newSong);
        close(fd);
        free(msg);
        free(fullPath);
        free(directoryPath);
        free(str);
        if (newSongMD5 != NULL) {
            free(newSongMD5);
        }
        printF(WHITE, "$ ");
        pthread_exit(NULL);
    }

    //printF(GREEN, "\nDescarga completada\n");
    close(fd);
    free(msg);
    free(fullPath);
    free(directoryPath);
    free(str);

    pthread_exit(NULL);
}

/**
 * @brief Maneja la recepción de un nuevo archivo desde el servidor Poole.
 *
 * Crea un hilo para manejar la descarga del archivo y procesa los datos recibidos.
 * 
 * @param data Datos del archivo recibido.
 */
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
    }
}

/**
 * @brief Maneja los datos de archivo recibidos del servidor Poole.
 *
 * Almacena los datos del archivo en una cola de mensajes para su posterior procesamiento.
 * 
 * @param data Datos del archivo recibido.
 */
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

/**
 * @brief Imprime todas las canciones que están siendo descargadas actualmente.
 *
 * Muestra el estado de descarga de cada canción en la lista de descargas.
 */

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