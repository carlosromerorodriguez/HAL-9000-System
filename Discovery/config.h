#ifndef CONFIG_H
#define CONFIG_H

// Librerias
#include "../global.h"

/**
 * @brief Lee todo el archivo Discovery y lo almacen en una estructura DiscoveryConfig
 * 
 * @param filename Nombre del archivo a leer
 * @param config Estructura donde se almacenara la informacion
 * 
 * @return 1 si se pudo leer el archivo correctamente, 0 en caso contrario
*/
unsigned char readDiscoveryConfig(const char* filename, DiscoveryConfig* config);

#endif // CONFIG_H