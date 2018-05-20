#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

void child(int argc, char * progname, char * argv[])
{
   execvp(progname, argv);
   printf("Shouldn't get here !!");
   exit(-1);   
} 

void process(char *buff)
{
    char *str, *left_off, *argv[50];
    int argc = 0;
    pid_t child_pid, pid_ret;

// Parse command line into an array of strings : argv[]

    str = strtok_r(buff, " \t", &left_off);
    while (str != NULL)
    {
        argv[argc++] = str;
        str = strtok_r(NULL, " \t", &left_off);
    }
    if (argc == 0) return; // exit if nothing there

    child_pid = fork();
    if (child_pid == 0)
    {
         argv[argc] = NULL; // last entry must be NULL
         child(argc, argv[0], argv);
         // Shouldn't be getting here
         printf("How did I get here?\n");
         exit(-1);
    }
    if (child_pid < 0)
    {
        fprintf(stderr,"Unable to fork: %s\n", strerror(errno));
        return;
    }       
    
}

int main(int argc, char * argv[]) 
{
    pid_t pid;
    char cbuff[500];
    char *str;
    int len;

    for (;;)
    {
        printf("\nmsh1==>");
        str = fgets(cbuff, sizeof(cbuff), stdin);
        len = strlen(str);
        // Strip any trailing new lines
        if (str[len-1] == '\n') str[len-1] = 0;
        process(str);
    }
    
    exit(0);
}
