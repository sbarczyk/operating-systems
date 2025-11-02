# Kompilacja
CC=gcc
CFLAGS=-Wall -Wextra

ARCHIVE_NAME=BarczykSzymon-cw09.tar.gz

all: integral

user: integral.c
	$(CC) $(CFLAGS) -o integral integral.c

clean:
	rm -f integral

pack:
	tar cvzf $(ARCHIVE_NAME) integral.c Makefile

.PHONY: all clean pack