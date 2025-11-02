CC = gcc
CFLAGS = -Wall -Wextra -std=c99

TARGETS = server client
SOURCES = server.c client.c Makefile
ARCHIVE = BarczykSzymon-cw07.tar.gz

.PHONY: all clean pack

all: $(TARGETS)

server: server.c
	$(CC) $(CFLAGS) server.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f $(TARGETS) $(ARCHIVE)

pack:
	tar -cvzf $(ARCHIVE) $(SOURCES)