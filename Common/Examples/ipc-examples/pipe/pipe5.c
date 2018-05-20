#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main()
{
    int data_processed, newfd;
    int file_pipes[2];
    const char some_data[] = "Four score and seven years ago "
                             " out fore fathers ......";
    pid_t fork_result;

    if (pipe(file_pipes) == 0) {
        fork_result = fork();
        if (fork_result == (pid_t)-1) {
            fprintf(stderr, "Fork failure");
            exit(EXIT_FAILURE);
        }

        if (fork_result == (pid_t)0) {
            close(0);
            dup2(file_pipes[0], STDIN_FILENO);
           // The book does the following, and I think
           // he is lucky it works. 
           // newfd = dup(file_pipes[0]);
           // printf("newfd = %d\n", newfd);
            close(file_pipes[0]);
            close(file_pipes[1]);

            execlp("od", "od", "-h", (char *)0);
            exit(EXIT_FAILURE);
        }
        else {
            close(file_pipes[0]);
            data_processed = write(file_pipes[1], some_data,
                                   strlen(some_data));
            close(file_pipes[1]);
            printf("%d - wrote %d bytes\n", (int)getpid(), data_processed);
        }
    }
    exit(EXIT_SUCCESS);
}
