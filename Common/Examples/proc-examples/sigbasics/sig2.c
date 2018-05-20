/*  In alarm.c, the first function, ding, simulates an alarm clock.  */
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h> 

static int alarm_fired = 0;

void mysig(int sig)
{
    int status;
    pid_t pid;
        
    printf("Signal %d \n", sig);

    if (sig == SIGALRM)
    {
       alarm_fired = 1;
    }
    if (sig == SIGCLD)
    {
       // harvest terminated DEFUNCT child process
       pid = waitpid(-1, &status, WNOHANG); 
       printf(" Child Process(%d) terminated with a status of %d\n",
            pid, status); 
    }
}

/*  In main, we tell the child process to wait for five seconds
    before sending a SIGALRM signal to its parent.  */

int main()
{
    int pid;

    printf("alarm application starting\n");

    if((pid = fork()) == 0) {
     // Child process
        sleep(5);
    // Wake up parent process with a SIGALRM
        kill(getppid(), SIGALRM);
        exit(0);
    }

/*  The parent process arranges to catch SIGALRM with a call to signal
    and then waits for the inevitable.  */

    printf("waiting for alarm to go off\n");
    (void) signal(SIGALRM, mysig);
    (void) signal(SIGCLD, mysig);
 
    do
    {
        pause(); // Wait for any signal call
    } while(!alarm_fired);
    
    printf("Ding!\007\n");

    printf("done\n");
    exit(0);
}
