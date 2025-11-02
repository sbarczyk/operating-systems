#include <string.h>
// #include <mqueue.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#define ROZM_BLOKU 1024

int main(void){//POSIX
  //stworz dwa zamkniete semafory o identyfikatorach id_sem0 i id_sem1
  sem_t *id_sem0 = sem_open("nazwa_sem0", O_CREAT, 0600, 0);
  if (id_sem0 == SEM_FAILED){
    perror("sem_open id_sem0");
    exit(1);
  }
  sem_t *id_sem1 = sem_open("nazwa_sem1", O_CREAT, 0600, 0);
  if (id_sem1 == SEM_FAILED){
    perror("sem_open id_sem1");
    exit(1);
  }

  //
  //stworz pamiec wspoldzielona, okresl jej rozmiar na ROZM_BLOKU
  int fd = shm_open("pamiec", O_CREAT | O_RDWR, 0644);
  if (fd == -1) {
        perror("shm_open");
        sem_close(id_sem0);
        sem_close(id_sem1);
        exit(1);
    }
  if (ftruncate(fd, ROZM_BLOKU) == -1) {
        perror("ftruncate");
        close(fd);
        sem_close(id_sem0);
        sem_close(id_sem1);
        exit(1);
    }
  

  //
  //dolacz segment pamieci do przestrzeni adresowej procesu;
  //wsk ma wskazywac na te pamiec

  char* wsk = (char *) mmap(NULL, ROZM_BLOKU, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (wsk == MAP_FAILED) {
        perror("mmap");
        close(fd);
        sem_close(id_sem0);
        sem_close(id_sem1);
        exit(1);
  }
  wsk[0]=0;
  while(wsk[0]!='!'){
    sem_post(id_sem1);
    sem_wait(id_sem0);
    if(wsk[0]!='!') printf("%s\n", wsk);
  }
  munmap(wsk, ROZM_BLOKU);
  close(fd);
  sem_unlink("nazwa_sem0");
  sem_unlink("nazwa_sem1");
  shm_unlink("pamiec");
  return 0;
}
//kompilacja -lrt -lpthread