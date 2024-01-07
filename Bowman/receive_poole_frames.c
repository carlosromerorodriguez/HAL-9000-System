#include "receive_poole_frames.h"

// Variables globales importadas de otros TADS
extern int bowman_sigint_received;
extern int bowman_is_connected;
extern int discovery_socket;
extern int poole_socket_for_bowman;
extern int msg_id;
extern char *username;
extern char* global_server_name;
extern char *discovery_ip; 
extern int global_server_port;
extern unsigned char connected;
extern char global_server_ip[INET_ADDRSTRLEN]; // Tamaño suficiente para direcciones IPv4
extern int poole_socket;

void *receive_frames(void *args) {
    thread_receive_frames trf = *(thread_receive_frames *)args;

    while (!bowman_sigint_received) {
        Frame response_frame;
        if (receive_frame(*trf.poole_socket, &response_frame) <= 0) {
            continue;   
        }
        //RECIBIR EL TAMAÑO DE LIST_SONGS A MOSTRAR
        if (!strncasecmp(response_frame.header_plus_data , "LIST_SONGS_SIZE", response_frame.header_length)) {
            handleListSongsSize(response_frame.header_plus_data + response_frame.header_length);
        }
        else if (!strncasecmp(response_frame.header_plus_data , "SONGS_RESPONSE", response_frame.header_length)) {
            handleSongsResponse(response_frame.header_plus_data + response_frame.header_length);            
        }
        else if (!strncasecmp(response_frame.header_plus_data , "LIST_PLAYLISTS_SIZE", response_frame.header_length)) {
            handleListPlaylistsSize(response_frame.header_plus_data + response_frame.header_length);
        }
        else if (!strncasecmp(response_frame.header_plus_data , "PLAYLISTS_RESPONSE", response_frame.header_length)) {
            handlePlaylistsResponse(response_frame.header_plus_data + response_frame.header_length);
        }
        else if (!strncasecmp(response_frame.header_plus_data, "NEW_FILE", response_frame.header_length)) {
            handleNewFile(response_frame.header_plus_data + response_frame.header_length);
        }
        else if (!strncasecmp(response_frame.header_plus_data, "FILE_DATA", response_frame.header_length)) {
            handleFileData(response_frame.header_plus_data + response_frame.header_length);
        }
        else if (!strncasecmp(response_frame.header_plus_data , "LOGOUT_OK", response_frame.header_length)) {
            printF(GREEN, "Logout successful\n");
            disconnect_notification_to_discovery(trf.username, trf.discovery_ip, trf.discovery_port, global_server_port, global_server_ip);
            pthread_exit(NULL);
        }
        else if (!strncasecmp(response_frame.header_plus_data , "LOGOUT_KO", response_frame.header_length)) {
            printF(RED, "Logout failed\n");
            pthread_exit(NULL);
        }
        else if (!strncasecmp(response_frame.header_plus_data , "POOLE_SHUTDOWN", response_frame.header_length)) {
            
            printF(RED, "\nPoole server has shutdown. Reconnecting to Discovery...\n");

            Frame logout_frame = frame_creator(0x06, "SHUTDOWN_OK", "");

            if (send_frame(poole_socket_for_bowman, &logout_frame) < 0) {
                printF(RED, "Error sending SHUTDOWN_OK frame to Poole server.\n");
                pthread_exit(NULL);
            } 

            close(*trf.poole_socket);
                        
            if(!connect_to_another_server(trf.username, trf.discovery_ip, trf.discovery_port, trf.poole_socket, discovery_socket, poole_socket_for_bowman, msg_id, global_server_name, global_server_port, global_server_ip)) {
                printF(RED, "Error connecting to another server. No Pooles available\n");
                bowman_is_connected = 0;
                printF(WHITE, "$ ");
                pthread_exit(NULL);
            }
            printF(WHITE, "$ ");
        }
        else if (!strncasecmp(response_frame.header_plus_data , "UNKNOWN", response_frame.header_length)) {
 
        }
    }

    pthread_exit(NULL);
}