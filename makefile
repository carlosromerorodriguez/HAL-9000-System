CC = gcc -g
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lpthread
BOWMAN_OBJS = Bowman/bowman.o Bowman/config.o Bowman/connection_handler.o Bowman/display_response_handler.o Bowman/download_manager.o Bowman/receive_poole_frames.o Bowman/session_handler.o Bowman/utilities.o global.o
POOLE_OBJS = Poole/poole.o Poole/config.o Poole/command_handler.o Poole/monolit.o global.o
DISCOVERY_OBJS = Discovery/discovery.o Discovery/config.o Discovery/connection_handler.o Discovery/poole_server_manager.o Discovery/user_manager.o Discovery/utilities.o global.o

# Regla por defecto
all: global.o Bowman/bowman Poole/poole Discovery/discovery clean_objs

global.o: global.c global.h
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para Bowman
Bowman/bowman: $(BOWMAN_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Regla para Poole
Poole/poole: $(POOLE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Regla para Discovery
Discovery/discovery: $(DISCOVERY_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Reglas para compilar los archivos objeto
Bowman/bowman.o: Bowman/bowman.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Bowman/config.o: Bowman/config.c Bowman/config.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Bowman/connection_handler.o: Bowman/connection_handler.c Bowman/connection_handler.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Bowman/display_response_handler.o: Bowman/display_response_handler.c Bowman/display_response_handler.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Bowman/download_manager.o: Bowman/download_manager.c Bowman/download_manager.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Bowman/receive_poole_frames.o: Bowman/receive_poole_frames.c Bowman/receive_poole_frames.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Bowman/session_handler.o: Bowman/session_handler.c Bowman/session_handler.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Bowman/utilities.o: Bowman/utilities.c Bowman/utilities.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Poole/poole.o: Poole/poole.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Poole/config.o: Poole/config.c Poole/config.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Poole/command_handler.o: Poole/command_handler.c Poole/command_handler.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Poole/monolit.o: Poole/monolit.c Poole/monolit.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Discovery/discovery.o: Discovery/discovery.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Discovery/config.o: Discovery/config.c Discovery/config.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Discovery/connection_handler.o: Discovery/connection_handler.c Discovery/connection_handler.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Discovery/poole_server_manager.o: Discovery/poole_server_manager.c Discovery/poole_server_manager.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Discovery/user_manager.o: Discovery/user_manager.c Discovery/user_manager.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Discovery/utilities.o: Discovery/utilities.c Discovery/utilities.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

# Ambos comandos son para limpiar todos los ejecutables (make clean)
clean_objs:
	rm -f $(BOWMAN_OBJS) $(POOLE_OBJS) $(DISCOVERY_OBJS) global.o

clean: clean_objs
	rm -f Bowman/bowman Poole/poole Discovery/discovery 