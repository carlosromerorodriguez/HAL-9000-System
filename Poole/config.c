#include "config.h"

/**
 * @brief Removes all the ampersands from a string (username)
 * 
 * @param str String to remove the ampersand from
 * @return void
*/
static void remove_ampersand(char *str) {
    char *amp;
    while ((amp = strchr(str, '&')) != NULL) {
        memmove(amp, amp + 1, strlen(amp));
    }
    free(amp);
}

/**
 * @brief Fill the Poole configuration with the data from the file
 * 
 * @param poole_config Poole configuration to fill
 * @param filename File to read the configuration from
 * @return unsigned char 1 if the configuration was filled correctly, 0 otherwise
*/
unsigned char fill_poole_config(PooleConfig *poole_config, char filename[1]) {
    int fd;
    char *fullPath, *aux;

    asprintf(&fullPath, "../%s", filename);

    fd = open(fullPath, O_RDONLY);
    free(fullPath);

    if (fd == -1) {
        printF(RED, "ERROR: Failed to open the Poole file\n");
        return 0;
    }

    poole_config->username = read_until_delimiter(fd, '\n'); 
        remove_ampersand(poole_config->username);

    poole_config->folder_path = read_until_delimiter(fd, '\n');
    poole_config->discovery_ip = read_until_delimiter(fd, '\n');
        
    aux = read_until_delimiter(fd, '\n');
    poole_config->discovery_port = atoi(aux);
    free(aux);

    poole_config->poole_ip = read_until_delimiter(fd, '\n');

    aux = read_until_delimiter(fd, '\n');
    poole_config->poole_port = atoi(aux);
    free(aux);

    close(fd);

    return 1;
}