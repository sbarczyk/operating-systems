#include <stdio.h>
#include <signal.h> 
#include <unistd.h>

void handler_v1(int signum, siginfo_t * si, void * p3){
    printf("Obsługa sygnału v1: %d, %d, war: %d\n", si->si_pid, si->si_uid, si->si_value);
}

int main(void){
    sigset_t set;
    struct sigaction act;
    printf("pid: %d\n", getpid());

    act.sa_sigaction=handler_v1;
    sigemptyset(&act.sa_mask);
    act.sa_flags=SA_SIGINFO;
    sigaction(SIGUSR1, &act, NULL);
}

