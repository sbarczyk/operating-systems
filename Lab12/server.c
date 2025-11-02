#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>

// ===== STAŁE KONFIGURACYJNE =====
#define MAX_CLIENTS 10          // Maksymalna liczba jednoczesnych klientów
#define MAX_NAME 32             // Maksymalna długość pseudonimu klienta
#define BUF_SIZE 512            // Rozmiar bufora na wiadomości
#define PING_INTERVAL 10        // Interwał wysyłania PING (sekundy) - sprawdzanie żywotności klientów
#define PING_TIMEOUT 30         // Timeout dla odpowiedzi na PING (sekundy) - po tym czasie klient uznawany za rozłączonego

// ===== STRUKTURA KLIENTA =====
struct client {
    struct sockaddr_in addr;    // Adres klienta (IP + port) - identyfikator sieciowy
    char name[MAX_NAME];        // Pseudonim klienta (nick)
    time_t last_alive;          // Timestamp ostatniej aktywności (odpowiedzi na PING lub wiadomości)
    int active;                 // Flaga stanu slotu: 0 = slot wolny, 1 = slot zajęty przez aktywnego klienta
};

// ===== ZMIENNE GLOBALNE =====
static struct client clients[MAX_CLIENTS];                         // Tablica wszystkich klientów (pula połączeń)
static pthread_mutex_t clients_mtx = PTHREAD_MUTEX_INITIALIZER;     // Mutex chroniący dostęp do tablicy klientów
static volatile sig_atomic_t running = 1;                          // Flaga kontrolująca główną pętlę serwera (volatile - może być zmieniana przez sygnały)
static int sock_fd = -1;                                           // Deskryptor gniazda UDP serwera

// ===== FUNKCJE POMOCNICZE =====

/**
 * Funkcja wyświetlająca błąd systemowy i kończąca program
 * @param msg - komunikat błędu do wyświetlenia
 */
static void fatal(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/**
 * Funkcja wysyłająca datagram UDP do konkretnego klienta
 * @param idx - indeks klienta w tablicy clients[]
 * @param data - dane do wysłania (string zakończony '\0')
 */
static void send_to_client(int idx, const char *data)
{
    // Sprawdź czy slot jest aktywny (zabezpieczenie przed wysłaniem do nieaktywnego klienta)
    if (!clients[idx].active) return;
    
    // Wyślij datagram UDP do klienta używając jego adresu
    sendto(sock_fd, data, strlen(data), 0,
           (struct sockaddr *)&clients[idx].addr, sizeof(clients[idx].addr));
}

/**
 * Funkcja rozgłaszająca wiadomość do wszystkich aktywnych klientów oprócz jednego
 * UWAGA: Wywołujący musi wcześniej zablokować mutex clients_mtx!
 * @param msg - wiadomość do rozgłoszenia
 * @param except_idx - indeks klienta, który ma być pominięty (zazwyczaj nadawca)
 */
static void broadcast_locked(const char *msg, int except_idx)
{
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active && i != except_idx) {
            send_to_client(i, msg);
        }
    }
}

/**
 * Funkcja znajdująca klienta po pseudonimie
 * @param name - pseudonim do wyszukania
 * @return indeks w tablicy clients[] lub -1 jeśli nie znaleziono
 */
static int find_by_name(const char *name)
{
    for (int i = 0; i < MAX_CLIENTS; ++i)
        if (clients[i].active && strcmp(clients[i].name, name) == 0)
            return i;
    return -1;
}

/**
 * Funkcja znajdująca klienta po adresie sieciowym (IP + port)
 * @param addr - adres do wyszukania
 * @return indeks w tablicy clients[] lub -1 jeśli nie znaleziono
 */
static int find_by_addr(const struct sockaddr_in *addr)
{
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active &&
            clients[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&    // Porównaj IP
            clients[i].addr.sin_port == addr->sin_port) {                  // Porównaj port
            return i;
        }
    }
    return -1;
}

