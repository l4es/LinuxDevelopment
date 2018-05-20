#include <stdlib.h>
#include <stdio.h>

int main()
{
    printf("Running ps with system\n");
    // Asynchronous: The Ampersand tells the shell,
    //  not to wait for the completion of this process
    system("ps -ax &");
    printf("Done.\n");
    exit(0);
}

