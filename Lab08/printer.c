#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h> 
#include <errno.h>


#define SHM_NAME       "/print_shm"   // Nazwa pamięci współdzielonej
#define SEM_MUTEX_NAME "/print_mutex" // Semafor mutex (ochrona dostępu)
#define SEM_SLOTS_NAME "/print_slots" // Semafor slotów (wolne miejsca w kolejce)
#define SEM_ITEMS_NAME "/print_items" // Semafor itemów (ilość zadań)
#define QUEUE_SIZE     10             // Maksymalna liczba zadań w kolejce
#define JOB_LEN        10             // Długość tekstu zadania

// Struktura w pamięci współdzielonej
typedef struct {
    int head; // indeks do czytania (drukarka)
    int tail; // indeks do pisania (uzytkownik)
    char buf[QUEUE_SIZE][JOB_LEN + 1]; // tablica zadań wydruku (kolejka pierścieniowa)
} shm_t;

int main() {
    int created = 0;

    // Próba utworzenia nowego segmentu pamięci współdzielonej
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd >= 0) {
        // Jeśli utworzono — ustaw rozmiar segmentu
        ftruncate(shm_fd, sizeof(shm_t));
        created = 1;
    } else if (errno == EEXIST) {
        // Jeśli segment już istnieje — otwórz istniejący
        shm_fd = shm_open(SHM_NAME, O_RDWR, 0);
        if (shm_fd == -1) { perror("shm_open"); exit(1); }
    } else {
        perror("shm_open");
        exit(1);
    }

    // Mapowanie pamięci współdzielonej
    shm_t *shm = mmap(NULL, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) { perror("mmap"); exit(1); }

    // Jeśli segment był nowo utworzony — ustaw head i tail na 0
    if (created) {
        shm->head = 0;
        shm->tail = 0;
    }

    // Tworzenie lub otwieranie semaforów
    sem_t *mutex = sem_open(SEM_MUTEX_NAME, O_CREAT | O_EXCL, 0666, 1);
    if (mutex == SEM_FAILED && errno == EEXIST)
        mutex = sem_open(SEM_MUTEX_NAME, 0);

    sem_t *slots = sem_open(SEM_SLOTS_NAME, O_CREAT | O_EXCL, 0666, QUEUE_SIZE);
    if (slots == SEM_FAILED && errno == EEXIST)
        slots = sem_open(SEM_SLOTS_NAME, 0);

    sem_t *items = sem_open(SEM_ITEMS_NAME, O_CREAT | O_EXCL, 0666, 0);
    if (items == SEM_FAILED && errno == EEXIST)
        items = sem_open(SEM_ITEMS_NAME, 0);

    if (!mutex || !slots || !items) {
        perror("sem_open");
        exit(1);
    }

    // Główna pętla drukarki
    while (1) {
        sem_wait(items);  // Czekaj na dostępne zadanie
        sem_wait(mutex);  // Wejście do sekcji krytycznej

        // Pobranie zadania z kolejki
        char job[JOB_LEN + 1];
        strncpy(job, shm->buf[shm->head], JOB_LEN + 1);
        shm->head = (shm->head + 1) % QUEUE_SIZE;

        sem_post(mutex);  // Wyjście z sekcji krytycznej
        sem_post(slots);  // Zwiększenie liczby wolnych miejsc

        // Symulacja drukowania zadania (1 znak na sekundę)
        printf("[PRINTER] printing: ");
        fflush(stdout);
        for (int i = 0; i < JOB_LEN; i++) {
            putchar(job[i]);
            fflush(stdout);
            sleep(1);
        }
        putchar('\n');
        fflush(stdout);
    }

    return 0;
}