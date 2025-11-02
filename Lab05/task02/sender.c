#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t ack_received = 0;

// Handler dla SIGUSR1 – potwierdzenie odbioru od catchera
void sigusr1_handler(int sig)
{
    (void)sig;
    ack_received = 1;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Użycie: %s <PID catchera> <tryb>\n", argv[0]);
        exit(1);
    }

    pid_t catcher_pid = atoi(argv[1]);
    int mode = atoi(argv[2]);

    // Ustawienie handlera dla SIGUSR1
    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        perror("sigaction SIGUSR1");
        exit(1);
    }

    // Blokujemy SIGUSR1, aby móc użyć sigsuspend do oczekiwania
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &mask, &oldmask) == -1)
    {
        perror("sigprocmask");
        exit(1);
    }

    // Wysyłamy sygnał SIGUSR1 do catchera wraz z trybem (używamy sigqueue)
    union sigval sv;
    sv.sival_int = mode;
    if (sigqueue(catcher_pid, SIGUSR1, sv) == -1)
    {
        perror("sigqueue");
        exit(1);
    }

    // Oczekujemy na potwierdzenie – odblokowujemy SIGUSR1 w sigsuspend
    sigset_t wait_mask;
    sigemptyset(&wait_mask);
    while (!ack_received)
    {
        sigsuspend(&wait_mask);
    }

    // Przywracamy poprzednią maskę sygnałów
    sigprocmask(SIG_SETMASK, &oldmask, NULL);

    printf("Odebrano potwierdzenie od catchera.\n");

    return 0;
}