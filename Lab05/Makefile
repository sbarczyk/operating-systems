CC = gcc
CFLAGS = -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c99
TARGET = task01
CATCHER = task02/catcher
SENDER = task02/sender

# Nazwa archiwum – zmodyfikuj ją według własnych danych
PACKNAME = BarczykSzymon-cw05.tar.gz

all: $(TARGET) $(CATCHER) $(SENDER)

$(TARGET): task01.c
	$(CC) $(CFLAGS) -o $(TARGET) task01.c

$(CATCHER): task02/catcher.c
	$(CC) $(CFLAGS) -o $(CATCHER) task02/catcher.c

$(SENDER): task02/sender.c
	$(CC) $(CFLAGS) -o $(SENDER) task02/sender.c

clean:
	rm -f $(TARGET) $(CATCHER) $(SENDER)


pack: clean
	tar cvzf $(PACKNAME) makefile task01.c task02