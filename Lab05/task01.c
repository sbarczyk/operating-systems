#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

void handler(int signum) {
    write(STDOUT_FILENO, "Odebrano SIGUSR1\n", 17);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Użycie: %s {none|ignore|handler|mask}\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char *option = argv[1];

    if (strcmp(option, "ignore") == 0) {
        if (signal(SIGUSR1, SIG_IGN) == SIG_ERR) {
            perror("Błąd ustawiania ignorowania SIGUSR1");
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(option, "handler") == 0) {
        if (signal(SIGUSR1, handler) == SIG_ERR) {
            perror("Błąd instalacji handlera dla SIGUSR1");
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(option, "mask") == 0) {
        sigset_t newmask, oldmask;
        sigemptyset(&newmask);
        sigaddset(&newmask, SIGUSR1);
        if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) == -1) {
            perror("Błąd maskowania SIGUSR1");
            exit(EXIT_FAILURE);
        }
    }

    if (raise(SIGUSR1) != 0) {
        perror("Błąd wysłania SIGUSR1");
        exit(EXIT_FAILURE);
    }
    
    if (strcmp(option, "mask") == 0) {
        sigset_t pending;
        sigemptyset(&pending);
        if (sigpending(&pending) == -1) {
            perror("Błąd pobierania zbioru oczekujących sygnałów");
            exit(EXIT_FAILURE);
        }
        if (sigismember(&pending, SIGUSR1)) {
            printf("SIGUSR1 oczekuje na odblokowanie\n");
        } else {
            printf("SIGUSR1 nie oczekuje na odblokowanie(0)\n");
        }
    }
    return 0;
}