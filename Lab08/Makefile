# Kompilacja
CC=gcc
CFLAGS=-Wall -Wextra -pthread


ARCHIVE_NAME=BarczykSzymon-cw08.tar.gz


all: user printer cleanup

user: user.c
	$(CC) $(CFLAGS) -o user user.c

printer: printer.c
	$(CC) $(CFLAGS) -o printer printer.c

cleanup: cleanup.c
	$(CC) $(CFLAGS) -o cleanup cleanup.c

clean:
	./cleanup
	rm -f user printer cleanup

pack:
	tar cvzf $(ARCHIVE_NAME) user.c printer.c cleanup.c Makefile

.PHONY: all clean package