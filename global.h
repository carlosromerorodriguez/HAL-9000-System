#ifndef GLOBAL_H
#define GLOBAL_H

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


//BOWMAN INCLUDES
#include "Bowman/config.h"
#include "Bowman/command_handler.h"

//POOLE INCLUDES
#include "Poole/config.h"
#include "Poole/command_handler.h"

//DISCOVERY INCLUDES
#include "Discovery/config.h"


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
#define HEADER_MAX_SIZE 253

typedef struct {
    char type; // 1 byte
    unsigned short header_length; // 2 bytes
    char header_plus_data[HEADER_MAX_SIZE]; // 253 bytes
} Frame;

typedef struct {
    long msg_type;
    char msg_text[HEADER_MAX_SIZE];
} Message_buffer;

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
#endif // !GLOBAL_H
