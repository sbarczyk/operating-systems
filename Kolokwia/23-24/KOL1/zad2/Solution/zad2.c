#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int fd[2];
int running = 1;
int counter = 0;
/*
1) program tworzy dwa procesy potomne. Nastepnie proces macierzysty co sekund�
wysy�a SIGUSR1 do procesu potomnego 1. Proces potomny 1 po otrzymaniu sygna�u
przesy�a kolejn� liczb� (rozpoczynajac od zera) przez potok nienazwany do
procesu potomnego 2, kt�ry wyswietla te liczbe.

2) Po wcisnieciu ctrl-c (SIGINT) powinno nastapic przerwanie wysy�ania sygnalow.
Powtorne wcisniecie ctrl-c powinno wznowic wysylanie sygnalow*/


void sigusr1_handler(int sig){
  write(fd[1], &counter, sizeof(counter));
  counter ++;
}

void sigint_handler(int sig){
  running = !running;
  if (running){
    printf("\nWznawiam wysyłanie sygnałw\n");
  }
  else{
    printf("\nWstrzymuję wysyłanie sygnałow\n");
  }
}

int main (int lpar, char *tab[]){
  pid_t pid1, pid2;
  int d,i;
  pipe(fd);
  pid1 = fork();
  if (pid1<0){
    perror("fork");
    return 0;
  }else if (pid1==0){//proces 1
    close(fd[0]);
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGINT, SIG_IGN);
    while(1);
    return 0;
  }else{
    pid2 = fork();
    if (pid2<0){
      perror("fork");
      return 0;
    }else if (pid2==0){//proces 2
      close(fd[1]);
      signal(SIGINT, SIG_IGN);
      while(1){
        d = read(fd[0], &i, sizeof(int));
        printf("przyjeto %d bajtow, wartosc:%d\n",d,i);
      }
      return 0;
    }
  }
  close(fd[0]);
  close(fd[1]);
  signal(SIGINT, sigint_handler);
  while(1) {
    if (running){
      kill(pid1, SIGUSR1);
      printf("wyslano SIGUSR1\n");
    }
    sleep(1);
  }
}