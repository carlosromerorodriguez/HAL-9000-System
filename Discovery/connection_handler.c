#include "connection_handler.h"

/**
 * @brief Función que maneja las conexiones entrantes de Poole
 * 
 * @param arg Puntero al socket de Poole
 * 
 * @return void
*/
void* handlePooleConnection(void* arg) {
    int poole_socket = *(int*)arg;
    free(arg);

    if (poole_thread_active) {
        //Liberar toda la memoria dinámica o recursos que se hayan podido reservar
        close(poole_socket);
        return NULL;
    }

    Frame request_frame;
    if (receive_frame(poole_socket, &request_frame) <= 0) {
        printF(RED, "Error receiving frame from Poole.\n");
        close(poole_socket);
        return NULL;
    }

    // printF(YELLOW, "Received frame: %s\n", request_frame.header_plus_data);

    // Verificar si la trama es NEW_POOLE
    if (strncmp(request_frame.header_plus_data, "NEW_POOLE", request_frame.header_length) == 0) {

        // Extraer el nombre, IP y puerto del servidor Poole de la trama
        char *data = request_frame.header_plus_data + request_frame.header_length;
        char *token = strtok(data, "&");
        char *server_name = token != NULL ? strdup(token) : NULL;
        token = strtok(NULL, "&");
        char *ip_address = token != NULL ? strdup(token) : NULL;
        token = strtok(NULL, "&");
        int port = token != NULL ? atoi(token) : -1;

        if (server_name != NULL && ip_address != NULL && port != -1) {
            // Añadir el servidor Poole a la lista
            add_poole_server(server_name, ip_address, port);
            free(server_name);
            free(ip_address);

            // Enviar trama CON_OK a Poole
            Frame response_frame = frame_creator(0x01, "CON_OK", "");
            send_frame(poole_socket, &response_frame);
            printF(YELLOW, "%s ->", fecha);
            printF(GREEN, " New connection from Poole\n");
        } else {
            // Enviar trama CON_KO a Poole si hay algún error
            Frame response_frame = frame_creator(0x01, "CON_KO", "");
            send_frame(poole_socket, &response_frame);
        }
    }
    else if (strncmp(request_frame.header_plus_data, "POOLE_SHUTDOWN", request_frame.header_length) == 0) {
        printF(YELLOW, "Info POOLE_SHUTDOWN: %s", request_frame.header_plus_data);
        
        // Enviar trama CON_KO a Poole
        Frame response_frame = frame_creator(0x01, "CON_KO", "");
        send_frame(poole_socket, &response_frame);

        // Cerrar el socket de Poole
        close(poole_socket);
        
        delete_poole_from_list(request_frame.header_plus_data + request_frame.header_length);
    } 
    else {
        printF(RED, "Received unexpected frame header from Poole.\n");
    }

    close(poole_socket);
    pthread_exit(NULL);
}

/**
 * @brief Función que maneja las conexiones entrantes de Bowman
 * 
 * @param arg Puntero al socket de Bowman
 * 
 * @return void
*/
void* handleBowmanConnection(void* arg) {
    int bowman_socket = *(int*)arg;
    free(arg);

    if (bowman_thread_active) {
        //Liberar toda la memoria dinámica o recursos que se hayan podido reservar
        close(bowman_socket);
        pthread_exit(NULL);
    }

    Frame request_frame;
    if (receive_frame(bowman_socket, &request_frame) <= 0) {
        printF(RED, "Error receiving frame from Bowman. Connection aborted.\n");
        close(bowman_socket);
        pthread_exit(NULL);
    }

    // Verificar si la trama es NEW_BOWMAN
    if (strncmp(request_frame.header_plus_data, "NEW_BOWMAN", request_frame.header_length) == 0) {
        PooleServer* least_loaded_poole = find_least_loaded_poole_server();
        Frame response_frame;

        if (least_loaded_poole != NULL) {
            least_loaded_poole->usernames = realloc(least_loaded_poole->usernames, (least_loaded_poole->connected_users + 1) * sizeof(char*));
            least_loaded_poole->usernames[least_loaded_poole->connected_users] = strdup(request_frame.header_plus_data + request_frame.header_length);
            if (least_loaded_poole->usernames[least_loaded_poole->connected_users] == NULL) {
                printF(RED, "Error allocating memory for username.\n");
                close(bowman_socket);
                pthread_exit(NULL);
            }

            // Incrementar el contador de usuarios conectados
            least_loaded_poole->connected_users++;

            printF(YELLOW, "%s -> ", fecha);
            printF(GREEN, "New connection from Bowman: %s\n", least_loaded_poole->usernames[least_loaded_poole->connected_users - 1]);

            char data[HEADER_MAX_SIZE];
            snprintf(data, sizeof(data), "%s&%s&%d", least_loaded_poole->server_name, least_loaded_poole->ip_address, least_loaded_poole->port);
            response_frame = frame_creator(0x01, "CON_OK", data);
        } else {    
            printF(RED, "No Poole servers available. Sending CON_KO to Bowman.\n");
            response_frame = frame_creator(0x01, "CON_KO", "");
        }

        // Enviar la trama de respuesta a Bowman
        if (send_frame(bowman_socket, &response_frame) < 0) {
            printF(RED, "Error sending response frame to Bowman.\n");
        }
    } else if (strncmp(request_frame.header_plus_data, "EXIT", request_frame.header_length) == 0) {
        logout_user(request_frame.header_plus_data, bowman_socket);
    } else {
        printF(RED, "Received an unexpected frame header from Bowman: %s\n", request_frame.header_plus_data);
    }

    close(bowman_socket);
    pthread_exit(NULL);
}