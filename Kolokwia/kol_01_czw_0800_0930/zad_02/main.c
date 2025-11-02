#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void sighandler(int signo, siginfo_t *info, void *context){
    printf("Child received signal %d with value %d\n", signo, info->si_value.sival_int);
}


int main(int argc, char* argv[]) {

    if(argc != 3){
        printf("Not a suitable number of program parameters\n");
        return 1;
    }

    int value = atoi(argv[1]);
    int sig = atoi(argv[2]);

    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = sighandler; 



    int child = fork();
    if(child == 0) {
        //zablokuj wszystkie sygnaly za wyjatkiem SIGUSR1
        //zdefiniuj obsluge SIGUSR1 w taki sposob aby proces potomny wydrukowal
        //na konsole przekazana przez rodzica wraz z sygnalem SIGUSR1 wartosc

        sigset_t set;
        sigfillset(&set);
        sigdelset(&set, SIGUSR1);
        sigprocmask(SIG_SETMASK, &set, NULL);

        if (sigaction(SIGUSR1, &action, NULL) == -1){
            perror("sigaction");
            exit(1);
        }

        pause();
        exit(0);
    }
    else {
        sleep(1);

        union sigval sval;
        sval.sival_int = value;

        if (sigqueue(child, sig, sval) == -1){
            perror("sigqueue");
            return 1;
        }
        wait(NULL);
    }

    return 0;
}
