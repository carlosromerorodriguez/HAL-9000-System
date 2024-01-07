#ifndef RECEIVE_POOLE_FRAMES_H
#define RECEIVE_POOLE_FRAMES_H

// Importación de librerías
#include "../global.h"
#include "display_response_handler.h"
#include "download_manager.h"
#include "connection_handler.h"
#include "session_handler.h"

/**
 * @brief Función del hilo que recibe y maneja tramas del servidor Poole.
 *
 * Este hilo se ejecuta constantemente en segundo plano, recibiendo tramas del servidor Poole y respondiendo adecuadamente.
 * Maneja diferentes tipos de tramas como respuestas a canciones, listas de reproducción, datos de archivo, y comandos de cierre de sesión o de apagado del servidor.
 *
 * @param args Estructura que contiene los parámetros necesarios para la función, incluyendo el socket de Poole y datos de usuario.
 * @return void* Retorna NULL al finalizar la ejecución del hilo.
 */
void *receive_frames(void *args);

#endif