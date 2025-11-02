CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread

ARCHIVE_NAME=BarczykSzymon-cw10.tar.gz
TARGET = lab10
SRC = lab10.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET) *.o

pack:
	tar cvzf $(ARCHIVE_NAME) $(SRC) Makefile

.PHONY: all clean