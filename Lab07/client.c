#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define MAX_TEXT 512

enum
{
    INIT = 1,
    MESSAGE = 2,
    STOP = 3
};

typedef struct
{
    long mtype;          // typ komunikatu
    int client_qid;      // msqid klienta (dla INIT)
    int sender_id;       // identyfikator nadawcy
    char text[MAX_TEXT]; // treść wiadomości
} msgbuf_t;

int server_msqid, client_msqid, client_id;
pid_t child_pid;

void cleanup(int sig)
{
    if (child_pid > 0)
        kill(child_pid, SIGTERM);

    // Wyślij do serwera informację, że klient się rozłącza
    msgbuf_t stop = {.mtype = STOP, .sender_id = client_id};
    msgsnd(server_msqid, &stop, sizeof(stop) - sizeof(long), 0);

    // Usuń własną kolejkę komunikatów
    msgctl(client_msqid, IPC_RMID, NULL);
    printf("\nKlient: usunięto kolejkę, kończę.\n");
    exit(0);
}

int main()
{
    // Uzyskaj klucz IPC na podstawie pliku serwera
    key_t key = ftok("server.c", 'S');
    if (key == -1)
    {
        perror("ftok(server)");
        exit(1);
    }

    // Pobierz identyfikator kolejki serwera
    server_msqid = msgget(key, 0);
    if (server_msqid == -1)
    {
        perror("msgget(server)");
        exit(1);
    }

    // Utwórz własną kolejkę komunikatów klienta
    client_msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    if (client_msqid == -1)
    {
        perror("msgget(client)");
        exit(1);
    }

    // Wyślij żądanie połączenia do serwera (INIT)
    msgbuf_t init_msg = {.mtype = INIT, .client_qid = client_msqid};
    if (msgsnd(server_msqid, &init_msg, sizeof(init_msg) - sizeof(long), 0) == -1)
    {
        perror("msgsnd(INIT)");
        exit(1);
    }

    // Odbierz przypisany identyfikator klienta
    msgbuf_t reply;
    if (msgrcv(client_msqid, &reply, sizeof(reply) - sizeof(long), INIT, 0) == -1)
    {
        perror("msgrcv(INIT)");
        exit(1);
    }
    client_id = reply.sender_id;
    printf("Połączono jako klient %d\n", client_id);

    // Rozdzielenie procesu: odbiór i wysyłanie działają niezależnie
    child_pid = fork();
    if (child_pid == 0)
    {
        // Proces dziecka: odbiera wiadomości od serwera
        signal(SIGINT, SIG_IGN);

        msgbuf_t in;
        while (1)
        {
            if (msgrcv(client_msqid, &in, sizeof(in) - sizeof(long), MESSAGE, 0) > 0)
            {
                printf("Klient %d: %s", in.sender_id, in.text); // wypisz wiadomość
            }
        }
    }
    else
    {
        // Proces rodzica: wysyła wiadomości do serwera
        signal(SIGINT, cleanup);

        char buf[MAX_TEXT];
        while (fgets(buf, MAX_TEXT, stdin) != NULL)
        {
            msgbuf_t out = {.mtype = MESSAGE, .sender_id = client_id};
            strncpy(out.text, buf, MAX_TEXT - 1);
            out.text[MAX_TEXT - 1] = '\0';

            if (msgsnd(server_msqid, &out, sizeof(out) - sizeof(long), 0) == -1)
                perror("msgsnd(MESSAGE)");
        }
        cleanup(0); // zakończ, gdy stdin się skończy
    }
    return 0;
}
