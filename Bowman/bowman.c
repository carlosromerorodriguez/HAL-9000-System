#include "../global.h"
#include "config.h" 
#include "session_handler.h"
#include "utilities.h"
#include "session_handler.h"

/*
    Defines
*/
#define MAX_PROMPT_SIZE 512 // para limitar el máximo de memória dinámica para un promt
                            // y evitar que el sistema reviente en casos extremos, p.ej:
                            // 50 millones de líneas

/*
    Variables globales necesárias (pata liberar en caso de Ctrl+C)
*/
BowmanConfig *bowman_config;// configuración de Bowman
static char *user_prompt;          // prompt del usuario
int discovery_socket;
int poole_socket;
extern char *global_server_name;
unsigned char connected;
pthread_t listen_poole_thread;
thread_receive_frames *pargs;
volatile sig_atomic_t bowman_sigint_received = 0;
extern char *bowman_folder_path;

/*
    Delaración de funciones
*/
void kctrlc(int signum);
void free_all_dynamic_memory(void);
void parse_and_store_server_info(const char* server_info);

/**
 * @brief Función principal del programa Bowman
 * 
 * @param argc Número de argumentos
 * @param argv Argumentos
 * @return int 0 si todo ha ido bien, 1 si ha habido algún error
*/
int main(int argc, char **argv) {

    check_input_arguments(argc, 2);

    unsigned char end = 0; connected = 0;
    bowman_config = (BowmanConfig *) malloc (sizeof(BowmanConfig));
    user_prompt = (char *) malloc (sizeof(char) * MAX_PROMPT_SIZE);

    signal(SIGINT, kctrlc); //Activar signal Ctrl+C
    
    // Rellenar la configuración de Bowman y comprobación si se rellena correctamente
    if (fill_bowman_config(bowman_config, argv[1]) == 0) {
        printF(RED, "ERROR: User connection unreachable. Please, check the user's filename and try again!\n");
        exit(EXIT_FAILURE);
    }

    printF(GREEN, "\n%s user initialized\n", bowman_config->username);

    do {
        printF(WHITE, "$ ");
        int n = read(0, user_prompt, MAX_PROMPT_SIZE);
        user_prompt[n-1] = '\0';

        end = handle_bowman_command(user_prompt, &connected, &discovery_socket, bowman_config->username, bowman_config->discovery_ip, bowman_config->discovery_port, &poole_socket, bowman_config->folder_path);
        sleep(0.1);
    } while (!end);

    

    close(discovery_socket);
    close(poole_socket);
    free_all_dynamic_memory();

    return (EXIT_SUCCESS);
}

/**
 * @brief Libera toda la memoria dinámica reservada en el programa
 * 
 * @return void
*/
void free_all_dynamic_memory(void) {
    freeSongDownloadingArray();

    bowman_sigint_received = 1;
    if (connected == 1) {
        logout(bowman_config->username);
    }

    pthread_join(listen_poole_thread, NULL);

    if (pargs) {
        free(pargs);
    }

    if (bowman_config != NULL) {
        if (bowman_config->username != NULL) {
            free(bowman_config->username);
        }
        if (bowman_config->folder_path != NULL) {
            free(bowman_config->folder_path);
        }
        if (bowman_config->discovery_ip != NULL) {
            free(bowman_config->discovery_ip);
        }
        free(bowman_config);
    } if (user_prompt != NULL) {
        free(user_prompt);
        user_prompt = NULL;
    } if (global_server_name != NULL) {
        free(global_server_name);
        global_server_name = NULL;
    } if (discovery_socket != 0) {
        close(discovery_socket);
        discovery_socket = 0;
    } if (poole_socket != 0) {
        close(poole_socket);
        poole_socket = 0;
    }
}

/**
 * @brief Función que se ejecuta cuando se pulsa Ctrl+C
 * 
 * @param signum Señal que se ha recibido
 * @return void
*/
void kctrlc(int signum) {
    if (signum == SIGINT) {
        printF(RED, "\n\nKCRTL+C received. Exiting...\n");
        free_all_dynamic_memory();
    }
    exit(EXIT_SUCCESS);
}