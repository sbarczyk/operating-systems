#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define MAX_LINE 1024

// Funkcja odwracająca znaki w linii
void reverse_line(char *line, size_t length)
{
    size_t len = length;
    if (len > 0 && line[len - 1] == '\n')
        len--;

    for (size_t i = 0; i < len / 2; i++)
    {
        char temp = line[i];
        line[i] = line[len - 1 - i];
        line[len - 1 - i] = temp;
    }
}

// Funkcja przetwarzająca plik
void process_file(const char *src_path, const char *dest_path)
{
    FILE *src = fopen(src_path, "rb");
    if (!src)
    {
        fprintf(stderr, "Błąd otwierania pliku źródłowego '%s': %s\n", src_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    FILE *dest = fopen(dest_path, "wb");
    if (!dest)
    {
        fprintf(stderr, "Błąd tworzenia pliku wynikowego '%s': %s\n", dest_path, strerror(errno));
        fclose(src);
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_LINE];
    size_t len = 0;
    char c;

    while (fread(&c, 1, 1, src) == 1)
    {
        buffer[len++] = c;

        if (c == '\n' || len == MAX_LINE - 1)
        {
            buffer[len] = '\0';

            reverse_line(buffer, len);

            if (fwrite(buffer, 1, len, dest) != len)
            {
                fprintf(stderr, "Błąd zapisu do pliku wynikowego '%s': %s\n", dest_path, strerror(errno));
                fclose(src);
                fclose(dest);
                exit(EXIT_FAILURE);
            }

            len = 0;
        }
    }

    if (ferror(src))
    {
        fprintf(stderr, "Błąd odczytu z pliku '%s': %s\n", src_path, strerror(errno));
        fclose(src);
        fclose(dest);
        exit(EXIT_FAILURE);
    }

    fclose(src);
    fclose(dest);
}

// Funkcja sprawdzająca czy plik ma rozszerzenie ".txt"
int is_text_file(const char *filename)
{
    const char *extension = strrchr(filename, '.');
    if (!extension)
        return 0;
    if (strlen(extension) != 4)
        return 0;
    return strcmp(extension, ".txt") == 0;
}

// Funkcja przeglądająca katalog i przetwarzająca pliki
void process_directory(const char *src_dir, const char *dest_dir)
{
    DIR *dir = opendir(src_dir);
    if (!dir)
    {
        fprintf(stderr, "Błąd otwierania katalogu źródłowego '%s': %s\n", src_dir, strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    struct stat entry_info;
    char src_path[1024], dest_path[1024];

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

        if (stat(src_path, &entry_info) != 0)
        {
            fprintf(stderr, "Błąd odczytu informacji o pliku '%s': %s\n", src_path, strerror(errno));
            closedir(dir);
            exit(EXIT_FAILURE);
        }

        if (S_ISREG(entry_info.st_mode))
        {
            if (is_text_file(entry->d_name))
            {
                printf("Przetwarzanie pliku: %s\n", entry->d_name);
                process_file(src_path, dest_path);
            }
            else
            {
                printf("Pomijanie pliku (nie .txt): %s\n", entry->d_name);
            }
        }
        else
        {
            printf("Pomijanie (nie plik regularny): %s\n", entry->d_name);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Użycie: %s <katalog_źródłowy> <katalog_wynikowy>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Sprawdzenie katalogu źródłowego
    struct stat st;
    if (stat(argv[1], &st) != 0)
    {
        fprintf(stderr, "Katalog źródłowy '%s' nie istnieje lub brak dostępu: %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }
    if (!S_ISDIR(st.st_mode))
    {
        fprintf(stderr, "Ścieżka '%s' nie jest katalogiem.\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Sprawdzenie katalogu wynikowego
    if (stat(argv[2], &st) != 0)
    {
        printf("Katalog wynikowy '%s' nie istnieje. Tworzę...\n", argv[2]);
        if (mkdir(argv[2], 0755) != 0)
        {
            fprintf(stderr, "Błąd tworzenia katalogu '%s': %s\n", argv[2], strerror(errno));
            return EXIT_FAILURE;
        }
    }
    else if (!S_ISDIR(st.st_mode))
    {
        fprintf(stderr, "Ścieżka '%s' nie jest katalogiem.\n", argv[2]);
        return EXIT_FAILURE;
    }

    // Rozpoczęcie przetwarzania
    process_directory(argv[1], argv[2]);

    printf("Przetwarzanie zakończone.\n");
    return EXIT_SUCCESS;
}