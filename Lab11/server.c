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

/* ───── stałe ───── */
#define MAX_CLIENTS 10
#define MAX_NAME 32
#define BUF_SIZE 512
#define PING_INTERVAL 10        // Interwał wysyłania PING (sekundy)
#define PING_TIMEOUT 30         // Timeout dla odpowiedzi na PING (sekundy)

/* ───── struktura klienta ───── */
struct client {
    int fd;                     // Deskryptor gniazda klienta
    char name[MAX_NAME];        // Pseudonim klienta
    time_t last_alive;          // Timestamp ostatniej aktywności (odpowiedzi na PING)
    int active;                 // Flaga: 0 = slot wolny, 1 = slot zajęty
};

/* ───── zmienne globalne ───── */
static struct client clients[MAX_CLIENTS];
static pthread_mutex_t clients_mtx = PTHREAD_MUTEX_INITIALIZER;  // Mutex chroniący dostęp do tablicy klientów
static volatile sig_atomic_t running = 1;
static int listen_fd = -1;                                 // Deskryptor gniazda nasłuchującego

/* ───── funkcje pomocnicze ───── */

// Funkcja wyświetlająca błąd systemowy i kończąca program
static void fatal(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Funkcja wysyłająca linię tekstu do konkretnego klienta
static void send_line(int fd, const char *line)
{
    // MSG_NOSIGNAL zapobiega generowaniu SIGPIPE przy zamkniętym połączeniu
    send(fd, line, strlen(line), MSG_NOSIGNAL);
}

// Funkcja rozgłaszająca wiadomość do wszystkich klientów oprócz jednego
// UWAGA: Wywołujący musi wcześniej zablokować mutex clients_mtx!
static void broadcast_locked(const char *msg, int except_fd)
{
    for (int i = 0; i < MAX_CLIENTS; ++i)
        if (clients[i].active && clients[i].fd != except_fd)
            send_line(clients[i].fd, msg);
}

// Funkcja znajdująca klienta po pseudonimie
// Zwraca indeks w tablicy lub -1 jeśli nie znaleziono
static int find_by_name(const char *name)
{
    for (int i = 0; i < MAX_CLIENTS; ++i)
        if (clients[i].active && strcmp(clients[i].name, name) == 0)
            return i;
    return -1;
}

// Funkcja usuwająca klienta z serwera
// UWAGA: Wywołujący musi wcześniej zablokować mutex clients_mtx!
static void remove_client_locked(int idx)
{
    if (!clients[idx].active) return;
    
    close(clients[idx].fd);         // Zamknij połączenie
    clients[idx].active = 0;        // Oznacz slot jako wolny
    
    fprintf(stderr, "[INFO] Klient %s rozłączony.\n", clients[idx].name);
}

/* ───── wątek obsługi pojedynczego klienta ───── */
static void *client_thread(void *arg)
{
    // Pobierz indeks klienta z argumentu i zwolnij pamięć
    int idx = *(int *)arg;
    free(arg);
    
    // Skopiuj dane klienta do zmiennych lokalnych (bezpieczeństwo wątkowe)
    int myfd;
    char myname[MAX_NAME];
    pthread_mutex_lock(&clients_mtx);
    myfd = clients[idx].fd;
    strncpy(myname, clients[idx].name, MAX_NAME);
    pthread_mutex_unlock(&clients_mtx);
    
    // Bufor do zbierania przychodzących danych
    char buf[BUF_SIZE];
    size_t buflen = 0;              // Aktualna długość danych w buforze
    
    // Główna pętla obsługi klienta
    while (running) {
        // Odbierz dane z gniazda klienta
        ssize_t n = recv(myfd, buf + buflen, sizeof(buf) - 1 - buflen, 0);
        if (n <= 0) break;          // Rozłączenie lub błąd - zakończ wątek
        
        buflen += (size_t)n;
        buf[buflen] = '\0';
        
        // Przetwarzaj dane linia po linii
        char *start = buf;
        char *nl;
        while ((nl = strpbrk(start, "\r\n"))) {    // Znajdź koniec linii
            *nl = '\0';                             // Zakończ linię
            char *line = start;                     // Wskaźnik na aktualną linię
            start = nl + 1;                         // Przesuń na następną linię
            
            if (*line == '\0') continue;            // Pomiń puste linie
            
            /* ---------- obsługa komend ---------- */
            
            // Komenda LIST - wyświetl listę aktywnych użytkowników
            if (strcmp(line, "LIST") == 0) {
                pthread_mutex_lock(&clients_mtx);
                char out[BUF_SIZE] = "USERS:";
                
                // Dodaj wszystkich aktywnych klientów do listy
                for (int k = 0; k < MAX_CLIENTS; ++k)
                    if (clients[k].active) {
                        strcat(out, " ");
                        strcat(out, clients[k].name);
                    }
                strcat(out, "\n");
                pthread_mutex_unlock(&clients_mtx);
                
                send_line(myfd, out);               // Wyślij listę do klienta
            }
            // Komenda 2ALL - wiadmość do wszystkich użytkowników
            else if (strncmp(line, "2ALL ", 5) == 0) {
                // Przygotuj timestamp
                time_t now = time(NULL);
                char ts[32];
                strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
                
                // Sformatuj wiadomość z timestampem i nadawcą
                char out[BUF_SIZE];
                snprintf(out, sizeof(out), "[%s] %s: %s\n", ts, myname, line + 5);
                
                // Rozgłoś wiadomość do wszystkich oprócz nadawcy
                pthread_mutex_lock(&clients_mtx);
                broadcast_locked(out, myfd);
                pthread_mutex_unlock(&clients_mtx);
            }
            // Komenda 2ONE - wiadomość prywatna do konkretnego użytkownika
            else if (strncmp(line, "2ONE ", 5) == 0) {
                // Znajdź spację oddzielającą nick od wiadomości
                char *sp = strchr(line + 5, ' ');
                if (!sp) {
                    send_line(myfd, "ERROR: cmd\n");
                    continue;
                }
                
                *sp = '\0';                         // Rozdziel nick od wiadomości
                const char *dest = line + 5;       // Nick odbiorcy
                const char *msg = sp + 1;          // Treść wiadomości
                
                pthread_mutex_lock(&clients_mtx);
                int d = find_by_name(dest);         // Znajdź odbiorcę
                
                if (d >= 0) {
                    // Odbiorca znaleziony - wyślij wiadomość
                    time_t now = time(NULL);
                    char ts[32];
                    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
                    
                    char out[BUF_SIZE];
                    snprintf(out, sizeof(out), "[%s] %s -> %s: %s\n",
                            ts, myname, dest, msg);
                    send_line(clients[d].fd, out);
                } else {
                    // Odbiorca nie znaleziony
                    send_line(myfd, "ERROR: user not found\n");
                }
                pthread_mutex_unlock(&clients_mtx);
            }
            // Komenda ALIVE - odpowiedź na PING (keep-alive)
            else if (strcmp(line, "ALIVE") == 0) {
                pthread_mutex_lock(&clients_mtx);
                clients[idx].last_alive = time(NULL);   // Zaktualizuj timestamp aktywności
                pthread_mutex_unlock(&clients_mtx);
            }
            // Komenda STOP - wyłącz serwer
            else if (strcmp(line, "STOP") == 0) {
                send_line(myfd, "BYE\n");  // <-- wysyłamy potwierdzenie rozłączenia
                break;
            }
        }
        
        // Przenieś resztę niedokończonej linii na początek bufora
        if (start != buf) {
            buflen = strlen(start);
            memmove(buf, start, buflen);
        } else if (buflen == sizeof(buf) - 1) {
            buflen = 0;         // Linia zbyt długa → zresetuj bufor
        }
    }
    
    // Sprzątanie po zakończeniu obsługi klienta
    pthread_mutex_lock(&clients_mtx);
    remove_client_locked(idx);
    pthread_mutex_unlock(&clients_mtx);
    
    return NULL;
}

/* ───── wątek wysyłający PING i sprawdzający timeouty ───── */
static void *ping_thread(void *arg)
{
    (void)arg;
    
    while (running) {
        sleep(PING_INTERVAL);       // Czekaj określony interwał
        time_t now = time(NULL);
        
        pthread_mutex_lock(&clients_mtx);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (!clients[i].active) continue;       // Pomiń nieaktywnych klientów
            
            // Sprawdź czy klient przekroczył timeout
            if (now - clients[i].last_alive > PING_TIMEOUT) {
                fprintf(stderr, "[WARN] Timeout %s\n", clients[i].name);
                remove_client_locked(i);            // Usuń klienta za brak odpowiedzi
            } else {
                send_line(clients[i].fd, "PING\n"); // Wyślij PING
            }
        }
        pthread_mutex_unlock(&clients_mtx);
    }
    return NULL;
}

