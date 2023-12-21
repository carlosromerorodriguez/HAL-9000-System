CC = gcc -g
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lpthread
BOWMAN_OBJS = Bowman/bowman.o Bowman/config.o Bowman/command_handler.o global.o 
POOLE_OBJS = Poole/poole.o Poole/config.o Poole/command_handler.o Poole/monolit.o global.o
DISCOVERY_OBJS = Discovery/discovery.o Discovery/config.o global.o

# Regla por defecto
all: Bowman/bowman Poole/poole Discovery/discovery clean_objs

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

Bowman/command_handler.o: Bowman/command_handler.c Bowman/command_handler.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Poole/poole.o: Poole/poole.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Poole/config.o: Poole/config.c Poole/config.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Poole/command_handler.o: Poole/command_handler.c Poole/command_handler.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Poole/monolit.o: Poole/monolit.c Poole/monolit.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

global.o: global.c global.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Discovery/discovery.o: Discovery/discovery.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

Discovery/config.o: Discovery/config.c Discovery/config.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

# Ambos comandos son para limpiar todos los ejecutables (make clean)
clean_objs:
	rm -f $(BOWMAN_OBJS) $(POOLE_OBJS) $(DISCOVERY_OBJS)

clean: clean_objs
	rm -f Bowman/bowman Poole/poole Discovery/discovery