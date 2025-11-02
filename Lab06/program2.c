// program2.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>

#define FIFO_INTERVAL "fifo_interval"
#define FIFO_RESULT   "fifo_result"

// Funkcja całkowana: f(x)=4/(x^2+1)
double f(double x) {
    return 4.0 / (x * x + 1.0);
}

int main() {
    // Otwarcie potoku do odczytu – odbiór przedziału od programu 1
    printf("Odczytuję przedział całkowania...\n");
    int fd_interval = open(FIFO_INTERVAL, O_RDONLY);
    if (fd_interval == -1) {
        perror("open fifo_interval for reading");
        exit(EXIT_FAILURE);
    }

    double a, b;
    if (read(fd_interval, &a, sizeof(double)) != sizeof(double)) {
        perror("read a");
        exit(EXIT_FAILURE);
    }
    if (read(fd_interval, &b, sizeof(double)) != sizeof(double)) {
        perror("read b");
        exit(EXIT_FAILURE);
    }
    close(fd_interval);

    // Ustalanie kroku całkowania – im mniejsze dx, tym dokładniejsze przybliżenie
    double dx = 1e-6;
    double sum = 0.0;
    printf("Obliczam całkę...\n");
    // Obliczamy całkę metodą prostokątów z poprawką dla ostatniego fragmentu
    for (double x = a; x < b; x += dx) {
        double current_dx = dx;
        if (x + dx > b) {
            current_dx = b - x; // poprawka dla ostatniego fragmentu
        }
        sum += f(x) * current_dx;
    }

    // Otwarcie potoku do zapisu – wysłanie wyniku do programu 1
    printf("Przesyłam wynik do programu 1...\n");
    int fd_result = open(FIFO_RESULT, O_WRONLY);
    if (fd_result == -1) {
        perror("open fifo_result for writing");
        exit(EXIT_FAILURE);
    }

    if (write(fd_result, &sum, sizeof(double)) != sizeof(double)) {
        perror("write result");
        exit(EXIT_FAILURE);
    }
    close(fd_result);

    return 0;
}