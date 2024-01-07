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
 * @brief Fill the Bowman configuration with the data from the file
 * 
 * @param bowman_config Bowman configuration to fill
 * @param filename File to read the configuration from
 * @return unsigned char 1 if the configuration was filled correctly, 0 otherwise
*/
unsigned char fill_bowman_config(BowmanConfig *bowman_config, char filename[1]) {
    int fd;
    char *aux;

    fd = open(filename, O_RDONLY);

    if (fd == -1) {
        printF(RED, "ERROR: Failed to open the Bowman file\n");
        return 0;
    }

    bowman_config->username = read_until_delimiter(fd, '\n'); 
        remove_ampersand(bowman_config->username);
        
    bowman_config->folder_path = read_until_delimiter(fd, '\n');
    bowman_config->discovery_ip = read_until_delimiter(fd, '\n');
        
    aux = read_until_delimiter(fd, '\n');
    bowman_config->discovery_port = atoi(aux);
    free(aux);

    close(fd);

    return 1;
}