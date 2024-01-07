#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include "../global.h"
#include "utilities.h"
#include "display_response_handler.h"

/**
 * @brief Inicia la descarga de una canción o una lista de reproducción.
 *
 * Envía una trama al servidor Poole para solicitar la descarga de una canción o una lista de reproducción.
 * 
 * @param name Nombre de la canción o lista de reproducción a descargar.
 */
void download(char *name);

/**
 * @brief Limpia las canciones completamente descargadas de la lista de descargas.
 *
 * Revisa la lista de canciones descargándose y elimina aquellas que han sido completamente descargadas.
 */
void clearDownloadedSongs();

/**
 * @brief Función ejecutada en un hilo para manejar la descarga de una canción.
 *
 * Procesa la información de la canción, crea un archivo y escribe los datos recibidos en él.
 * 
 * @param args Argumentos pasados al hilo, incluyendo la información de la canción.
 * @return void* Puntero a NULL al finalizar la ejecución del hilo.
 */
void* startSongDownload(void* args);

/**
 * @brief Maneja la recepción de un nuevo archivo desde el servidor Poole.
 *
 * Crea un hilo para manejar la descarga del archivo y procesa los datos recibidos.
 * 
 * @param data Datos del archivo recibido.
 */
void handleNewFile(char* data);

/**
 * @brief Maneja los datos de archivo recibidos del servidor Poole.
 *
 * Almacena los datos del archivo en una cola de mensajes para su posterior procesamiento.
 * 
 * @param data Datos del archivo recibido.
 */
void handleFileData(char* data);

/**
 * @brief Imprime todas las canciones que están siendo descargadas actualmente.
 *
 * Muestra el estado de descarga de cada canción en la lista de descargas.
 */
void printAllSongsDownloading();

#endif