#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

int debug=1;

void read_status(int status)
{
  if (debug)printf("CHILD Just Died\n");
  if (WIFEXITED(status))
       if (debug)printf("child terminated normally\n"
              "      exit return code = %d\n", WEXITSTATUS(status)); 
   
  if (WIFSIGNALED(status))
       if(debug) printf("child terminated on an uncaught signal=%d\n" ,  
                WTERMSIG(status));

  if (WIFSTOPPED(status))
       if(debug) printf ("child is stopped, signal=\n",
            WSTOPSIG(status));

}

void mysig(int signum)
{
    pid_t pid_ret;
    int status;

    if(debug) printf("msh received signum = %d\n", signum);
    if (signum == SIGCLD)
    {    
        pid_ret = waitpid(-1, &status, WNOHANG);
        if (pid_ret > 0)
            read_status(status);
    }
   
}

void child(int argc, char * progname, char * argv[])
{
// child needs to set signals back to default setting
   signal(SIGINT, SIG_DFL);
   signal(SIGCLD, SIG_DFL);

   execvp(progname, argv);
   printf("Shouldn't get here !!");
   exit(-1);   
} 

void process(char *buff)
{
    char *str, *left_off, *argv[50];
    int argc = 0;
    int status, background = 0;
    pid_t child_pid, pid_ret;

    str = strtok_r(buff, " \t", &left_off);
    while (str != NULL)
    {
        argv[argc++] = str;
        str = strtok_r(NULL, " \t", &left_off);
    }
    if (argc == 0) return;

    if ( argc > 0 && strcmp(argv[argc-1],"&")==0)
    {
        background = 1;
        argc --;
    }
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
    if (background == 0)
    {
         pid_ret = waitpid(child_pid, &status, 0);
         read_status(status);
    }
    else
        if(debug) printf("Background: %s\n", argv[0]);
        
    
}

int main(int argc, char * argv[]) 
{
    pid_t pid;
    char cbuff[500];
    char *str;
    int len;

    if (argc > 1 && strcmp(argv[1],"-debug")==0)
        debug = 0;
 
    signal(SIGINT, mysig); // keep running through Control-C
    signal(SIGCLD, mysig); // Harvest our background Children Zombies

    for (;;)
    {
        printf("\nmsh2==>");
        str = fgets(cbuff, sizeof(cbuff), stdin);
        len = strlen(str);
        // Strip any trailing new lines
        if (str[len-1] == '\n') str[len-1] = 0;
        process(str);
    }
    
    exit(0);
}