/**
 * Funkcja usuwająca klienta z serwera (zwalnianie slotu)
 * UWAGA: Wywołujący musi wcześniej zablokować mutex clients_mtx!
 * @param idx - indeks klienta do usunięcia
 */
static void remove_client_locked(int idx)
{
    if (!clients[idx].active) return;
    
    clients[idx].active = 0;
    fprintf(stderr, "[INFO] Klient %s rozłączony.\n", clients[idx].name);
    memset(clients[idx].name, 0, MAX_NAME);
}

// ===== OBSŁUGA PROTOKOŁU KOMUNIKACJI =====

/**
 * Funkcja obsługująca rejestrację nowego klienta na serwerze
 * @param from - adres klienta próbującego się zarejestrować
 * @param nick - pseudonim podany przez klienta
 */
static void handle_register(const struct sockaddr_in *from, const char *nick)
{
    // ===== WALIDACJA PSEUDONIMU =====
    if (nick[0] == '\0') {
        // Pseudonim pusty - odrzuć rejestrację
        sendto(sock_fd, "ERROR: nick pusty", 17, 0,
               (struct sockaddr *)from, sizeof(*from));
        return;
    }

    pthread_mutex_lock(&clients_mtx);       // Zablokuj dostęp do tablicy klientów

    // ===== SPRAWDŹ CZY KLIENT JUŻ ISTNIEJE =====
    int existing_idx = find_by_addr(from);
    if (existing_idx != -1) {
        // Klient już zarejestrowany - zaktualizuj tylko timestamp aktywności
        clients[existing_idx].last_alive = time(NULL);
        pthread_mutex_unlock(&clients_mtx);
        sendto(sock_fd, "REGISTERED", 10, 0,
               (struct sockaddr *)from, sizeof(*from));
        return;
    }

    // ===== SPRAWDŹ CZY PSEUDONIM JEST DOSTĘPNY =====
    if (find_by_name(nick) != -1) {
        // Pseudonim już zajęty przez innego klienta
        pthread_mutex_unlock(&clients_mtx);
        sendto(sock_fd, "ERROR: nick zajęty", 18, 0,
               (struct sockaddr *)from, sizeof(*from));
        return;
    }

    // ===== ZNAJDŹ WOLNY SLOT =====
    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i].active) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        // Serwer pełny - brak wolnych slotów
        pthread_mutex_unlock(&clients_mtx);
        sendto(sock_fd, "SERVER FULL", 11, 0,
               (struct sockaddr *)from, sizeof(*from));
        return;
    }

    // ===== ZAREJESTRUJ KLIENTA =====
    clients[slot].addr = *from;                                     // Skopiuj adres klienta
    strncpy(clients[slot].name, nick, MAX_NAME - 1);               // Skopiuj pseudonim
    clients[slot].name[MAX_NAME - 1] = '\0';                       // Zagwarantuj terminator stringu
    clients[slot].last_alive = time(NULL);                         // Ustaw timestamp aktywności
    clients[slot].active = 1;                                      // Oznacz slot jako aktywny

    pthread_mutex_unlock(&clients_mtx);     // Odblokuj dostęp do tablicy klientów

    fprintf(stderr, "[INFO] Zarejestrowano %s (slot %d)\n", nick, slot);
    sendto(sock_fd, "REGISTERED", 10, 0,
           (struct sockaddr *)from, sizeof(*from));
}

/**
 * Funkcja obsługująca przychodzące wiadomości od zarejestrowanych klientów
 * @param from - adres nadawcy
 * @param data - treść wiadomości
 */
