#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>

#define L_PYTAN 10

int main(void){
 int status,gniazdo,i;
 struct sockaddr_in ser,cli;
 char buf[200];
 char pytanie[]="abccbahhff";

 gniazdo = socket(AF_INET,SOCK_STREAM,0);
 if (gniazdo==-1) {printf("blad socket\n"); return 0;}

 memset(&ser, 0, sizeof(ser));
 ser.sin_family = AF_INET;
 ser.sin_port = htons((9000));
 ser.sin_addr.s_addr = inet_addr("127.0.0.1");

 //nawiaz polaczenie z IP 127.0.0.1 i usluga na porcie takim samym jak ustaliles w serwerze, zwroc status
 status = connect(gniazdo, (struct sockaddr*) &ser, sizeof(ser));
 if (status<0) {printf("blad 01\n"); return 0;}

 for (i=0; i<L_PYTAN; i++){
  status = write(gniazdo, pytanie+i, 1);
  if (status < 0) {printf("błąd write"); return 0;}
  //odczytaj dane otrzymane z sieci, wpisz do tablicy 'buf' i wyĹwietl na standardowym wyjĹciu (ekranie)
  status = read(gniazdo, &buf, sizeof(buf) - 1);
  buf[status] = '\0';
  if (status < 0) {printf("błąd read"); return 0;}
  printf("%s ", buf);
 }
 printf ("\n");

close(gniazdo);  
printf("KONIEC DZIALANIA KLIENTA\n");
return 0;
}