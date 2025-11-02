CC = gcc
CFLAGS = -fPIC
DLFLAGS = -ldl

LIB_SRC = bibl1.c
LIB_OBJ = bibl1.o
LIB_SO = bibl1.so
MAIN_SRC = zad1.c
MAIN_BIN = zad1

all: $(LIB_SO) $(MAIN_BIN)

$(MAIN_BIN): $(MAIN_SRC)
	$(CC) -o $(MAIN_BIN) $(MAIN_SRC) $(DLFLAGS)

$(LIB_OBJ): $(LIB_SRC)
	$(CC) $(CFLAGS) -c $(LIB_SRC)

$(LIB_SO): $(LIB_OBJ)
	$(CC) -shared -o $(LIB_SO) $(LIB_OBJ)

clean:
	rm -f *.o *.so $(MAIN_BIN)

