CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -g -O0

TARGET = countdown

all: $(TARGET)

countdown: countdown.c
	$(CC) $(CFLAGS) countdown.c -o countdown

clean:
	rm -f $(TARGET)
	rm -f *.o
	rm -rf *.dSYM

.PHONY: all countdown clean