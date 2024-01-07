#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include "../global.h"
#include "utilities.h"

/**
 * @brief Crea un socket y se conecta al servidor Poole asignado
 * 
 * @param psocket Puntero al descriptor del socket
 * 
 * @return void 
*/
void connect_to_server(int *psocket, int global_server_port, char *global_server_ip, int poole_socket);

/**
 * @brief Establece una conexión con el servidor Discovery
 * 
 * @param config Configuración de Bowman que contiene la IP y el puerto del servidor Discovery
 * @return int Descriptor del socket si la conexión es exitosa, EXIT_FAILURE en caso de error
*/
int connect_to_discovery(char *username, char *discovery_ip, int discovery_port);

/**
 * @brief Crea una trama EXIT y la envía al servidor Poole
 * 
 * @param username Nombre de usuario
 * @param discovery_ip Dirección IP del servidor Discovery
 * @param discovery_port Puerto del servidor Discovery
 * 
 * @return EXIT_SUCCESS si el logout es exitoso, EXIT_FAILURE en caso contrario
*/
int disconnect_notification_to_discovery(char *username, char *discovery_ip, int discovery_port, int global_server_port, char *global_server_ip);

/**
 * @brief Conecta a un servidor Poole alternativo en caso de fallo del servidor actual.
 *
 * Primero intenta establecer una conexión con el servidor Discovery usando las credenciales
 * proporcionadas y luego establece una conexión con un servidor Poole asignado por el servidor
 * Discovery. Gestiona la creación y envío de tramas para el inicio de sesión y la recepción de
 * tramas de respuesta. En caso de éxito, lanza un hilo para escuchar las tramas del servidor Poole.
 *
 * @param username Nombre de usuario para la conexión.
 * @param discovery_ip Dirección IP del servidor Discovery.
 * @param port Puerto del servidor Discovery.
 * @param psocket Puntero al descriptor del socket de Poole.
 * @param discovery_socket Descriptor del socket de Discovery (ya conectado).
 * @param poole_socket Descriptor del socket de Poole (ya conectado).
 * @param msg_id Identificador para la cola de mensajes.
 * @param global_server_name Nombre global del servidor Poole asignado.
 * @param global_server_port Puerto global del servidor Poole asignado.
 * @param global_server_ip Dirección IP global del servidor Poole asignado.
 * 
 * @return 1 en caso de éxito, 0 en caso de error.
 */
int connect_to_another_server(char *username, char *discovery_ip, int port, int *psocket, int discovery_socket, int poole_socket, int msg_id, char* global_server_name, int global_server_port, char *global_server_ip);

/**
 * @brief Establece una conexión con un servidor Poole.
 *
 * Utiliza las variables globales global_server_ip y global_server_port para
 * configurar la dirección y puerto del servidor. Almacena el descriptor del
 * socket en la variable global poole_socket_for_bowman.
 *
 * @return Descriptor del socket conectado en caso de éxito, o -1 en caso de error.
 */
int connect_to_poole_server();

#endif