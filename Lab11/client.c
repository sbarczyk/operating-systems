#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/select.h>

#define BUF_SIZE 512

// Zmienne globalne
static int sock_fd = -1;                        // Deskryptor gniazda połączenia z serwerem
static volatile sig_atomic_t running = 1;       // Flaga kontrolująca pętlę główną programu

// Funkcja wyświetlająca błąd systemowy i kończąca program
static void fatal(const char *m) {
    perror(m);
    exit(EXIT_FAILURE);
}

// Funkcja wysyłająca linię tekstu do serwera
static void send_line(const char *line)
{
    // Wysyłamy dane przez gniazdo, MSG_NOSIGNAL zapobiega generowaniu sygnału SIGPIPE
    send(sock_fd, line, strlen(line), MSG_NOSIGNAL);
}

// Obsługa sygnału SIGINT (Ctrl+C) - ustawia flagę zakończenia programu
static void sigint_handler(int s) {
    (void)s;
    running = 0;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Użycie: %s <nick> <ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *nick = argv[1];                 // Pseudonim użytkownika
    const char *ip = argv[2];                   // Adres IP serwera
    uint16_t port = (uint16_t)atoi(argv[3]);    // Port serwera

    signal(SIGPIPE, SIG_IGN);           // Ignorujemy SIGPIPE (broken pipe)
    signal(SIGINT, sigint_handler);

    // Tworzenie gniazda TCP
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) fatal("socket");

    // Konfiguracja struktury adresu serwera
    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;                           // Protokół IPv4
    srv.sin_port = htons(port);                         // Port w formacie sieciowym
    if (inet_pton(AF_INET, ip, &srv.sin_addr) != 1)    // Konwersja IP z tekstu na format binarny
        fatal("inet_pton");

    // Nawiązanie połączenia z serwerem
    if (connect(sock_fd, (struct sockaddr *)&srv, sizeof(srv)) < 0)
        fatal("connect");

    // Wysłanie nicku do serwera jako pierwsza wiadomość
    char buf[BUF_SIZE];
    snprintf(buf, sizeof(buf), "%s\n", nick);
    send_line(buf);

    fprintf(stderr, "[INFO] Połączono z %s:%d jako %s.\n", ip, port, nick);

    // Główna pętla programu - obsługa komunikacji
    while (running) {
        // Przygotowanie zbiorów deskryptorów dla select()
        fd_set rfds;
        FD_ZERO(&rfds);                     // Wyczyść zbiór
        FD_SET(STDIN_FILENO, &rfds);        // Dodaj standardowe wejście (klawiatura)
        FD_SET(sock_fd, &rfds);             // Dodaj gniazdo serwera

        // Znajdź największy numer deskryptora (wymagane przez select)
        int maxfd = (STDIN_FILENO > sock_fd ? STDIN_FILENO : sock_fd);

        // Czekamy na aktywność na którymś z deskryptorów
        int ready = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (ready < 0) {
            if (errno == EINTR) continue;   // Przerwanie przez sygnał - kontynuuj
            fatal("select");
        }

        /* --- Obsługa wejścia z klawiatury --- */
        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            // Czytamy linię z klawiatury
            if (!fgets(buf, sizeof(buf), stdin)) {
                running = 0;    // EOF (Ctrl-D) - kończymy program
            } else if (strcmp(buf, "\n") != 0) {    // Jeśli to nie jest pusta linia
                send_line(buf);                     // Wysyłamy do serwera
            }
        }

        /* --- Obsługa komunikatów z serwera --- */
        if (FD_ISSET(sock_fd, &rfds)) {
            // Odbieramy dane z serwera
            ssize_t n = recv(sock_fd, buf, sizeof(buf) - 1, 0);
            if (n <= 0) {
                // Połączenie zostało zamknięte lub wystąpił błąd
                fprintf(stderr, "[INFO] Serwer rozłączył.\n");
                break;
            }

            buf[n] = '\0';  // Dodajemy terminator stringu

            // Sprawdzamy czy to wiadomość PING od serwera
            if (strncmp(buf, "PING", 4) == 0) {
                send_line("ALIVE\n");
            } else if (strncmp(buf, "BYE", 3) == 0) {
                fprintf(stderr, "[INFO] Serwer potwierdził rozłączenie.\n");
                running = 0;
            } else {
                fputs(buf, stdout);
                fflush(stdout);
}
        }
    }

    // Sprzątanie przed zakończeniem programu
    if (sock_fd != -1) {
        send_line("STOP\n");    // Informujemy serwer o zakończeniu
        close(sock_fd);         // Zamykamy gniazdo
    }

    fprintf(stderr, "[INFO] Klient zakończył.\n");
    return 0;
}