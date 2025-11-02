#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t current_mode = 0;
volatile sig_atomic_t total_requests = 0;
volatile sig_atomic_t mode2_counter = 0;

// Handler dla SIGINT w trybie 4 (wypisuje komunikat)
void sigint_handler(int sig)
{
    printf("Wciśnięto CTRL+C\n");
}

// Handler dla SIGALRM (obsługuje tryb 2 – wypisywanie liczb co sekundę)
void sigalrm_handler(int sig)
{
    if (current_mode == 2)
    {
        printf("%d\n", mode2_counter++);
        alarm(1);
    }
}

// Handler dla SIGUSR1 – odbiera żądanie zmiany trybu i wysyła potwierdzenie do nadawcy
void sigusr1_handler(int sig, siginfo_t *info, void *ucontext)
{
    total_requests++;
    int new_mode = info->si_value.sival_int;
    pid_t sender_pid = info->si_pid;

    if (current_mode == 2 && new_mode != 2)
    {
        alarm(0);
    }

    current_mode = new_mode;

    switch (new_mode)
    {
    case 1:
        printf("Otrzymano tryb 1. Liczba żądań od uruchomienia: %d\n", total_requests);
        break;
    case 2:
        mode2_counter = 1;
        printf("Otrzymano tryb 2. Rozpoczynam numerację:\n");
        alarm(1);
        break;
    case 3:
        printf("Otrzymano tryb 3. Ignorowanie sygnału CTRL+C.\n");
        signal(SIGINT, SIG_IGN);
        break;
    case 4:
        printf("Otrzymano tryb 4. Ustawiono reakcję na CTRL+C.\n");
        struct sigaction sa_int;
        sa_int.sa_handler = sigint_handler;
        sigemptyset(&sa_int.sa_mask);
        sa_int.sa_flags = 0;
        if (sigaction(SIGINT, &sa_int, NULL) == -1)
        {
            perror("sigaction SIGINT");
            exit(1);
        };
        break;
    case 5:
        printf("Otrzymano tryb 5. Kończenie działania catchera.\n");
        kill(sender_pid, SIGUSR1);
        exit(0);
        break;
    default:
        printf("Nieznany tryb: %d\n", new_mode);
        break;
    }
    kill(sender_pid, SIGUSR1);
}

int main()
{
    printf("Catcher PID: %d\n", getpid());

    struct sigaction sa_usr;
    sa_usr.sa_sigaction = sigusr1_handler;
    sa_usr.sa_flags = SA_SIGINFO;
    sigemptyset(&sa_usr.sa_mask);
    if (sigaction(SIGUSR1, &sa_usr, NULL) == -1)
    {
        perror("sigaction SIGUSR1");
        exit(1);
    }

    struct sigaction sa_alrm;
    sa_alrm.sa_handler = sigalrm_handler;
    sigemptyset(&sa_alrm.sa_mask);
    sa_alrm.sa_flags = 0;
    if (sigaction(SIGALRM, &sa_alrm, NULL) == -1)
    {
        perror("sigaction SIGALRM");
        exit(1);
    }

    while (1)
    {
        pause();
    }

    return 0;
}