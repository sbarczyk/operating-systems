CC = gcc
CFLAGS = -Wall -Wextra -std=c99

NAME = BarczykSzymon-cw04
ARCHIVE = $(NAME).tar.gz

SRC := $(wildcard *.c) $(wildcard *.h) $(wildcard *.sh) $(wildcard *.txt) Makefile

all: task01 task02

task01: task01.c
	$(CC) $(CFLAGS) -o task01 task01.c

task02: task02.c
	$(CC) $(CFLAGS) -o task02 task02.c

clean:
	rm -f task01 task02 $(ARCHIVE)

pack: clean
	tar cvzf $(ARCHIVE) $(SRC)