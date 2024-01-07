#ifndef MONOLIT_H
#define MONOLIT_H

#include "../global.h"

extern volatile sig_atomic_t server_sigint_received;

void throwMonolitServer(int read_pipe);

#endif // MONOLIT_H