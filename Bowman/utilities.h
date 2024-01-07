#ifndef UTILITIES_H
#define UTILITIES_H

#include "../global.h"

/**
 * @brief Parse the command to remove the extra spaces
 * @example "LIST             SONGS" -> "LIST SONGS
 * 
 * @param command Command to parse
 * @return char* Parsed command
*/
char *parse_command(char *command);

/**
 * @brief Remove the extra spaces from the command
 * 
 * @param str Command to trim
 * 
 * @return char* Trimmed command
*/
char *trim_whitespace(char* str);

/**
 * @brief Cuenta la cantidad de canciones en una cadena de texto
 * 
 * @param str Cadena de texto
 * 
 * @return int Cantidad de canciones
*/
int countSongs(const char *str);

/**
 * @brief Cuenta la cantidad de listas de reproducción en una cadena de texto
 * 
 * @param str Cadena de texto
 * 
 * @return int Cantidad de listas de reproducción
*/
int countPlaylists(const char *str);

/**
 * @brief Parsea la información de una canción y la almacena en una estructura Song
 * 
 * @param str Cadena de texto con la información de la canción
 * @param song Estructura Song donde se almacenará la información
 * 
 * @return void
*/
void parseSongInfo(const char *str, Song *song);

/**
 * @brief Parsea la información del servidor Poole y la almacena en variables globales
 * 
 * @param server_info Información del servidor Poole
 * 
 * @return void
*/
void parse_and_store_server_info(const char* server_info);

/**
 * @brief Verifica si el nombre de un archivo corresponde a una lista de reproducción
 * 
 * @param name Nombre del archivo
 * 
 * @return int 0 si es una canción, 1 si es una lista de reproducción
*/
int check_if_playlist(char *name);

/**
 * @brief Añade una canción al arreglo de canciones descargándose
 * 
 * @param newSong Canción a añadir
 * @param thread_id ID del hilo que descarga la canción
 * 
 * @return void
*/
void addSongToArray(Song newSong, pthread_t thread_id);

/**
 * @brief Elimina una canción del arreglo de canciones descargándose
 * 
 * @param song Canción a eliminar
 * 
 * @return void
*/
void deleteSongFromArray(Song song);

/**
 * @brief Libera la memoria dinámica del arreglo de canciones descargándose
 * 
 * @return void
*/
void freeSongDownloadingArray();

#endif