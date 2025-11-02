#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>

#define L_PYTAN 10

int main(void){
    int status, gniazdo, i, lbajtow;
    struct sockaddr_in ser;
    char buf[200];
    char pytanie[] = "abccbahhhh";

    // stworz gniazdo UDP
    gniazdo = socket(AF_INET, SOCK_DGRAM, 0);
    if (gniazdo == -1) {printf("błąd socket\n"); return 0;}
    
    memset(&ser, 0, sizeof(ser));
    ser.sin_family = AF_INET;
    ser.sin_port = htons(9000);
    ser.sin_addr.s_addr = inet_addr("127.0.0.1");

    for (i = 0; i < L_PYTAN; i++) {
        // wysyłanie pojedynczej litery
        status = sendto(gniazdo, pytanie + i, 1, 0, (struct sockaddr*)&ser, sizeof(ser));
        if (status < 0) {printf("błąd sendto\n"); return 0;}

        // odbiór odpowiedzi od serwera
        socklen_t ser_len = sizeof(ser);
        lbajtow = recvfrom(gniazdo, buf, sizeof(buf)-1, 0, (struct sockaddr*)&ser, &ser_len);
        if (lbajtow < 0) {printf("błąd recvfrom\n"); return 0;}
        buf[lbajtow] = '\0';
        printf("%s ", buf);
    }
    printf("\n");

    close(gniazdo);
    printf("KONIEC DZIALANIA KLIENTA\n");
    return 0;
}