static void handle_message(const struct sockaddr_in *from, const char *data)
{
    // ===== IDENTYFIKACJA KLIENTA =====
    pthread_mutex_lock(&clients_mtx);
    int client_idx = find_by_addr(from);
    pthread_mutex_unlock(&clients_mtx);

    // Jeśli klient nie jest zarejestrowany, odrzuć wiadomość
    if (client_idx == -1) {
        sendto(sock_fd, "ERROR: nie zarejestrowany", 25, 0,
               (struct sockaddr *)from, sizeof(*from));
        return;
    }

    // Skopiuj pseudonim klienta do zmiennej lokalnej (thread-safety)
    char client_name[MAX_NAME];
    pthread_mutex_lock(&clients_mtx);
    strncpy(client_name, clients[client_idx].name, MAX_NAME);
    pthread_mutex_unlock(&clients_mtx);

    // ===== OBSŁUGA KOMEND PROTOKOŁU =====

    // Komenda LIST - wyświetl listę aktywnych użytkowników
    if (strcmp(data, "LIST") == 0) {
        pthread_mutex_lock(&clients_mtx);
        
        char out[BUF_SIZE] = "USERS:";                      // Nagłówek listy
        
        // Dodaj wszystkich aktywnych klientów do listy
        for (int k = 0; k < MAX_CLIENTS; ++k) {
            if (clients[k].active) {
                strcat(out, " ");
                strcat(out, clients[k].name);
            }
        }
        
        pthread_mutex_unlock(&clients_mtx);
        send_to_client(client_idx, out);                    // Wyślij listę do żądającego klienta
    }
    
    // Komenda 2ALL - wiadomość publiczna do wszystkich użytkowników
    else if (strncmp(data, "2ALL ", 5) == 0) {
        // ===== PRZYGOTUJ TIMESTAMP =====
        time_t now = time(NULL);
        char ts[32];
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));

        // ===== SFORMATUJ WIADOMOŚĆ =====
        char out[BUF_SIZE];
        snprintf(out, sizeof(out), "[%s] %s: %s", ts, client_name, data + 5);

        // ===== ROZGŁOŚ WIADOMOŚĆ =====
        pthread_mutex_lock(&clients_mtx);
        broadcast_locked(out, client_idx);                  // Wyślij do wszystkich oprócz nadawcy
        pthread_mutex_unlock(&clients_mtx);
    }
    
    // Komenda 2ONE - wiadomość prywatna do konkretnego użytkownika
    else if (strncmp(data, "2ONE ", 5) == 0) {
        // ===== PARSOWANIE KOMENDY =====
        // Format: "2ONE nick wiadomość"
        
        // Znajdź spację oddzielającą nick od wiadomości
        char *sp = strchr(data + 5, ' ');
        if (!sp) {
            send_to_client(client_idx, "ERROR: cmd");       // Błędny format komendy
            return;
        }

        // Skopiuj dane do bufora roboczego (aby móc je modyfikować)
        char work_buf[BUF_SIZE];
        strncpy(work_buf, data + 5, sizeof(work_buf) - 1);
        work_buf[sizeof(work_buf) - 1] = '\0';

        // Znajdź spację w kopii roboczej
        char *sp_work = strchr(work_buf, ' ');
        if (!sp_work) {
            send_to_client(client_idx, "ERROR: cmd");
            return;
        }

        *sp_work = '\0';                                    // Rozdziel nick od wiadomości (zastąp spację terminatorem)
        const char *dest = work_buf;                        // Nick odbiorcy
        const char *msg = sp_work + 1;                      // Treść wiadomości

        // ===== WYSZUKAJ ODBIORCĘ I WYŚLIJ WIADOMOŚĆ =====
        pthread_mutex_lock(&clients_mtx);
        int d = find_by_name(dest);                         // Znajdź odbiorcę po pseudonimie
        
        if (d >= 0) {
            // Odbiorca znaleziony - przygotuj i wyślij wiadomość prywatną
            time_t now = time(NULL);
            char ts[32];
            strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
            
            char out[BUF_SIZE];
            snprintf(out, sizeof(out), "[%s] %s -> %s: %s",
                     ts, client_name, dest, msg);
            
            send_to_client(d, out);                         // Wyślij tylko do odbiorcy
        } else {
            // Odbiorca nie znaleziony
            send_to_client(client_idx, "ERROR: user not found");
        }
        
        pthread_mutex_unlock(&clients_mtx);
    }
    
    // Komenda ALIVE - odpowiedź na PING (mechanizm keep-alive)
    else if (strcmp(data, "ALIVE") == 0) {
        // Zaktualizuj timestamp ostatniej aktywności klienta
        pthread_mutex_lock(&clients_mtx);
        clients[client_idx].last_alive = time(NULL);
        pthread_mutex_unlock(&clients_mtx);
    }
    
    // Komenda STOP - graceful disconnection (rozłączenie na żądanie klienta)
    else if (strcmp(data, "STOP") == 0) {
        send_to_client(client_idx, "BYE");                  // Potwierdź rozłączenie
        
        pthread_mutex_lock(&clients_mtx);
        remove_client_locked(client_idx);                   // Usuń klienta z serwera
        pthread_mutex_unlock(&clients_mtx);
    }
    

}

