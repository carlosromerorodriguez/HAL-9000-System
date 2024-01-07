#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include "../global.h"
#include "utilities.h"
#include "display_response_handler.h"

void download(char *name);

void clearDownloadedSongs();

void handleNewFile(char* data);

void handleFileData(char* data);

void* startSongDownload(void* args);

void printAllSongsDownloading();

#endif