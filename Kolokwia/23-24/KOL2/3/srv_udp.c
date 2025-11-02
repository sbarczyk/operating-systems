#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>

#define L_SLOW 8

int main(void){
    int status, gniazdo, lbajtow, i;
    struct sockaddr_in ser, cli;
    char buf[200];
    char pytanie[L_SLOW] = "abcdefgh";
    char odpowiedz[L_SLOW][10] = {"alfa", "bravo", "charlie", "delta", "echo", "foxtrot", "golf", "hotel"};

    // stworz gniazdo UDP
    gniazdo = socket(AF_INET, SOCK_DGRAM, 0);
    if (gniazdo == -1) {printf("błąd gniazda\n"); return 0;}
 
    memset(&ser, 0, sizeof(ser));
    ser.sin_family = AF_INET;
    ser.sin_port = htons(9000);
    ser.sin_addr.s_addr = inet_addr("127.0.0.1");

    status = bind(gniazdo, (struct sockaddr*) &ser, sizeof(ser));
    if (status == -1) {printf("błąd bind\n"); return 0;}

    while (1){
        socklen_t len = sizeof(cli);
        lbajtow = recvfrom(gniazdo, buf, sizeof(buf), 0, (struct sockaddr *) &cli, &len);
        if (lbajtow > 0){
            buf[lbajtow] = 0;  // bezpieczeństwo przy stringach
            for(i=0; i<L_SLOW && pytanie[i]!=buf[0]; i++);
            if (i<L_SLOW)
                sendto(gniazdo, odpowiedz[i], strlen(odpowiedz[i]), 0, (struct sockaddr*) &cli, len);
        }
    }

    close(gniazdo);
    printf("KONIEC DZIALANIA SERWERA\n");
    return 0;
}