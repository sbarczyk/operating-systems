#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Sprawdzamy, czy został podany dokładnie jeden argument
    if (argc != 2) {
        fprintf(stderr, "Użycie: %s liczba_procesów\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Liczba procesów musi być dodatnia.\n");
        return 1;
    }

    // Tworzymy n procesów potomnych
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            // W razie błędu przy tworzeniu procesu wypisujemy komunikat
            fprintf(stderr, "Błąd przy tworzeniu procesu.\n");
            return 1;
        } 
        if (pid == 0) { 
            // Proces potomny:
            // Wypisujemy identyfikatory oraz wartość zmiennej i i jej adres.
            printf("Proces macierzysty: %d, Proces potomny: %d, i = %d, adres i = %p\n",
                   getppid(), getpid(), i, (void *)(&i));
            return 0; // Proces potomny kończy działanie
        }
    }

    // Proces macierzysty czeka na zakończenie wszystkich procesów potomnych
    while (wait(NULL) > 0);

    // Na końcu wypisujemy argv[1] – liczbę utworzonych procesów potomnych
    printf("%s\n", argv[1]);

    return 0;
}