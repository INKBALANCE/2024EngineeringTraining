# Makefile
CC = gcc
CFLAGS = -Wall
TARGETS = server client

all: $(TARGETS)

server: SelectServer.c
	$(CC) $(CFLAGS) -o server SelectServer.c

client: Client.c
	$(CC) $(CFLAGS) -o client Client.c

clean:
	rm -f $(TARGETS)

