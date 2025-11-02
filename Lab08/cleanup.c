#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define SHM_NAME "/print_shm"
#define SEM_MUTEX_NAME "/print_mutex"
#define SEM_SLOTS_NAME "/print_slots"
#define SEM_ITEMS_NAME "/print_items"

int main() {
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_MUTEX_NAME);
    sem_unlink(SEM_SLOTS_NAME);
    sem_unlink(SEM_ITEMS_NAME);

    printf("Shared memory and semaphores cleaned up.\n");
    return 0;
}