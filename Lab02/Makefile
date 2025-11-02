CC = gcc
CFLAGS = -Wall -Wextra -O2

LIB_STATIC = libcollatz.a
LIB_SHARED = libcollatz.so


all: client_static client_shared client_dynamic

# KOMPILACJA DO BIBLIOTEKI STATYCZNEJ

# Obiekt do biblioteki statycznej (bez -fPIC)
collatz_static.o: collatz.c
	$(CC) $(CFLAGS) -c collatz.c -o collatz_static.o

$(LIB_STATIC): collatz_static.o
	ar rcs $(LIB_STATIC) collatz_static.o

# KOMPILACJA DO BIBLIOTEKI WSPÓŁDZIELONEJ

# Obiekt do współdzielonej (z -fPIC)
collatz_shared.o: collatz.c
	$(CC) $(CFLAGS) -fPIC -c collatz.c -o collatz_shared.o

$(LIB_SHARED): collatz_shared.o
	$(CC) -shared -o $(LIB_SHARED) collatz_shared.o

# KLIENT STATYCZNY

client_static: client.c $(LIB_STATIC)
	$(CC) $(CFLAGS) -o client_static client.c $(LIB_STATIC)

# KLIENT WSPÓŁDZIELONY

client_shared: client.c $(LIB_SHARED)
	$(CC) $(CFLAGS) -o client_shared client.c -L. -lcollatz

# KLIENT DYNAMICZNY (dlopen)

client_dynamic: client.c $(LIB_SHARED)
	$(CC) $(CFLAGS) -DDYNAMIC_LOADING -o client_dynamic client.c -ldl

# OSOBNE CELE DO RĘCZNEGO TWORZENIA BIBLIOTEK

static_lib: $(LIB_STATIC)

shared_lib: $(LIB_SHARED)


clean:
	rm -f *.o $(LIB_STATIC) $(LIB_SHARED) client_static client_shared client_dynamic