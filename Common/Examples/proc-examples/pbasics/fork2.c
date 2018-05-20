#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

void read_status(int status)
{
  printf("CHILD Just Died\n");
  if (WIFEXITED(status))
       printf("child terminated normally\n"
              "      exit return code = %d\n", WEXITSTATUS(status)); 
   
  if (WIFSIGNALED(status))
       printf("child terminated on an uncaught signal=%d\n" ,  
                WTERMSIG(status));

  if (WIFSTOPPED(status))
       printf ("child is stopped, signal=\n",
            WSTOPSIG(status));

}

void status_check(pid_t pid)
{
    int pid_ret, status;

    pid_ret = waitpid(pid, &status, WNOHANG);
    printf("waitpid returns %d\n", pid_ret);
    if (pid_ret == 0) printf("CHILD STILL EXISTS\n");
    if (pid_ret < 0)
            printf(" Child doesn't exist: %s\n", strerror(errno));

    if (pid_ret > 0 ) read_status(status);
}

void parent(pid_t pid, int loop)
{
    int i;
    int status;
    pid_t pid_ret;
    printf("Created Child: %d\n", pid);
    for (i=0; i < loop; i++)
    {
        printf("parent: %d\n", i);
        sleep(1);
        status_check(pid);
    }
}
void child(int loop, int sigint_flag)
{
    int i;
    for (i=0; i < loop; i++)
    {
        printf("child: %d\n", i);
        sleep(1);
    }
    if (sigint_flag) 
        kill (getpid(), SIGINT);  
    exit(77); 
}

int main(int argc, char * argv[]) 
{
    pid_t pid;
    int child_loop_count=2;
    int parent_loop_count = 5;
    int sigint_flag = 0;

    if (argc >= 3)
    {
        child_loop_count = atoi(argv[1]);
        parent_loop_count = atoi(argv[2]);
    }
    if (argc >= 4)
        sigint_flag = 1;
    printf("child_loop_count(%d) parent_loop_count(%d)\n", 
        child_loop_count, parent_loop_count);
    if (sigint_flag)
       printf("child will die by signal\n");

    pid = fork();
    switch(pid) 
    {
    case -1:
        printf("Something serious, fork usually doesn't break:\n%s\n",
                strerror(errno));
        exit(1);
    case 0:
        child(child_loop_count, sigint_flag);
        printf("Shouldn't see this print statement\n");
        break;
    default:
        parent(pid, parent_loop_count);
        break;
    }
    exit(1);
}
