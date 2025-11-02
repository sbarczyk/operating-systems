#include <stdio.h>
#include <signal.h> 

void handler(int signum){
    printf("Obsługa sygnału\n");
}

int main(void){
    signal(SIGUSR1, handler);
    raise(SIGUSR1);
    while(1);
    return 0;
}