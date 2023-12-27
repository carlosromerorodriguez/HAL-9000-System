#ifndef CONFIG_BOWMAN_H
#define CONFIG_BOWMAN_H

#include "../global.h"

// Structure to store the Bowman's configuration
typedef struct {
    char *username;
    char *folder_path;
    char *discovery_ip;
    int discovery_port;
} BowmanConfig;

/**
 * @brief Fill the Bowman configuration with the data from the file
 * 
 * @param bowman_config Bowman configuration to fill
 * @param filename File to read the configuration from
 * @return unsigned char 1 if the configuration was filled correctly, 0 otherwise
*/
unsigned char fill_bowman_config(BowmanConfig *bowman_config, char filename[1]);

#endif // !CONFIG_BOWMAN_H
