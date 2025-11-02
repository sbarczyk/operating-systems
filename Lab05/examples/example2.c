#include <stdio.h>
#include <signal.h> 


int main(void){
    sigset_t newmask, oldmask, set;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &newmask, &oldmask);

    raise(SIGUSR1);
    sigpending(&set);
    if (sigismember(&set, SIGUSR1) == 1){
        printf("SIGUSR1 oczekuje na odblokowanie(1)\n");
    }
}