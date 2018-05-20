#include <signal.h>
#include <stdio.h>
#include <unistd.h>
struct sigaction act_open;

void ouch(int sig)
{
    printf("OUCH! - I got signal %d\n", sig);
    
    // Set SIGINT back to the default action
    act_open.sa_handler = SIG_DFL;
    sigaction(SIGINT, &act_open, 0);

}

int main()
{
    act_open.sa_flags = 0;
    
  // Create a mostly open mask -- only masking SIGINT
    sigemptyset(&act_open.sa_mask);
    sigaddset(&act_open.sa_mask, SIGINT);

    act_open.sa_handler = ouch;
    

    sigaction(SIGINT, &act_open, 0);

  while(1) {
    printf("Hello World!\n");
    sleep(1);
  }
}
