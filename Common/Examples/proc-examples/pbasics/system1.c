#include <stdlib.h>
#include <stdio.h>

int main()
{   int retcode;
    printf("Running ps with system\n");
// Synchronous call : We only return when call is finished.
    retcode = system("ps -ax");
    printf("Done. retcode = %d\n", retcode);
    // Expected return codes:
    // 127 -- shell can't be started to run the command
    // -1 -- another error (worthwhile checking sterror(errno) in this case
    // anything else is probably the executed program's return code
    
    exit(0);
}

