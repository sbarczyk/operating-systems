CC      := gcc
CFLAGS  := -Wall -Wextra -O2

SERVER_SRC := server.c
CLIENT_SRC := client.c
SERVER_BIN := server
CLIENT_BIN := client
ARCHIVE_NAME=BarczykSzymon-cw12.tar.gz

# — główny cel —
all: $(SERVER_BIN) $(CLIENT_BIN)

# — serwer potrzebuje wątki POSIX —
$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) -pthread -o $@ $<

# — klient bez dodatkowych bibliotek —
$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $<

# — porządkowanie —
.PHONY: clean
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) *.o $(ARCHIVE_NAME)

pack:
	tar cvzf $(ARCHIVE_NAME) $(SERVER_SRC) $(CLIENT_SRC) Makefile