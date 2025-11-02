CC = gcc
CFLAGS = -Wall -O0

bibl1.o: bibl1.c
	$(CC) $(CFLAGS) -fPIC -c bibl1.c -o bibl1.o

bibl1.so: bibl1.o
	$(CC) $(CFLAGS) -shared bibl1.o -o bibl1.so

zad1.o: zad1.c
	$(CC) $(CFLAGS) -c zad1.c -o zad1.o

zad1: zad1.o
	$(CC) $(CFLAGS) zad1.o -ldl -o zad1