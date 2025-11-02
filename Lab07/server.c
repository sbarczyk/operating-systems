#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define MAX_TEXT    512

/* Typy komunikatów */
enum { INIT = 1, MESSAGE = 2, STOP = 3 };

/* Struktura komunikatu */
typedef struct {
    long mtype;             // typ komunikatu: INIT, MESSAGE, STOP
    int client_qid;         // przy INIT: msqid kolejki klienta
    int sender_id;          // przy MESSAGE/STOP: identyfikator nadawcy
    char text[MAX_TEXT];    // treść wiadomości
} msgbuf_t;

int server_msqid;
int client_qids[MAX_CLIENTS + 1];  // indeksy od 1 do MAX_CLIENTS

/* Funkcja sprzątająca przy zamknięciu serwera */
void cleanup(int sig) {
    msgctl(server_msqid, IPC_RMID, NULL);
    printf("\nSerwer: usunięto kolejkę, kończę.\n");
    exit(0);
}

int main() {
    // Tworzenie kolejki serwera
    key_t key = ftok("server.c", 'S');
    if (key == -1) { perror("ftok"); exit(1); }
    server_msqid = msgget(key, IPC_CREAT | 0666);
    if (server_msqid == -1) { perror("msgget(server)"); exit(1); }

    signal(SIGINT, cleanup);
    printf("Serwer uruchomiony, msqid=%d\n", server_msqid);
    memset(client_qids, 0, sizeof(client_qids));

    while (1) {
        msgbuf_t msg;
        // Odbieranie dowolnego komunikatu
        if (msgrcv(server_msqid, &msg, sizeof(msg) - sizeof(long), 0, 0) == -1) {
            perror("msgrcv");
            continue;
        }

        if (msg.mtype == INIT) {
            // Rejestracja nowego klienta
            int cid = 0;
            for (int i = 1; i <= MAX_CLIENTS; ++i) {
                if (client_qids[i] == 0) {
                    cid = i;
                    client_qids[cid] = msg.client_qid;
                    break;
                }
            }
            if (cid == 0) {
                fprintf(stderr, "Osiągnięto maksymalną liczbę klientów\n");
                continue;
            }

            msgbuf_t reply = { .mtype = INIT, .sender_id = cid };
            if (msgsnd(msg.client_qid, &reply, sizeof(reply) - sizeof(long), 0) == -1)
                perror("msgsnd(INIT reply)");

            printf("Klient %d podłączony (queue=%d)\n", cid, msg.client_qid);

        } else if (msg.mtype == MESSAGE) {
            // Rozsyłanie wiadomości do pozostałych klientów
            for (int i = 1; i <= MAX_CLIENTS; ++i) {
                if (i == msg.sender_id || client_qids[i] == 0) continue;
                msgbuf_t fwd = { .mtype = MESSAGE, .sender_id = msg.sender_id };
                strncpy(fwd.text, msg.text, MAX_TEXT - 1);
                fwd.text[MAX_TEXT - 1] = '\0';
                if (msgsnd(client_qids[i], &fwd, sizeof(fwd) - sizeof(long), 0) == -1)
                    perror("msgsnd(fwd)");
            }
            printf("Klient %d: %s", msg.sender_id, msg.text);

        } else if (msg.mtype == STOP) {
            // Obsługa wylogowania klienta
            int id = msg.sender_id;
            if (id >= 1 && id <= MAX_CLIENTS && client_qids[id] != 0) {
                client_qids[id] = 0;
                printf("Klient %d zakończył połączenie\n", id);
            }
        }
    }
    return 0;
}