// ===== WĄTEK ZARZĄDZANIA POŁĄCZENIAMI =====

/**
 * Wątek odpowiedzialny za wysyłanie PING do klientów i sprawdzanie timeoutów
 * Działa w tle i periodycznie:
 * 1. Wysyła PING do wszystkich aktywnych klientów
 * 2. Sprawdza czy klienci odpowiadają w określonym czasie
 * 3. Usuwa klientów, którzy nie odpowiadają (timeout)
 */
static void *ping_thread(void *arg)
{
    (void)arg;

    // Główna pętla wątku PING
    while (running) {
        sleep(PING_INTERVAL);                               // Czekaj określony interwał między sprawdzeniami

        time_t now = time(NULL);

        pthread_mutex_lock(&clients_mtx);


        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (!clients[i].active) continue;               // Pomiń nieaktywnych klientów

            // Sprawdź czy klient przekroczył timeout (nie odpowiadał na PING zbyt długo)
            if (now - clients[i].last_alive > PING_TIMEOUT) {
                fprintf(stderr, "[WARN] Timeout %s\n", clients[i].name);
                remove_client_locked(i);                    // Usuń klienta za brak odpowiedzi
            } else {
                send_to_client(i, "PING");                  // Wyślij PING do sprawdzenia żywotności
            }
        }

        pthread_mutex_unlock(&clients_mtx);
    }

    return NULL;
}

// ===== OBSŁUGA SYGNAŁÓW =====

/**
 * Obsługa sygnału SIGINT (Ctrl+C) - graceful shutdown serwera
 */
static void sigint_handler(int s)
{
    (void)s;
    running = 0;
}

// ===== FUNKCJA GŁÓWNA =====

/**
 * Główna funkcja programu - serwer UDP chat
 */
