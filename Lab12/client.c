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
#include <pthread.h>

#define BUF_SIZE 512

// ===== ZMIENNE GLOBALNE =====
static int sock_fd = -1;                        // Deskryptor gniazda UDP
static struct sockaddr_in srv_addr;             // Struktura zawierająca adres serwera (IP + port)
static volatile sig_atomic_t running = 1;
static char client_nick[64];                    

// ===== FUNKCJE POMOCNICZE =====

/**
 * Funkcja wyświetlająca błąd systemowy i kończąca program
 * @param m - komunikat błędu do wyświetlenia
 */
static void fatal(const char *m) {
    perror(m);                  // Wyświetl komunikat błędu z opisem errno
    exit(EXIT_FAILURE);         // Zakończ program z kodem błędu
}

/**
 * Funkcja wysyłająca datagram UDP do serwera
 * @param data - dane do wysłania (string zakończony '\0')
 */
static void send_datagram(const char *data)
{
    // Wysyłamy datagram UDP do serwera używając wcześniej skonfigurowanego adresu
    sendto(sock_fd, data, strlen(data), 0, 
           (struct sockaddr *)&srv_addr, sizeof(srv_addr));
}

/**
 * Obsługa sygnału SIGINT (Ctrl+C)
 */
static void sigint_handler(int s) {
    (void)s;
    running = 0;
}

// ===== WĄTKI =====

/**
 * Wątek odbierający wiadomości z serwera
 * Nasłuchuje na gnieździe UDP i przetwarza przychodzące wiadomości
 * @param arg - argument wątku (nieużywany)
 * @return NULL przy zakończeniu
 */
static void *receiver_thread(void *arg)
{
    (void)arg;
    char buf[BUF_SIZE];
    
    // Główna pętla odbierania wiadomości
    while (running) {
        struct sockaddr_in from_addr;           // Adres nadawcy datagramu
        socklen_t from_len = sizeof(from_addr); // Rozmiar struktury adresu
        
        // Używamy select() z timeoutem aby móc periodycznie sprawdzać flagę running
        // Bez tego wątek mógłby zawiesić się na recvfrom() i nie reagować na SIGINT
        fd_set rfds;                            // Zbiór deskryptorów do monitorowania
        FD_ZERO(&rfds);                         // Wyczyść zbiór
        FD_SET(sock_fd, &rfds);                 // Dodaj nasze gniazdo do zbioru
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        // Czekaj maksymalnie 1 sekundę na dane do odczytu
        int ready = select(sock_fd + 1, &rfds, NULL, NULL, &timeout);
        if (ready < 0) {
            if (errno == EINTR) continue;       // Przerwanie przez sygnał - kontynuuj
            perror("select");
            break;                              // Błąd krytyczny - wyjdź z pętli
        }
        
        if (ready == 0) continue;               // Timeout - sprawdź flagę running i kontynuuj
        
        // Sprawdź czy nasze gniazdo ma dane do odczytu
        if (FD_ISSET(sock_fd, &rfds)) {
            // Odbieramy datagram z serwera
            ssize_t n = recvfrom(sock_fd, buf, sizeof(buf) - 1, 0,
                               (struct sockaddr *)&from_addr, &from_len);
            if (n < 0) {
                if (errno == EINTR) continue;   // Przerwanie przez sygnał
                perror("recvfrom");
                continue;                       // Błąd, ale kontynuujemy nasłuchiwanie
            }

            buf[n] = '\0';                      // Dodajemy terminator stringu (bezpieczeństwo)

            // ===== PROTOKÓŁ KOMUNIKACJI =====
            // Sprawdzamy typ otrzymanej wiadomości i reagujemy odpowiednio
            
            if (strncmp(buf, "PING", 4) == 0) {
                // Serwer sprawdza czy klient żyje - odpowiadamy ALIVE
                send_datagram("ALIVE");
            } else if (strncmp(buf, "BYE", 3) == 0) {
                // Serwer potwierdza rozłączenie
                fprintf(stderr, "[INFO] Serwer potwierdził rozłączenie.\n");
                running = 0;                    // Kończymy program
            } else if (strncmp(buf, "REGISTERED", 10) == 0) {
                // Serwer potwierdza rejestrację klienta
                fprintf(stderr, "[INFO] Pomyślnie zarejestrowano na serwerze.\n");
            } else if (strncmp(buf, "ERROR:", 6) == 0) {
                // Serwer przesłał komunikat o błędzie
                fprintf(stderr, "[ERROR] %s\n", buf);
            } else {
                // Zwykła wiadomość czatu - wyświetl ją użytkownikowi
                printf("%s\n", buf);
                fflush(stdout);                 // Wymuś natychmiastowe wyświetlenie
            }
        }
    }
    
    return NULL;                                // Zakończenie wątku
}

/**
 * Wątek wysyłający wiadomości (obsługa wejścia z klawiatury)
 * Czyta linie z stdin i wysyła je do serwera
 * @param arg - argument wątku (nieużywany)
 * @return NULL przy zakończeniu
 */
