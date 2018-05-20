#include <sys/types.h> 
#include <sys/wait.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <arpa/inet.h> 
#include <signal.h> 

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/time.h> 
#include <ctype.h>

void server(int aPort);
void myChild(int fd_client); 

// Signal handler to process terminated children
void mysig(int nsig) 
{ 
    int nStatus, nPid; 
    
    nPid = waitpid(-1, &nStatus, WNOHANG); 
    if (nPid > 0) printf("Child %d died\n", nPid);
    printf("\nSignal(%d) received\n****************************\n\n\n\n\n\n\n", 
        nsig); 
    signal(SIGCHLD, mysig); 
} 
 
main (int argc, char **argv) 
{ 
    int i, aPort; 
    
    if (argc < 2) 
    { 
        fprintf(stderr, 
            "usage:%s porta  \n" , argv[0]); 
        exit(1); 
    } 
    aPort = atoi(argv[1]);     
    
    printf("\nI will be an echo server for port %d\n", aPort);    
    
    signal(SIGCHLD, mysig); // To clean up terminated children
    server(aPort);
    
    exit(0); 
} 

// Server Loop for processing Echo Server requests.
// Any new connect to Echo Server results in spawning a child
// process to baby sit the connection

void server(int aPort)
{  
    struct sockaddr_in sin, fsin; 
    int nChildCnt = 0;   
    int fd_connect, fd_accept;   
    int fromlen, nOpt =1; 
    long pid;

    if ((fd_accept = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        perror("server: socket()"); 
        exit(1); 
    } 
  // The following makes it quicker to reclaim a port when you 
  // restart your application -- normally ports can't be reused for a time
    setsockopt(fd_accept, SOL_SOCKET, SO_REUSEADDR, &nOpt, sizeof(int)); 
    
    memset(&sin, 0, sizeof(sin)); 
    sin.sin_family = AF_INET; 
    sin.sin_port = htons(aPort); 
    sin.sin_addr.s_addr = htonl(INADDR_ANY); 
    
    if (bind(fd_accept, (struct sockaddr *)&sin, sizeof(sin)) < 0) 
    { 
        perror("server: bind()"); 
        exit(1); 
    } 
    
    if (listen(fd_accept, 5) < 0) 
    { 
        perror("server: listen()"); 
        exit(1); 
    } 
    
    fromlen = sizeof(fsin); 
    
    for (;;) 
    { 
        if ((fd_connect = accept(fd_accept, (struct sockaddr *)&fsin, &fromlen)) < 0) 
        { 
            perror("server:accept()"); 
            continue; /* hope this is normally due to a child death*/ 
        } 
        nChildCnt += 1; 
        printf("\n#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_\nCHILD(%d)\n", 
            nChildCnt); 
        fflush(stdout); 
        
        
        if ((pid = fork()) > 0) 
        { 
            /* Parent */ 
            close(fd_connect);
            printf("Child created: %d\n", pid); 
            // Go wait on another Connect in the accept call
            continue; 
        } 
        if (pid == 0) 
        { 
            /* Must be the child */ 
            myChild(fd_connect);  
            exit(0); // Don't want to continue
        } 
        if (pid < 0)
            printf("**** Can't fork CHILD\n");
    } 
    
    close(fd_accept);  

}

// Child process to manager one connect requests reads/writes 
// When the connection seems to go away, the child terminates

#define BUFF_SIZE 10000 

void myChild(int fd_client ) 
{ 
    int i, nBytes;
    unsigned char buffer[BUFF_SIZE]; 

    for (;;)
    {
        nBytes = read(fd_client, buffer, BUFF_SIZE);
        if (nBytes <= 0)
        {
          // We end up here when client closes its connection
            printf("\nRead Error on on %d\n", fd_client);
            break; 
        } 
        if (nBytes > 0) 
        { 
            printf("received on file descriptor: %d\n", fd_client);
            for (i= 0; i < nBytes; i++)
                buffer[i] = toupper(buffer[i]);
            if (write(fd_client, buffer, nBytes) <= 0) 
            { 
                printf("\nWrite Error on %d ==> ", fd_client);
                break; 
            } 
        }
    }    
    
}
