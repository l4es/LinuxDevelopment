#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


void parent(pid_t pid, int loop)
{
    int i;
    printf("Created Child: %d\n", pid);
    for (i=0; i < loop; i++)
    {
        printf("parent: %d\n", i);
        sleep(1);
    }
}
void child(int loop)
{
    int i;
    for (i=0; i < loop; i++)
    {
        printf("child: %d\n", i);
        sleep(1);
    }
    exit(77); 
}

int main(int argc, char * argv[]) 
{
    pid_t pid;
    int child_loop_count=2;
    int parent_loop_count = 5;

    if (argc >= 3)
    {
        child_loop_count = atoi(argv[1]);
        parent_loop_count = atoi(argv[2]);
    }
    printf("child_loop_count(%d) parent_loop_count(%d)\n", 
        child_loop_count, parent_loop_count);

    pid = fork();
    switch(pid) 
    {
    case -1:
        printf("Something serious, fork usually doesn't break:\n%s\n",
                strerror(errno));
        exit(1);
    case 0:
     // Must be the child
        child(child_loop_count);
        printf("Shouldn't see this print statement\n");
        break;
    default:
     // Must be the parent
        parent(pid, parent_loop_count);
        break;
    }
    exit(1);
}
