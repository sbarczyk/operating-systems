CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2

TARGETS = task01 program1 program2
PACKNAME = BarczykSzymon-cw06.tar.gz

all: $(TARGETS)

task01: task01.c
	$(CC) $(CFLAGS) -o task01 task01.c

task02: program1 program2

program1: program1.c
	$(CC) $(CFLAGS) -o program1 program1.c

program2: program2.c
	$(CC) $(CFLAGS) -o program2 program2.c

clean:
	rm -f $(TARGETS)
	rm -f $(PACKNAME)

pack: clean
	tar cvzf $(PACKNAME) Makefile task01.c program1.c program2.c

.PHONY: all clean pack