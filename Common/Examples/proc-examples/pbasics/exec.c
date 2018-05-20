#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main()
{
// Example of an argument list
// Note that we need a program name for argv[0]
char  *ps_argv[] = {"ps", "-af", NULL};

// Example environment, not terribly useful
char  *ps_envp[] = {"PATH=/bin:/usr/bin", "TERM=console", NULL};
int rc;



    printf("Running ps with execlp\n");
    
 //   rc=execl ("/bin/ps", "ps", "-af", NULL);
      rc=execlp("ps",      "ps", "-af", NULL);
 //   rc=execle("/bin/ps", "ps", "-af", NULL, ps_envp);
 //   rc=execv ("/bin/ps", ps_argv);
 //   rc=execvp ("ps",     ps_argv);
 //   rc=execve ("/bin/ps",ps_argv, ps_envp);
    
    printf("Should never get here: rc(%d) \nError: %s",
        rc, strerror(errno));
    exit(0);
}