static void *sender_thread(void *arg)
{
    (void)arg;
    char buf[BUF_SIZE];
    
    // Główna pętla wysyłania wiadomości
    while (running) {
        // Używamy select() z timeoutem aby móc sprawdzać flagę running
        // Bez tego wątek mógłby zawiesić się na fgets() i nie reagować na SIGINT
        fd_set rfds;                            // Zbiór deskryptorów do monitorowania
        FD_ZERO(&rfds);                         // Wyczyść zbiór
        FD_SET(STDIN_FILENO, &rfds);           // Dodaj stdin (klawiatura) do zbioru
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        // Czekaj maksymalnie 1 sekundę na wejście z klawiatury
        int ready = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &timeout);
        if (ready < 0) {
            if (errno == EINTR) continue;       // Przerwanie przez sygnał - kontynuuj
            perror("select");
            break;                              // Błąd krytyczny - wyjdź z pętli
        }
        
        if (ready == 0) continue;               // Timeout - sprawdź flagę running i kontynuuj
        
        // Sprawdź czy stdin ma dane do odczytu
        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            // Czytamy linię z klawiatury
            if (!fgets(buf, sizeof(buf), stdin)) {
                // EOF (Ctrl-D) lub błąd odczytu
                running = 0;                    // Kończymy program
                break;
            }
            
            // Usuń znak nowej linii z końca (jeśli istnieje)
            char *newline = strchr(buf, '\n');
            if (newline) *newline = '\0';
            
            // Wyślij wiadomość tylko jeśli nie jest pusta
            if (strlen(buf) > 0) {
                send_datagram(buf);             // Wysyłamy do serwera
            }
        }
    }
    
    return NULL;                                // Zakończenie wątku
}

// ===== FUNKCJA GŁÓWNA =====

/**
 * Główna funkcja programu - klient UDP chat
 */
int main(int argc, char *argv[])
{
    // ===== WALIDACJA ARGUMENTÓW =====
    if (argc != 4) {
        fprintf(stderr, "Użycie: %s <nick> <ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parsowanie argumentów wiersza poleceń
    const char *nick = argv[1];                 // Pseudonim użytkownika
    const char *ip = argv[2];                   // Adres IP serwera (string)
    uint16_t port = (uint16_t)atoi(argv[3]);    // Port serwera (konwersja string->int)

    // Kopiuj nick do globalnej zmiennej (bezpieczne kopiowanie z ograniczeniem długości)
    strncpy(client_nick, nick, sizeof(client_nick) - 1);
    client_nick[sizeof(client_nick) - 1] = '\0';  // Zagwarantuj terminator stringu

    // ===== KONFIGURACJA SYGNAŁÓW =====
    signal(SIGPIPE, SIG_IGN);           // Ignorujemy SIGPIPE (broken pipe) - może wystąpić przy zamkniętym połączeniu
    signal(SIGINT, sigint_handler);     // Obsługa Ctrl+C - graceful shutdown

    // ===== TWORZENIE GNIAZDA UDP =====
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) fatal("socket");

    // ===== KONFIGURACJA ADRESU SERWERA =====
    memset(&srv_addr, 0, sizeof(srv_addr));            // Wyzeruj strukturę
    srv_addr.sin_family = AF_INET;                      // Protokół IPv4
    srv_addr.sin_port = htons(port);                    // Port w formacie sieciowym (network byte order)
    
    // Konwersja adresu IP z formatu tekstowego na binarny
    if (inet_pton(AF_INET, ip, &srv_addr.sin_addr) != 1) {
        fatal("inet_pton");                             // Nieprawidłowy adres IP
    }

    // ===== REJESTRACJA NA SERWERZE =====
    char buf[BUF_SIZE];
    snprintf(buf, sizeof(buf), "REGISTER %s", nick);    // Stwórz wiadomość rejestracji
    send_datagram(buf);                                 // Wyślij do serwera

    fprintf(stderr, "[INFO] Łączenie z %s:%d jako %s.\n", ip, port, nick);

    // ===== TWORZENIE WĄTKÓW =====
    pthread_t receiver_tid, sender_tid;                 // Identyfikatory wątków
    
    // Utwórz wątek odbierający wiadomości
    if (pthread_create(&receiver_tid, NULL, receiver_thread, NULL) != 0) {
        fatal("pthread_create receiver");
    }
    
    // Utwórz wątek wysyłający wiadomości
    if (pthread_create(&sender_tid, NULL, sender_thread, NULL) != 0) {
        fatal("pthread_create sender");
    }

    // ===== OCZEKIWANIE NA ZAKOŃCZENIE WĄTKÓW =====
    // Główny wątek czeka na zakończenie obu wątków roboczych
    pthread_join(receiver_tid, NULL);                   // Czekaj na zakończenie receivera
    pthread_join(sender_tid, NULL);                     // Czekaj na zakończenie sendera

    // ===== SPRZĄTANIE PRZED ZAKOŃCZENIEM =====
    if (sock_fd != -1) {
        send_datagram("STOP");                          // Informujemy serwer o zakończeniu
        close(sock_fd);                                 // Zamykamy gniazdo UDP
    }

    fprintf(stderr, "[INFO] Klient zakończył.\n");
    return 0;                                           // Sukces
}