int main(int argc, char *argv[])
{
    // ===== WALIDACJA ARGUMENTÓW =====
    if (argc != 2) {
        fprintf(stderr, "Użycie: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // ===== KONFIGURACJA OBSŁUGI SYGNAŁÓW =====
    signal(SIGPIPE, SIG_IGN);                               // Ignoruj SIGPIPE (broken pipe) - może wystąpić przy zamkniętym połączeniu
    signal(SIGINT, sigint_handler);                         // Obsługa Ctrl+C - graceful shutdown

    uint16_t port = (uint16_t)atoi(argv[1]);

    // ===== TWORZENIE I KONFIGURACJA GNIAZDA UDP =====
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);               // Utwórz gniazdo UDP
    if (sock_fd < 0) fatal("socket");

    // Pozwól na ponowne użycie adresu (unikaj błędu "Address already in use")
    int yes = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // ===== KONFIGURACJA ADRESU SERWERA =====
    struct sockaddr_in srv = {0};                           // Wyzeruj strukturę
    srv.sin_family = AF_INET;                               // Protokół IPv4
    srv.sin_port = htons(port);                             // Port w formacie sieciowym (network byte order)
    srv.sin_addr.s_addr = INADDR_ANY;                       // Nasłuchuj na wszystkich interfejsach sieciowych

    // ===== POWIĄZANIE GNIAZDA Z ADRESEM =====
    if (bind(sock_fd, (struct sockaddr *)&srv, sizeof(srv)) < 0) 
        fatal("bind");

    fprintf(stderr, "[INFO] Serwer UDP uruchomiony na porcie %d.\n", port);

    // ===== URUCHOMIENIE WĄTKU PING =====
    pthread_t pinger;                                       // Identyfikator wątku
    pthread_create(&pinger, NULL, ping_thread, NULL);       // Utwórz wątek odpowiedzialny za PING i timeouty

    // ===== GŁÓWNA PĘTLA SERWERA =====
    char buf[BUF_SIZE];                                     // Bufor na odbierane wiadomości
    
    while (running) {
        // Używamy select() z timeoutem aby móc sprawdzać flagę running
        // Bez tego serwer mógłby zawiesić się na recvfrom() i nie reagować na SIGINT
        fd_set rfds;                                        // Zbiór deskryptorów do monitorowania
        FD_ZERO(&rfds);                                     // Wyczyść zbiór
        FD_SET(sock_fd, &rfds);                             // Dodaj nasze gniazdo do zbioru

        // Timeout dla select - sprawdzamy flagę running co sekundę
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // Czekaj maksymalnie 1 sekundę na dane do odczytu
        int ready = select(sock_fd + 1, &rfds, NULL, NULL, &timeout);
        if (ready < 0) {
            if (errno == EINTR) continue;                   // Przerwanie przez sygnał - kontynuuj
            perror("select");
            continue;                                       // Błąd, ale kontynuujemy działanie
        }

        if (ready == 0) continue;                           // Timeout - sprawdź flagę running i kontynuuj

        // Sprawdź czy nasze gniazdo ma dane do odczytu
        if (FD_ISSET(sock_fd, &rfds)) {
            struct sockaddr_in from_addr;                   // Adres nadawcy datagramu
            socklen_t from_len = sizeof(from_addr);         // Rozmiar struktury adresu

            // ===== ODBIERANIE DATAGRAMU =====
            ssize_t n = recvfrom(sock_fd, buf, sizeof(buf) - 1, 0,
                               (struct sockaddr *)&from_addr, &from_len);
            if (n < 0) {
                if (errno == EINTR) continue;               // Przerwanie przez sygnał
                perror("recvfrom");
                continue;                                   // Błąd, ale kontynuujemy nasłuchiwanie
            }

            buf[n] = '\0';                                  // Dodaj terminator stringu (bezpieczeństwo)

            // ===== ROUTING WIADOMOŚCI =====
            // Sprawdź typ wiadomości i przekieruj do odpowiedniej funkcji obsługi
            
            if (strncmp(buf, "REGISTER ", 9) == 0) {
                // Rejestracja nowego klienta
                handle_register(&from_addr, buf + 9);       // Przekaż pseudonim (pomiń "REGISTER ")
            } else {
                // Zwykła wiadomość od zarejestrowanego klienta
                handle_message(&from_addr, buf);
            }
        }
    }

    // ===== SPRZĄTANIE PRZED ZAKOŃCZENIEM SERWERA =====
    
    running = 0;                                            // Zatrzymaj wszystkie wątki (redundantne, ale dla pewności)
    pthread_join(pinger, NULL);                             // Poczekaj na zakończenie wątku PING

    // Powiadom wszystkich klientów o zamknięciu serwera i rozłącz ich
    pthread_mutex_lock(&clients_mtx);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active) {
            send_to_client(i, "SERVER SHUTDOWN");           // Powiadom klienta
            remove_client_locked(i);                        // Usuń klienta z serwera
        }
    }
    pthread_mutex_unlock(&clients_mtx);

    // Zamknij gniazdo sieciowe
    if (sock_fd != -1) close(sock_fd);

    fprintf(stderr, "[INFO] Serwer UDP zakończył działanie.\n");
    return 0;                                               // Sukces
}