#ifndef GLOBAL_H
#define GLOBAL_H

// Importaciones de librerias
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#include <sys/dir.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>  
#include <sys/wait.h> 


/**
 * @brief Colors to print the messages
*/
#define WHITE    0
#define GREEN    1
#define YELLOW   2
#define RED      3

/**
 * @brief Maximum size for frames protocol
*/
#define FRAME_SIZE 256
#define HEADER_SIZE 3
#define HEADER_MAX_SIZE 253

// Estructura para almacenar un frame
typedef struct {
    char type; // 1 byte
    unsigned short header_length; // 2 bytes
    char header_plus_data[HEADER_MAX_SIZE]; // 253 bytes
} Frame;

// Estructura para almacenar un frame binario en la cola de mensajes
typedef struct {
    long msg_type;
    char msg_text[HEADER_MAX_SIZE];
} Message_buffer;

// Estructura para almacenar la informacion de un servidor Poole
typedef struct {
    int *poole_socket;
    char *username;
    char *discovery_ip;
    int discovery_port;
} thread_receive_frames;

// Estructura para almacenar la información de una canción
typedef struct {
    char *fileName;
    long fileSize;
    char *md5sum;
    int id;
} Song;

// Estructura para almacenar la información de una canción descargada o descargando
typedef struct {
    Song song;
    long downloaded_bytes;
    long song_size;
    pthread_t thread_id;
} Song_Downloading;

// Structure to store the Bowman's configuration
typedef struct {
    char *username;
    char *folder_path;
    char *discovery_ip;
    int discovery_port;
} BowmanConfig;

// Estructura para almacenar la configuracion de Poole
typedef struct {
    char *username;
    char *folder_path;
    char *discovery_ip;
    int discovery_port;
    char *poole_ip;
    int poole_port;
} PooleConfig;

// Estructura para pasar los argumentos al thread
typedef struct {
    int client_socket;
    int is_song;
    char *playlist_name;
    char* username;
    char* server_directory;
    char* song_name;
    char* list_name;
    pthread_t list_songs_thread;
    pthread_t list_playlists_thread;
    pthread_t download_song_thread;
    pthread_t download_playlist_thread;
} thread_args;

// Estructura para almacenar la configuración del servidor
typedef struct PooleServer {
    char *server_name;       // Nombre del servidor Poole
    char *ip_address;        // Dirección IP del servidor Poole
    int port;                // Puerto del servidor Poole
    int connected_users;     // Número de usuarios actualmente conectados a este servidor Poole
    char **usernames;        // Lista de nombres de usuario conectados a este servidor Poole
    struct PooleServer* next;// Puntero al siguiente servidor Poole en la lista
} PooleServer;

// Estructura para almacenar la configuración
typedef struct {
    struct sockaddr_in poole_addr;
    struct sockaddr_in bowman_addr;
} DiscoveryConfig;

/**
 * @brief Print a message with the color specified
 * 
 * @param color Color to print the message
 * @param format Format of the message
 * @param ... Arguments of the message
 * @return void
*/
void printF(int color, const char *format, ...);

/**
 * @brief Read from a file descriptor until a delimiter is found
 * 
 * @param fd File descriptor to read from
 * @param end Delimiter to stop reading
 * @return char* String read from the file descriptor
*/
char *read_until_delimiter(int fd, char end);

/**
 * @brief Check if the number of arguments is the expected
 * 
 * @param current Number of arguments received
 * @param expected Number of arguments expected
 * @return void
*/
void check_input_arguments(const int current, const int expected);

/**
 * @brief Display a loading spinner for a certain amount of time
 * 
 * @param color Color of the spinner
 * @param duration Duration of the spinner
 * @return void
*/
void display_loading_spinner(int color, int duration);

/**
 * @brief Crea un frame con los datos especificados y lo retorna
 * 
 * @param type Tipo de frame
 * @param header Header del frame
 * @param data Datos del frame
 * 
 * @return Frame Frame creado
*/
Frame frame_creator(char type, char* header, char* data);

/**
 * @brief Crea un frame con los datos especificados binarios y lo retorna
 * 
 * @param type Tipo de frame
 * @param header Header del frame
 * @param data Datos del frame
 * @param data_size Tamaño de los datos
 * 
 * @return Frame Frame creado
*/
Frame binary_frame_creator(char type, char* header, char* data, int data_size);

/**
 * @brief Sends a frame through a socket
 * 
 * @param socket Socket to send the frame through
 * @param frame Frame to send
 * 
 * @return void
*/
int send_frame(int socket, Frame *frame);

/**
 * @brief Receives a frame through a socket
 * 
 * @param socket Socket to receive the frame through
 * @param frame Frame to receive
 * 
 * @return int Number of bytes read
*/
int receive_frame(int socket, Frame *frame);

/**
 * @brief Write a message to a message queue
 * 
 * @param msg_id Message queue id
 * @param message Message to write
 * 
 * @return void 
*/
void msg_queue_writer(int msg_id, Message_buffer *message);

/**
 * @brief Read a message from a message queue
 * 
 * @param msg_id Message queue id
 * @param message Message to read
 * 
 * @return void 
*/
void msg_queue_reader(int msg_id, Message_buffer *message);

/**
 * @brief Deletes a message queue
 * 
 * @param msg_id Message queue id
 * 
 * @return void
*/
void msg_queue_delete(int msg_id);

/**
 * @brief Converts int to string
 * 
 * @param num num to convert
 * 
 * @return str
*/
char* intToStr(int num);

/**
 * @brief Gets the md5sum of a file in a given path
 * 
 * @param filePath Path of the file
 * 
 * @return md5sum
*/
char* getFileMD5(char *filePath);

#endif // !GLOBAL_H
