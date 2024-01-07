#include "config.h"
#include "connection_handler.h"
#include "poole_server_manager.h"
#include "user_manager.h"
#include "utilities.h"

// Definiciones de constantes
#define BACKLOG 128
#define DATA_MAX_SIZE 253

// Variables globales

DiscoveryConfig *discoverConfig;
int poole_fd, bowman_fd;
pthread_t poole_thread_id, bowman_thread_id;
volatile sig_atomic_t poole_thread_active = 0;
volatile sig_atomic_t bowman_thread_active = 0;
int poole_thread_created = 0;
int bowman_thread_created = 0;

char fecha[20];

/**
 * @brief Función principal del programa
 * 
 * @param argc Número de argumentos de la línea de comandos
 * @param argv Argumentos de la línea de comandos
 * 
 * @return int 0 si todo ha ido bien, 1 si ha habido algún error
*/
int main(int argc, char* argv[]) {

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", tm);

    check_input_arguments(argc, 2);

    signal(SIGINT, kctrlc); //Activar signal Ctrl+C

    discoverConfig = (DiscoveryConfig *) malloc (sizeof(DiscoveryConfig));

    if (readDiscoveryConfig(argv[1], discoverConfig) == 0) {
        printF(RED, "Error reading discovery file\n");
        exit(EXIT_FAILURE);
    }

    poole_fd = socket(AF_INET, SOCK_STREAM, 0);
    bowman_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (poole_fd < 0 || bowman_fd < 0) {
        printF(RED, "Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    // Enlazar y escuchar en las direcciones de Poole y Bowman
    if (bind(poole_fd, (struct sockaddr *)&discoverConfig->poole_addr, sizeof(discoverConfig->poole_addr)) < 0 ||
        bind(bowman_fd, (struct sockaddr *)&discoverConfig->bowman_addr, sizeof(discoverConfig->bowman_addr)) < 0) {
        printF(RED, "Bind failed\n");
        free_all_dynamic_memory();
        exit(EXIT_FAILURE);
    }

    listen(poole_fd, BACKLOG);
    listen(bowman_fd, BACKLOG);

    // Configurar los sockets para que sean no bloqueantes
    fcntl(poole_fd, F_SETFL, O_NONBLOCK);
    fcntl(bowman_fd, F_SETFL, O_NONBLOCK);

    // Aceptar las conexiones y crear hilos para manejar cada una de ellas
    printF(YELLOW, "%s -> ", fecha);
    printF(GREEN, "Discovery server initiated\n");
    printF(YELLOW, "Listening for incoming connections...\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int* client_fd_ptr;

        // Aceptar conexiones de Poole
        int client_fd = accept(poole_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd >= 0) {
            client_fd_ptr = malloc(sizeof(int));
            *client_fd_ptr = client_fd;
            if (pthread_create(&poole_thread_id, NULL, handlePooleConnection, client_fd_ptr) == 0) {
                poole_thread_created = 1;
                pthread_detach(poole_thread_id);
            }
        }

        // Aceptar conexiones de Bowman
        client_fd = accept(bowman_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd >= 0) {
            client_fd_ptr = malloc(sizeof(int));
            *client_fd_ptr = client_fd;
            if (pthread_create(&bowman_thread_id, NULL, handleBowmanConnection, client_fd_ptr) == 0) {
                bowman_thread_created = 1;
                pthread_detach(bowman_thread_id);
            }
        
        }

        usleep(10000); // Esperar 10ms (pequeño retardo para reducir el uso de CPU)
    }

    free_all_dynamic_memory();

    return EXIT_SUCCESS;
}