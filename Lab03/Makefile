TARGET = flipper
CC = gcc
CFLAGS = -Wall -Wextra -O2
SRC = flipper.c
OBJ = $(SRC:.c=.o)
OUTPUT_DIR = output
INPUT_DIR = art

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) $(INPUT_DIR) $(OUTPUT_DIR)

clean:
	rm -f $(OBJ) $(TARGET)
	rm -rf $(OUTPUT_DIR)

help:
	@echo "Dostępne komendy:"
	@echo "  make            - kompilacja programu"
	@echo "  make run        - uruchomienie programu z katalogami 'art' (wejście) i 'output' (wyjście)"
	@echo "  make clean      - usunięcie plików binarnych, obiektowych oraz katalogu wynikowego 'output'"
	@echo "  make help       - wyświetlenie tej pomocy"