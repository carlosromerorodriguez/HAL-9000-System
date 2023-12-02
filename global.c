#include "global.h"

/**
 * @brief Print a message with the color specified
 * 
 * @param color Color to print the message
 * @param format Format of the message
 * @param ... Arguments of the message
 * @return void
*/
void printF(int color, const char *format, ...) {
    char *colorCode;
    switch (color) {
        case GREEN:
            colorCode = "\033[32m";
            break;
        case YELLOW:
            colorCode = "\033[33m";
            break;
        case RED:
            colorCode = "\033[31m";
            break;
        default:
            colorCode = "";
            break;
    }

    va_list args;
    va_start(args, format);

    char *buffer = NULL;
    char *full_format = malloc(strlen(colorCode) + strlen(format) + strlen("\033[0m") + 1);
    if (!full_format) {
        va_end(args);
        return; 
    }
    sprintf(full_format, "%s%s\033[0m", colorCode, format);
    
    int len = vasprintf(&buffer, full_format, args);
    if (buffer) {
        write(1, buffer, len);
        free(buffer);
    }

    free(full_format);
    va_end(args);
}

/**
 * @brief Read from a file descriptor until a delimiter is found
 * 
 * @param fd File descriptor to read from
 * @param end Delimiter to stop reading
 * @return char* String read from the file descriptor
*/
char *read_until_delimiter(int fd, char end) {
    int i = 0, size;
    char c = '\0';
    char *string = (char *)malloc(sizeof(char));

    while (1) {
        size = read(fd, &c, sizeof(char));

        if (c != end && size > 0) {
            string = (char *)realloc(string, sizeof(char) * (i + 2));
            string[i++] = c;
        } else { 
            break; 
        }
    }

    string[i] = '\0';
    return string;
}

/**
 * @brief Check if the number of arguments is the expected
 * 
 * @param current Number of arguments received
 * @param expected Number of arguments expected
 * @return void
*/
void check_input_arguments(const int current, const int expected) {
    if (current != expected) {
        if (current < expected) {
                printF(RED, "ERROR: Less arguments than expected!\n");
        } else {
                printF(RED, "ERROR: More arguments than expected!\n");
        }
        exit(EXIT_FAILURE);
    } 
}

/**
 * @brief Display a loading spinner for a certain amount of time
 * 
 * @param color Color of the spinner
 * @param duration Duration of the spinner
 * @return void
*/
void display_loading_spinner(int color, int duration) {
    char spinner[] = "|/-\\\0";
    struct timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = 200 * 1000000;

    int end_time = time(NULL) + duration;

    while (time(NULL) < end_time) {
        for (int i = 0; i < (int) strlen(spinner); i++) {
            printF(color, "\r%c", spinner[i]);
            fflush(stdout);
            nanosleep(&delay, NULL);
        }
    }
    printF(color, "\r ");
    printF(WHITE, "\n");
}

/**
 * @brief Crea un frame con los datos especificados y lo retorna
 * 
 * @param type Tipo de frame
 * @param header Header del frame
 * @param data Datos del frame
 * 
 * @return Frame Frame creado
*/
Frame frame_creator(char type, char* header, char* data) {
    Frame frame;
    memset(&frame, 0, sizeof(frame));  // Padding initialization to zeros

    frame.type = type;
    frame.header_length = strlen(header);

    snprintf(frame.header_plus_data, frame.header_length + 1, "%s", header);

    snprintf(frame.header_plus_data + frame.header_length, HEADER_MAX_SIZE - frame.header_length, "%s", data);

    return frame;
}

/**
 * @brief Sends a frame through a socket
 * 
 * @param socket Socket to send the frame through
 * @param frame Frame to send
 * 
 * @return void
*/
int send_frame(int socket, Frame *frame) {
    char buffer[FRAME_SIZE];
    memset(buffer, 0, FRAME_SIZE);
    int index = 0;

    buffer[index++] = frame->type;
    memcpy(buffer + index, &frame->header_length, sizeof(frame->header_length));
    index += sizeof(frame->header_length);
    memcpy(buffer + index, frame->header_plus_data, HEADER_MAX_SIZE);

    if (write(socket, buffer, FRAME_SIZE) < FRAME_SIZE) {
        return -1;
    }

    return 0;
}

/**
 * @brief Receives a frame through a socket
 * 
 * @param socket Socket to receive the frame through
 * @param frame Frame to receive
 * 
 * @return int Number of bytes read
*/
int receive_frame(int socket, Frame *frame) {
    char buffer[FRAME_SIZE];
    memset(buffer, 0, FRAME_SIZE);

    int read_bytes = read(socket, buffer, FRAME_SIZE);
    if (read_bytes <= 0) {
        return read_bytes;
    }

    frame->type = buffer[0];
    memcpy(&frame->header_length, buffer + 1, sizeof(frame->header_length));
    memcpy(frame->header_plus_data, buffer + 3, HEADER_MAX_SIZE);

    return read_bytes;
}

void msg_queue_writer(int msg_id, Message_buffer *message) {
    if (message == NULL) {
        printF(RED, "Invalid message buffer\n");
        return;
    }
    
    if (msgsnd(msg_id, message, sizeof(message->msg_text), 0) == -1) {
        printF(RED, "msgsnd error\n");
    }
}

void msg_queue_reader(int msg_id, Message_buffer *message) {
    if (message == NULL) {
        printF(RED, "Invalid message buffer\n");
        return;
    }

    if (msgrcv(msg_id, message, sizeof(message->msg_text), message->msg_type, 0) == -1) {
        printF(RED, "msgrcv error\n");
    }
}

void msg_queue_delete(int msg_id) {
    if (msgctl(msg_id, IPC_RMID, NULL) == -1) {
        printF(RED, "msgctl error\n");
    } else {
        printF(GREEN, "Message queue deleted successfully\n");
    }
}