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
void redirection(int argc, char *argv[])
{
int i=1, j=1, input_redirected=0;
char *str, *input_filename, *output_filename;
   
// Need to look for redirections : > file1   or < file2
// When they are found we use freopen to make them happen and then
// the 2 parameters need to be removed from the argv string array

   while( i <= argc) // <= copies the NULL in the last entry
    {
        argv[j] = argv[i];
        str = argv[i];
        if (str == NULL) break;

        if (str[0] == '<')
        {
            // redirect input
            input_redirected = 1;
            if (strlen(str) > 1)
            {
               input_filename = &str[1]; 
               i += 1;
            }
            else 
            {
                input_filename = argv[i+1];
                i += 2;
            }
            if (freopen(input_filename,"r", stdin)== NULL)
            {
                fprintf(stderr, "could not redirect stdin from file(%s):%s\n",
                    input_filename, strerror(errno));
                exit(-10);
            }

        }        
        else if (str[0] == '>')
        {
            // redirect output
            if (strlen(str) > 1)
            {
                output_filename = &str[1];
                i += 1;
            }
            else 
            {
                output_filename = argv[i+1];
                i += 2;
            }
        
        
            if (freopen(output_filename,"w", stdout)== NULL)
            {
                fprintf(stderr, "could not redirect stdout from file(%s):%s\n",
                    output_filename, strerror(errno));
                exit(-11);
            }
            i += 2;
        }
        else
            {
                i += 1;
                j += 1;       
            } 
    }// end of while loop

    if (!input_redirected)
    {
        if (freopen("/dev/tty","r", stdin)== NULL)
            {
                fprintf(stderr, "could not redirect stdin from file(%s):%s\n",
                    "/dev/tty", strerror(errno));
                exit(-12);
            }
    }

}

void child(int argc, char * progname, char * argv[])
{
   redirection(argc, argv);

   signal(SIGINT, SIG_DFL);
   signal(SIGCLD, SIG_DFL);

   execvp(progname, argv);
   printf("Should NOT get here !!");
   exit(-1);   
} 

int local_cmd(int argc, char * progname, char * argv[])
{
    if (strcmp(progname, "EXPORT") == 0)
    {
        // Fill in code for:
        // EXPORT xyz=abc
        return 1;
    }
    else if (strcmp(progname, "cd") == 0)
    {
        // Fill in code for cd
        return 1;
    }
    else if (strcmp(progname, "pwd") == 0)
    {
        // Fill in code for pwd
        return 1;
    }
    else if (strcmp(progname, "exit") == 0)
    {
        exit(0);
    }
    else 
        return 0;
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
    if (local_cmd(argc, argv[0],  argv))
        return;

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
 
    signal(SIGINT, mysig);
    signal(SIGCLD, mysig);

    for (;;)
    {
        printf("\nmsh==>");
        str = fgets(cbuff, sizeof(cbuff), stdin);
        len = strlen(str);
        // Strip any trailing new lines
        if (str[len-1] == '\n') str[len-1] = 0;
        process(str);
    }
    
    exit(0);
}
