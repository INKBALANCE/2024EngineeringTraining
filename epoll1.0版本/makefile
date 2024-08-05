# Makefile
CC = gcc
CFLAGS = -Wall
TARGETS = server client

all: $(TARGETS)

server: Server.c
	$(CC) $(CFLAGS) -o server Server.c

client: Client.c
	$(CC) $(CFLAGS) -o client Client.c

clean:
	rm -f $(TARGETS)