/* ───── obsługa sygnału SIGINT (Ctrl+C) ───── */
static void sigint_handler(int s)
{
    (void)s;        // Usunięcie ostrzeżenia o nieużywanym parametrze
    running = 0;    // Zatrzymaj serwer
    
    // Zamknij gniazdo nasłuchujące aby przerwać accept()
    if (listen_fd != -1) close(listen_fd);
}

/* ───── funkcja główna ───── */
int main(int argc, char *argv[])
{
    // Sprawdź argumenty wywołania
    if (argc != 2) {
        fprintf(stderr, "Użycie: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Konfiguracja obsługi sygnałów
    signal(SIGPIPE, SIG_IGN);           // Ignoruj SIGPIPE
    signal(SIGINT, sigint_handler);     // Obsługa Ctrl+C
    
    uint16_t port = (uint16_t)atoi(argv[1]);    // Port z argumentu
    
    // Tworzenie gniazda nasłuchującego
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) fatal("socket");
    
    // Pozwól na ponowne użycie adresu (unikaj "Address already in use")
    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    
    // Konfiguracja adresu serwera
    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;                   // IPv4
    srv.sin_port = htons(port);                 // Port w formacie sieciowym
    if (inet_pton(AF_INET, "127.0.0.1", &srv.sin_addr) != 1) fatal("inet_pton");    // Nasłuchuj na lokalnym interfejsie
    
    // Powiąż gniazdo z adresem
    if (bind(listen_fd, (struct sockaddr *)&srv, sizeof(srv)) < 0) fatal("bind");
    
    // Rozpocznij nasłuchiwanie
    if (listen(listen_fd, MAX_CLIENTS) < 0) fatal("listen");
    
    fprintf(stderr, "[INFO] Serwer start – port %d.\n", port);
    
    // Uruchom wątek odpowiedzialny za PING i timeouty
    pthread_t pinger;
    pthread_create(&pinger, NULL, ping_thread, NULL);
    
    // Główna pętla serwera - akceptowanie nowych połączeń
    while (running) {
        // Akceptuj nowe połączenie
        int cfd = accept(listen_fd, NULL, NULL);
        if (cfd < 0) {
            if (errno == EINTR || errno == EBADF) break;
            perror("accept");
            continue;
        }
        
        // Odbierz pseudonim od nowego klienta
        char namebuf[MAX_NAME] = {0};
        ssize_t n = recv(cfd, namebuf, MAX_NAME - 1, 0);
        if (n <= 0) {
            close(cfd);
            continue;
        }
        
        // Usuń znaki końca linii z pseudonimu
        char *e = strpbrk(namebuf, "\r\n");
        if (e) *e = '\0';
        
        // Sprawdź czy pseudonim nie jest pusty
        if (namebuf[0] == '\0') {
            send_line(cfd, "ERROR: nick\n");
            close(cfd);
            continue;
        }
        
        // Sprawdź czy pseudonim nie jest już zajęty
        pthread_mutex_lock(&clients_mtx);
        int duplicate = (find_by_name(namebuf) != -1);
        pthread_mutex_unlock(&clients_mtx);
        
        if (duplicate) {
            send_line(cfd, "ERROR: nick zajęty\n");
            close(cfd);
            continue;
        }
        
        // Znajdź wolny slot dla nowego klienta
        pthread_mutex_lock(&clients_mtx);
        int slot = -1;
        for (int i = 0; i < MAX_CLIENTS; ++i)
            if (!clients[i].active) {
                slot = i;
                break;
            }
        pthread_mutex_unlock(&clients_mtx);
        
        // Sprawdź czy serwer nie jest pełny
        if (slot == -1) {
            send_line(cfd, "SERVER FULL\n");
            close(cfd);
            continue;
        }
        
        // Zarejestruj nowego klienta
        pthread_mutex_lock(&clients_mtx);
        clients[slot].fd = cfd;
        strncpy(clients[slot].name, namebuf, MAX_NAME);
        clients[slot].last_alive = time(NULL);      // Ustaw timestamp aktywności
        clients[slot].active = 1;                   // Oznacz slot jako zajęty
        pthread_mutex_unlock(&clients_mtx);
        
        fprintf(stderr, "[INFO] Dołączył %s (slot %d)\n", namebuf, slot);
        
        // Stwórz nowy wątek do obsługi klienta
        int *arg = malloc(sizeof(int));             // Argument dla wątku
        if (!arg) fatal("malloc");
        *arg = slot;
        
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, arg) != 0) {
            perror("pthread_create");
            free(arg);
            // W przypadku błędu usuń klienta
            pthread_mutex_lock(&clients_mtx);
            remove_client_locked(slot);
            pthread_mutex_unlock(&clients_mtx);
        } else {
            pthread_detach(tid);        // Wątek będzie się sam sprzątał
        }
    }
    
    // Sprzątanie przed zakończeniem serwera
    running = 0;                        // Zatrzymaj wszystkie wątki
    pthread_join(pinger, NULL);         // Poczekaj na zakończenie wątku PING
    
    // Rozłącz wszystkich klientów
    pthread_mutex_lock(&clients_mtx);
    for (int i = 0; i < MAX_CLIENTS; ++i)
        remove_client_locked(i);
    pthread_mutex_unlock(&clients_mtx);
    
    // Zamknij gniazdo nasłuchujące
    if (listen_fd != -1) close(listen_fd);
    
    fprintf(stderr, "[INFO] Serwer zakończył działanie.\n");
    return 0;
}