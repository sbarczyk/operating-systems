#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int global = 0;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }

    printf("Program name: %s\n", argv[0]);
    int local = 0;
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Error while creating process");
        return 2;
    }
    else if (pid == 0)
    {
        printf("child process\n");

        global++;
        local++;

        printf("child pid = %d, parent pid = %d\n", getpid(), getppid());
        printf("child's local = %d, child's global = %d\n", local, global);

        int ret = execl("/bin/ls", "ls", argv[1], (char *)NULL);

        perror("execl error");
        return ret;
    }
    else
    {
        printf("parent process\n");
        printf("parent pid = %d, child pid = %d\n", getpid(), pid);

        int status;
        wait(&status);

        if (WIFEXITED(status))
        {
            int child_exit_code = WEXITSTATUS(status);
            printf("Child exit code: %d\n", child_exit_code);
        }
        else if (WIFSIGNALED(status))
        {
            printf("Child terminated by signal: %d\n", WTERMSIG(status));
        }
        else
        {
            printf("Child terminated abnormally.\n");
        }

        printf("Parent's local = %d, parent's global = %d\n", local, global);

        return 0;
    }
}