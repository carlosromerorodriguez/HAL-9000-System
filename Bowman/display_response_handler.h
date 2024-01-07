#ifndef DISPLAY_RESPONSE_HANDLER_H
#define DISPLAY_RESPONSE_HANDLER_H

#include "../global.h"
#include "utilities.h"

void list_songs();

void list_playlists();

void handleListSongsSize(char* data);

void handleSongsResponse(char *data);

void handleListPlaylistsSize(char *data);

void handlePlaylistsResponse(char *data);

void print_playlists(char *to_print);

void printSongsInPlaylists(char *playlist, char playlistIndex);

void printSongDownloading(Song_Downloading songDownloading);

#endif