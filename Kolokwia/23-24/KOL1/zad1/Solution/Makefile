CC = gcc
CFLAGS = -Wall -O0

all: bibl1.so zad1

bibl1.o: bibl1.c
	$(CC) $(CFLAGS) -c -fPIC bibl1.c -o bibl1.o

bibl1.so: bibl1.o
	$(CC) $(CFLAGS) -shared bibl1.o -o bibl1.so

zad1.o: zad1.c
	$(CC) $(CFLAGS) -c zad1.c -o zad1.o

zad1: zad1.o bibl1.so
	$(CC) $(CFLAGS) zad1.o -o zad1

clean:
	rm -f *.o *.so zad1 