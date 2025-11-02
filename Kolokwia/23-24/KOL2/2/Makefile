CC = gcc
CFLAGS = -Wall -std=c11
LDFLAGS = -lrt -lpthread

.PHONY: all clean

all: wys1 odb1

wys1: wys1.c
	$(CC) $(CFLAGS) -o wys1 wys1.c $(LDFLAGS)

odb1: odb1.c
	$(CC) $(CFLAGS) -o odb1 odb1.c $(LDFLAGS)

clean:
	rm -f wys1 odb1