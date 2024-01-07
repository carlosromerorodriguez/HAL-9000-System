#ifndef MONOLIT_H
#define MONOLIT_H

#include "../global.h"

// Variables globales importadas de otros TADS
extern volatile sig_atomic_t server_sigint_received;

/**
 * @brief Lee datos de la pipe y los procesa en el servidor Monolit.
 * 
 * Esta función lee continuamente de un pipe especificado. Utiliza un buffer dinámico para
 * acumular datos leídos del pipe. Una vez que se detecta el final de una cadena de datos,
 * procesa estos datos (por ejemplo, actualizando estadísticas) y luego reinicia el buffer 
 * para la siguiente lectura.
 * 
 * @param read_pipe El file descriptr de archivo del pipe desde el cual se leen los datos.
 * 
 * @note La función maneja errores de lectura y problemas de memoria. También utiliza un mutex
 *       para sincronizar el acceso al archivo de estadísticas durante su actualización.
 *       Termina la ejecución si se recibe una señal de interrupción.
 */
void throwMonolitServer(int read_pipe);

#endif // MONOLIT_H