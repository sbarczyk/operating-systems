#define _POSIX_C_SOURCE 200112L


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h> 


int main(int argc, char *argv[]){
    union sigval war;
    int pid = atoi(argv[1]);

    war.sival_int = 123;
    sigqueue(pid, SIGUSR1, war);
    return 0;
}

