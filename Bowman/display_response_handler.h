#ifndef DISPLAY_RESPONSE_HANDLER_H
#define DISPLAY_RESPONSE_HANDLER_H

#include "../global.h"
#include "utilities.h"

/**
 * @brief Crea una trama LIST_SONGS y la envía al servidor Poole
 * 
 * @return void
*/
void list_songs();

/**
 * @brief Crea una trama LIST_PLAYLISTS y la envía al servidor Poole
 * 
 * @return void
*/
void list_playlists();

/**
 * @brief Maneja la respuesta del tamaño de la lista de canciones.
 *
 * Copia los datos recibidos a una nueva cadena, crea un buffer de mensaje con estos datos y los escribe en la cola de mensajes.
 * 
 * @param data Cadena que contiene la información sobre el tamaño de la lista de canciones.
 */
void handleListSongsSize(char* data);

/**
 * @brief Procesa la respuesta recibida con la información de las canciones.
 *
 * Realiza una copia de los datos recibidos, crea un buffer de mensaje con esta copia y lo envía a la cola de mensajes.
 * 
 * @param data Cadena con los datos de la respuesta sobre las canciones.
 */
void handleSongsResponse(char *data);

/**
 * @brief Maneja la respuesta con el tamaño de la lista de reproducción.
 *
 * Copia los datos recibidos en un nuevo buffer, crea un mensaje con estos datos y lo envía a la cola de mensajes.
 * 
 * @param data Cadena con los datos de la respuesta sobre el tamaño de la lista de reproducción.
 */
void handleListPlaylistsSize(char *data);

/**
 * @brief Procesa la respuesta de listas de reproducción y la envía a la cola de mensajes.
 *
 * Esta función crea una copia de los datos recibidos, los almacena en un mensaje y los coloca en la cola de mensajes.
 * 
 * @param data Cadena con los datos de la respuesta sobre listas de reproducción.
 */
void handlePlaylistsResponse(char *data);

/**
 * @brief Imprime las listas de reproducción y sus canciones.
 *
 * La función divide la cadena de entrada en listas de reproducción individuales usando '#' como delimitador
 * y llama a la función printSongsInPlaylists para cada lista de reproducción encontrada.
 * 
 * @param to_print Cadena con los datos de las listas de reproducción a imprimir.
 */
void print_playlists(char *to_print);

/**
 * @brief Imprime las canciones de una lista de reproducción específica.
 *
 * Separa la lista de reproducción en su nombre y canciones utilizando '&' como delimitador.
 * Imprime el nombre de la lista de reproducción seguido de un listado de sus canciones.
 * 
 * @param playlist Cadena con los datos de la lista de reproducción y sus canciones.
 * @param playlistIndex Índice numérico de la lista de reproducción.
 */
void printSongsInPlaylists(char *playlist, char playlistIndex);

/**
 * @brief Imprime el estado actual de descarga de una canción.
 *
 * Calcula y muestra el porcentaje de descarga de una canción junto con una barra de progreso visual.
 * La barra de progreso se rellena proporcionalmente al porcentaje de descarga completado.
 * 
 * @param songDownloading Estructura que contiene información sobre la canción y su estado de descarga.
 */
void printSongDownloading(Song_Downloading songDownloading);

#endif