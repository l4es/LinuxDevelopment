#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>  
#include <netdb.h> 


int main(int argc, char * argv[])
{
    int sockfd;
    int len;
    struct sockaddr_in address;
 
    struct hostent *hp; 
    char * hostname = "gettysburg";// default host for server is gettysburg

    int result;
    char ch = 'A';
    

/*  Create a socket for the client.  */

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

/*  Name the socket, as agreed with the server.  */
    if (argc > 1) hostname = argv[1];
    hp = gethostbyname(hostname); 
    if (hp == NULL) 
    { 
        fprintf(stderr, "\n%s: unknown host.\n", hostname); 
        return -1; 
    } 

    address.sin_family = AF_INET;
   // Old line:  address.sin_addr.s_addr = inet_addr("127.0.0.1");
    memcpy(&address.sin_addr, hp->h_addr, hp->h_length); 
    address.sin_port = htons(17xx0);
    len = sizeof(address);

/*  Now connect our socket to the server's socket.  */

    result = connect(sockfd, (struct sockaddr *)&address, len);

    if(result == -1) {
        perror("oops: client2");
        exit(1);
    }

/*  We can now read/write via sockfd.  */

    write(sockfd, &ch, 1);
    read(sockfd, &ch, 1);
    printf("char from server = %c\n", ch);
    close(sockfd);
    exit(0);
}
