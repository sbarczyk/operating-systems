#include <sys/types.h>
#include<sys/wait.h>
#include<stdio.h>
#include <unistd.h>

int main(void){
    pid_t pid;
    int i;
    for (i=0; i<10; i++){
        pid = fork();
        if (pid == 0){
            return 0;
        }
    }
    while (wait(NULL) > 0);
    return 0;
}