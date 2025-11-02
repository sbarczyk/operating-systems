#include <sys/stat.h>
#include <fcntl.h>

#include <sys/types.h>  
#include <stdio.h>      
#include <stdlib.h>     
#include <unistd.h>     
#include <string.h>     

/* 
Wstaw we wskazanych miejscach odpowienie deskryptory, sposrod 
fd0, fd1, fd2, by uzyskac na terminalu nastepujacy efekt dzialania 
programu:

Hello,
Hello, 12345678
HELLO, 12345678
WITAM! 12345678

Prosze nie wprowadzac innych zmian do programu niz wskazane powyzej w miejscach oznaczonych komentarzami
*/

int
main(int argc, char *argv[])
{
    int fd0, fd1, fd2; 
#define file "a"
    char cmd[] = "cat " file "; echo";

    fd1 = open(file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);  // "open fd1"
    if (fd1 == -1)
    {
        fprintf(stderr, "open fd1");
        exit(-1);
    }

    fd2 = dup(fd1);  // "dup"
    if (fd2 == -1)
    {
        fprintf(stderr, "dup");
        exit(-1);
    }

    fd0 = open(file, O_RDWR);  // "open fd0"
    if (fd0 == -1)
    {
        fprintf(stderr, "open fd0");
        exit(-1);
    }

    if (write(fd1, "Hello,", 6) == -1)  // write1
    {
        fprintf(stderr, "write1");
        exit(-1);
    }

    system(cmd);  // → Hello,

    if (write(fd2, " 12345678", 9) == -1)  // write2
    {
        fprintf(stderr, "write2");
        exit(-1);
    }

    system(cmd);  // → Hello, 12345678

    if (lseek(fd0, 0, SEEK_SET) == -1)
    {
        fprintf(stderr, "lseek");
        exit(-1);
    }

    if (write(fd0, "HELLO,", 6) == -1)  // write3
    {
        fprintf(stderr, "write3");
        exit(-1);
    }

    system(cmd);  // → HELLO, 12345678

    if (lseek(fd1, 0, SEEK_SET) == -1)  // potrzebne! żeby nadpisać początek
    {
        fprintf(stderr, "lseek before write4");
        exit(-1);
    }

    if (write(fd1, "WITAM!", 6) == -1)  // write4
    {
        fprintf(stderr, "write4");
        exit(-1);
    }

    system(cmd);  // → WITAM! 12345678

    if (close(fd0) == -1)
    {
        fprintf(stderr, "close output");
        exit(-1);
    }

    if (close(fd1) == -1)
    {
        fprintf(stderr, "close output");
        exit(-1);
    }

    if (close(fd2) == -1)
    {
        fprintf(stderr, "close output");
        exit(-1);
    }

    exit(EXIT_SUCCESS);
}