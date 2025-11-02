#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    double width;
    double *results;
    int *ready;
    int num_steps;
    int num_threads;
} SharedData;

typedef struct {
    int thread_id;
    SharedData *shared;
} ThreadData;

double f(double x) {
    return 4.0 / (x * x + 1);
}

void* compute_integral(void* arg) {
    ThreadData *data = (ThreadData*) arg;
    SharedData *shared = data->shared;
    int id = data->thread_id;

    int steps_per_thread = shared->num_steps / shared->num_threads;
    int start = id * steps_per_thread;
    int end = (id == shared->num_threads - 1) ? shared->num_steps : (id + 1) * steps_per_thread;

    double partial_sum = 0.0;
    for (int i = start; i < end; ++i) {
        double x = i * shared->width;
        partial_sum += f(x) * shared->width;
    }

    shared->results[id] = partial_sum;
    shared->ready[id] = 1;

    return NULL;
}

// Funkcja pomocnicza do liczenia różnicy czasu w sekundach
double time_diff_in_seconds(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

// Funkcja pomocnicza do pomiaru czasu wykonania pracy
double measure_integration_time(SharedData *shared) {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_REALTIME, &start_time);

    pthread_t *threads = malloc(sizeof(pthread_t) * shared->num_threads);
    ThreadData *thread_data = malloc(sizeof(ThreadData) * shared->num_threads);

    for (int i = 0; i < shared->num_threads; ++i) {
        thread_data[i].thread_id = i;
        thread_data[i].shared = shared;
        pthread_create(&threads[i], NULL, compute_integral, &thread_data[i]);
    }

    int all_ready = 0;
    while (!all_ready) {
        all_ready = 1;
        for (int i = 0; i < shared->num_threads; ++i) {
            if (shared->ready[i] == 0) {
                all_ready = 0;
                break;
            }
        }
        if (!all_ready) {
            struct timespec req = {0, 1000000}; // 1 ms = 1 000 000 ns
            nanosleep(&req, NULL);
        }
    }

    clock_gettime(CLOCK_REALTIME, &end_time);

    free(threads);
    free(thread_data);

    return time_diff_in_seconds(start_time, end_time);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Użycie: %s <szerokość prostokąta> <maksymalna liczba wątków>\n", argv[0]);
        return 1;
    }

    double width = atof(argv[1]);
    int max_threads = atoi(argv[2]);
    int num_steps = (int)(1.0 / width);

    printf("Całkowanie od 1 do %d wątków...\n", max_threads);
    printf("----------------------------------------\n");

    for (int num_threads = 1; num_threads <= max_threads; ++num_threads) {
        if (num_steps < num_threads) {
            printf("%2d wątków: Za dużo wątków względem liczby kroków (%d kroków)\n", num_threads, num_steps);
            continue;
        }

        double *results = malloc(num_threads * sizeof(double));
        int *ready = calloc(num_threads, sizeof(int));

        SharedData shared = {
            .width = width,
            .results = results,
            .ready = ready,
            .num_steps = num_steps,
            .num_threads = num_threads
        };

        double elapsed = measure_integration_time(&shared);

        double total_sum = 0.0;
        for (int i = 0; i < num_threads; ++i) {
            total_sum += results[i];
        }

        printf("%2d wątków: wartość całki = %.15f, czas = %.6f sekund\n", num_threads, total_sum, elapsed);

        free(results);
        free(ready);
    }

    printf("----------------------------------------\n");

    return 0;
}