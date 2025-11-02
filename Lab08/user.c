#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

// Nazwy zasobów IPC (POSIX)
#define SHM_NAME       "/print_shm"
#define SEM_MUTEX_NAME "/print_mutex"
#define SEM_SLOTS_NAME "/print_slots"
#define SEM_ITEMS_NAME "/print_items"

// Parametry kolejki i zadań
#define QUEUE_SIZE     10    // Liczba miejsc w kolejce
#define JOB_LEN        10    // Długość zadania (10 znaków)

typedef struct {
    int head;  // indeks do czytania (drukarka)
    int tail;  // indeks do pisania (użytkownik)
    char buf[QUEUE_SIZE][JOB_LEN + 1]; // kolejka zadań
} shm_t;

int main() {
    int shm_fd;
    // Czekanie aż pamięć współdzielona będzie dostępna
    while ((shm_fd = shm_open(SHM_NAME, O_RDWR, 0)) == -1) {
        perror("[USER] Waiting for shared memory...");
        sleep(1); // Odczekaj 1 sekundę i spróbuj ponownie
    }

    // Mapowanie segmentu pamięci do przestrzeni adresowej procesu
    shm_t *shm = mmap(NULL, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) { perror("mmap"); exit(1); }

    sem_t *mutex, *slots, *items;

    // Czekanie aż semafor mutex będzie dostępny
    while ((mutex = sem_open(SEM_MUTEX_NAME, 0)) == SEM_FAILED) {
        perror("[USER] Waiting for mutex semaphore...");
        sleep(1);
    }

    // Czekanie aż semafor slotów będzie dostępny
    while ((slots = sem_open(SEM_SLOTS_NAME, 0)) == SEM_FAILED) {
        perror("[USER] Waiting for slots semaphore...");
        sleep(1);
    }

    // Czekanie aż semafor zadań będzie dostępny
    while ((items = sem_open(SEM_ITEMS_NAME, 0)) == SEM_FAILED) {
        perror("[USER] Waiting for items semaphore...");
        sleep(1);
    }

    // Inicjalizacja generatora liczb losowych
    srand(time(NULL) ^ getpid());

    // Główna pętla użytkownika
    while (1) {
        // Tworzenie losowego zadania wydruku (10 liter 'a'..'z')
        char job[JOB_LEN + 1];
        for (int i = 0; i < JOB_LEN; i++) {
            job[i] = 'a' + (rand() % 26);
        }
        job[JOB_LEN] = '\0';

        // Synchronizacja: czekaj na wolne miejsce w kolejce
        sem_wait(slots);

        // Wejście do sekcji krytycznej (blokowanie dostępu do kolejki)
        sem_wait(mutex);

        // Wpisanie nowego zadania do kolejki
        strncpy(shm->buf[shm->tail], job, JOB_LEN + 1);
        shm->tail = (shm->tail + 1) % QUEUE_SIZE;

        // Wyjście z sekcji krytycznej
        sem_post(mutex);

        // Powiadom drukarki, że jest nowe zadanie
        sem_post(items);

        printf("[USER %d] queued: %s\n", getpid(), job);
        fflush(stdout);

        // Losowa przerwa 1-5 sekund
        sleep(1 + rand() % 5);
    }

    return 0;
}