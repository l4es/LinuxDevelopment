#include <signal.h>
#include <stdio.h>
#include <unistd.h>
struct sigaction act_protected;

void ouch(int sig)
{
    printf("OUCH! - I got signal %d\n", sig);
}

int main()
{
    struct sigaction act_open;
    sigset_t signals_open, signals_protected;

// Create a protected interupt mask -- only unmasking SIGSEGV
    sigfillset(&signals_protected);
    sigdelset(&signals_protected, SIGSEGV);

// Create a mostly open interupt mask -- only masking SIGINT
    sigemptyset(&signals_open);
    sigaddset(&signals_open, SIGINT);


// Fill in sigaction -- only masking SIGINT    
    act_open.sa_flags = 0;
    sigemptyset(&act_open.sa_mask);
    sigaddset(&act_open.sa_mask, SIGINT);

    act_open.sa_handler = ouch;
    
    // during SIGINT call, further SIGINT calls are blocked
    sigaction(SIGINT, &act_open, 0);

  while(1) {
    sigprocmask(SIG_SETMASK, &signals_open, NULL);
// Now all signals are allowed except SIGINT
    printf("slightly critical only SIGINT blocked\n");
    sleep(5);
       
    // the following enables SIGINT 
    sigprocmask(SIG_UNBLOCK, &signals_open, NULL);
// Now all signals are allowed including SIGINT
    printf("Open completely to interrupts\n");
    sleep(5);    
    
    sigprocmask(SIG_BLOCK, &signals_protected, NULL);
// All signals are blocked except SIGSEGV
    printf("IMPORTANT DATABASE WORK complete protected -- except SIGSEGV\n");
    sleep(5);
      }
}

