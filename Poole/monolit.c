#include "monolit.h"

pthread_mutex_t write_file = PTHREAD_MUTEX_INITIALIZER; 

/**
 * @brief Actualiza las estadísticas de reproducción de una canción en un archivo.
 * 
 * Esta función abre y lee el archivo 'stats.txt', buscando una línea correspondiente
 * al nombre de la canción proporcionado. Si encuentra la canción, incrementa su contador
 * de reproducciones. Si la canción no está en el archivo, añade una nueva línea para ella.
 * Utiliza un buffer dinámico para leer y modificar el archivo.
 * 
 * @param songName El nombre de la canción cuyas estadísticas se actualizarán o agregarán.
 * 
 * @note Maneja errores en la apertura del archivo, asignación de memoria y lectura del archivo.
 *       Utiliza funciones de manipulación de strings y acceso a archivos para actualizar las estadísticas.
 */

void updateStatistics(const char *songName) {
    int fd = open("stats.txt", O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        printF(RED, "ERROR: Unable to open stats.txt\n");
        return;
    }

    size_t bufferCapacity = 1024;
    size_t bufferIncrement = 1024;
    char *buffer = malloc(bufferCapacity);
    if (!buffer) {
        printF(RED, "ERROR: Memory allocation failed\n");
        close(fd);
        return;
    }

    ssize_t bytesRead;
    size_t totalBytesRead = 0;
    int found = 0;
    while ((bytesRead = read(fd, buffer + totalBytesRead, bufferCapacity - totalBytesRead - 1)) > 0) {
        totalBytesRead += bytesRead;
        if (totalBytesRead >= bufferCapacity - 1) {
            bufferCapacity += bufferIncrement;
            char *newBuffer = realloc(buffer, bufferCapacity);
            if (!newBuffer) {
                printF(RED, "ERROR: Memory reallocation failed\n");
                free(buffer);
                close(fd);
                return;
            }
            buffer = newBuffer;
        }
    }
    buffer[totalBytesRead] = '\0';

    char *line = strtok(buffer, "\n");
    long pos = 0;
    while (line) {
        if (strncmp(line, songName, strlen(songName)) == 0 && line[strlen(songName)] == ',') {
            found = 1;
            int count = atoi(line + strlen(songName) + 1) + 1;
            int newLineLength = snprintf(NULL, 0, "%s,%d\n", songName, count);
            char *newLine = malloc(newLineLength + 1);
            if (!newLine) {
                printF(RED, "ERROR: Memory allocation failed for new line\n");
                break;
            }
            snprintf(newLine, newLineLength + 1, "%s,%d\n", songName, count);
            lseek(fd, pos, SEEK_SET);
            write(fd, newLine, newLineLength);
            free(newLine);
            break;
        }
        pos += strlen(line) + 1;
        line = strtok(NULL, "\n");
    }

    if (!found) {
        int newLineLength = snprintf(NULL, 0, "%s,1\n", songName);
        char *newLine = malloc(newLineLength + 1);
        if (!newLine) {
            printF(RED, "ERROR: Memory allocation failed for new line\n");
        } else {
            snprintf(newLine, newLineLength + 1, "%s,1\n", songName);
            write(fd, newLine, newLineLength);
            free(newLine);
        }
    }

    free(buffer);
    close(fd);
}

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
void throwMonolitServer(int read_pipe) {
    char *dynamicBuffer = NULL;
    size_t currentSize = 0;

    while (!server_sigint_received) {
        char buffer[1024];
        ssize_t count = read(read_pipe, buffer, sizeof(buffer) - 1);

        if (count == -1) {
            printF(RED, "ERROR: Error reading from pipe\n");
            free(dynamicBuffer);
            break;
        } else if (count == 0) {
            printF(RED, "ERROR: Pipe closed\n");
            break;
        } else {
            // Se ajusta el tamaño del buffer dinámicamente para añadir el nombre de la canción
            char *temp = realloc(dynamicBuffer, currentSize + count + 1);
            if (temp == NULL) {
                printF(RED, "ERROR: Memory allocation failed\n");
                free(dynamicBuffer);
                break;
            }
            dynamicBuffer = temp;
            memcpy(dynamicBuffer + currentSize, buffer, count);
            currentSize += count;
            dynamicBuffer[currentSize] = '\0';

            // Procesar la información recibida si se detecta el final de la cadena
            if (buffer[count - 1] == '\0' || count < (ssize_t)sizeof(buffer) - 1) {
                printF(GREEN, "Received in monolit.c: %s\n", dynamicBuffer);
                
                // Actualizarías stats.txt
                pthread_mutex_lock(&write_file);
                updateStatistics(dynamicBuffer);
                pthread_mutex_unlock(&write_file);

                // Resetear el buffer dinámico para la próxima lectura
                free(dynamicBuffer);
                dynamicBuffer = NULL;
                currentSize = 0;
            }
        }

        sleep(0.05);
    }

    free(dynamicBuffer);
}