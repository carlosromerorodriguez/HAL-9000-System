#ifndef CONNECTION_HANDLER_2_H
#define CONNECTION_HANDLER_2_H

// Librerias
#include "../global.h"
#include "poole_server_manager.h"
#include "user_manager.h"

extern volatile sig_atomic_t poole_thread_active;
extern volatile sig_atomic_t bowman_thread_active;
extern char fecha[20];

void* handlePooleConnection(void* arg);

void* handleBowmanConnection(void* arg);

#endif 