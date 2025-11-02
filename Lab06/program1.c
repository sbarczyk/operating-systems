#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#define FIFO_INTERVAL "fifo_interval"
#define FIFO_RESULT   "fifo_result"

int main() {
    double a, b;
    printf("Podaj dolną granicę przedziału całkowania: ");
    if (scanf("%lf", &a) != 1) {
        fprintf(stderr, "Błąd odczytu dolnej granicy.\n");
        exit(EXIT_FAILURE);
    }
    printf("Podaj górną granicę przedziału całkowania: ");
    if (scanf("%lf", &b) != 1) {
        fprintf(stderr, "Błąd odczytu górnej granicy.\n");
        exit(EXIT_FAILURE);
    }

    // Utworzenie potoków nazwanych (FIFO) jeśli nie istnieją
    if (mkfifo(FIFO_INTERVAL, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo fifo_interval");
            exit(EXIT_FAILURE);
        }
    }
    if (mkfifo(FIFO_RESULT, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo fifo_result");
            exit(EXIT_FAILURE);
        }
    }

    // Otwarcie potoku do zapisu – przesłanie przedziału do programu 2
    int fd_interval = open(FIFO_INTERVAL, O_WRONLY);
    if (fd_interval == -1) {
        perror("open fifo_interval for writing");
        exit(EXIT_FAILURE);
    }

    // Wysyłamy przedział (a i b) jako dane binarne
    if (write(fd_interval, &a, sizeof(double)) != sizeof(double)) {
        perror("write a");
        exit(EXIT_FAILURE);
    }
    if (write(fd_interval, &b, sizeof(double)) != sizeof(double)) {
        perror("write b");
        exit(EXIT_FAILURE);
    }
    close(fd_interval);

    // Otwarcie potoku do odczytu – odebranie wyniku od programu 2
    int fd_result = open(FIFO_RESULT, O_RDONLY);
    if (fd_result == -1) {
        perror("open fifo_result for reading");
        exit(EXIT_FAILURE);
    }

    double result;
    if (read(fd_result, &result, sizeof(double)) != sizeof(double)) {
        perror("read result");
        exit(EXIT_FAILURE);
    }
    close(fd_result);

    printf("Wynik całkowania funkcji f(x)=4/(x^2+1) w przedziale [%.6f, %.6f] wynosi: %.12f\n", a, b, result);

    unlink(FIFO_INTERVAL);
    unlink(FIFO_RESULT);

    return 0;
}