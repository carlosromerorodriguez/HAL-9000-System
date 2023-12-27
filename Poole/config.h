#ifndef CONFIG_POOLE_H
#define CONFIG_POOLE_H

#include "../global.h"

// Estructura para almacenar la configuracion de Poole
typedef struct {
    char *username;
    char *folder_path;
    char *discovery_ip;
    int discovery_port;
    char *poole_ip;
    int poole_port;
} PooleConfig;

/**
 * @brief Fill the Poole configuration with the data from the file
 * 
 * @param poole_config Poole configuration to fill
 * @param filename File to read the configuration from
 * @return unsigned char 1 if the configuration was filled correctly, 0 otherwise
*/
unsigned char fill_poole_config(PooleConfig *poole_config, char filename[1]);

#endif // !CONFIG_POOLE_H