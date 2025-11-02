#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <math.h>

// Funkcja podcałkowa: f(x) = 4/(x^2+1)
double f(double x) {
    return 4.0 / (x * x + 1.0);
}

// Funkcja obliczająca różnicę czasu (w sekundach) między dwoma punktami czasowymi
double diff_time(struct timespec start, struct timespec end) {
    double elapsed_sec = end.tv_sec - start.tv_sec;
    double elapsed_nsec = (end.tv_nsec - start.tv_nsec) / 1e9;
    return elapsed_sec + elapsed_nsec;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Użycie: %s dx n\n", argv[0]);
        fprintf(stderr, "  dx - szerokość prostokąta (krok całkowania)\n");
        fprintf(stderr, "  n  - maksymalna liczba procesów\n");
        return 1;
    }
    
    double dx = atof(argv[1]);
    int max_processes = atoi(argv[2]);

    if (dx <= 0 || max_processes < 1) {
        fprintf(stderr, "Nieprawidłowe argumenty.\n");
        return 1;
    }
    
    // Obliczamy liczbę prostokątów potrzebnych do pokrycia przedziału [0,1]
    long long steps = (long long)(1.0 / dx);
    if (steps * dx < 1.0) {
        steps++; // zapewnienie pełnego pokrycia przedziału
    }
    
    printf("Całkowita liczba prostokątów: %lld\n", steps);
    
    for (int k = 1; k <= max_processes; k++) {
        // Rozpoczynamy pomiar czasu przy użyciu zegara CLOCK_REALTIME
        struct timespec start, end;
        if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
            perror("clock_gettime");
            exit(EXIT_FAILURE);
        }
        
        // Tablica potoków - każdy proces potomny korzysta z osobnego potoku
        int pipes[k][2];
        
        // Podział prostokątów między k procesów
        long long base = steps / k;         // liczba prostokątów przypadająca na każdy proces
        long long remainder = steps % k;      // reszta do rozdzielenia między pierwsze procesy
        
        for (int i = 0; i < k; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                // Proces potomny – zamykamy koniec czytania potoku
                close(pipes[i][0]);
                
                // Obliczenie zakresu indeksów do przetworzenia przez dany proces
                long long start_index, count;
                if (i < remainder) {
                    count = base + 1;
                    start_index = i * (base + 1);
                } else {
                    count = base;
                    start_index = remainder * (base + 1) + (i - remainder) * base;
                }
                
                double partial_sum = 0.0;
                for (long long j = start_index; j < start_index + count; j++) {
                    double x = j * dx;
                    if (x > 1.0) x = 1.0; // zabezpieczenie, aby nie wyjść poza przedział
                    partial_sum += f(x) * dx;
                }
                
                // Wysłanie wyniku częściowego do procesu macierzystego
                if (write(pipes[i][1], &partial_sum, sizeof(double)) != sizeof(double)) {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
                
                close(pipes[i][1]);
                exit(0);
            } else {
                // Proces macierzysty – zamykamy koniec zapisu potoku
                close(pipes[i][1]);
            }
        }
        
        // Oczekiwanie na zakończenie wszystkich procesów potomnych
        for (int i = 0; i < k; i++) {
            wait(NULL);
        }
        
        // Odczyt wyników z potoków i ich sumowanie
        double total_sum = 0.0;
        for (int i = 0; i < k; i++) {
            double partial_sum;
            if (read(pipes[i][0], &partial_sum, sizeof(double)) != sizeof(double)) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            total_sum += partial_sum;
            close(pipes[i][0]);
        }
        
        // Zakończenie pomiaru czasu
        if (clock_gettime(CLOCK_REALTIME, &end) == -1) {
            perror("clock_gettime");
            exit(EXIT_FAILURE);
        }
        
        double elapsed = diff_time(start, end);
        
        printf("Liczba procesów: %d, Wynik: %.12f, Czas: %.6f s\n", k, total_sum, elapsed);
    }
    
    return 0;
}