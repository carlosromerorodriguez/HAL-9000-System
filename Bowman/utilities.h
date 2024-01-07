#ifndef UTILITIES_H
#define UTILITIES_H

#include "../global.h"

char *parse_command(char *command);

char *trim_whitespace(char* str);

int countSongs(const char *str);

int countPlaylists(const char *str);

void parseSongInfo(const char *str, Song *song);

void parse_and_store_server_info(const char* server_info);

int check_if_playlist(char *name);

void addSongToArray(Song newSong, pthread_t thread_id);

void deleteSongFromArray(Song song);

void freeSongDownloadingArray();

#endif