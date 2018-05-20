#include <sys/types.h> 
#include <sys/wait.h> 
#include <sys/socket.h> 
#include <netinet/in.h>  
#include <netdb.h> 
#include <arpa/inet.h> 
#include <signal.h> 

#include <errno.h>
#include <string.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/time.h> 

void process_cmds(FILE *fp);
int  connect_host(char * hostname, int port);
void disconnect_host(int socket);
void sendBuffer (int socket, char * sendBuff, int numBytes);
void receive(int socket, char * receiveBuff, int size_of_buff);
void receiveUntilDone(int socket, char * receiveBuff, int size_of_buff);

main (int argc, char * argv[])
{
    FILE *fp;
    
    if (argc > 1)
        {
          fp = fopen(argv[1], "r");
          if (fp == NULL)
          {
            fprintf(stderr,"Unable to open (%s) : %s\n",
                argv[1], strerror(errno));
            return;
          }
          process_cmds(fp);  
        }
        else
          process_cmds(stdin);
}

void process_cmds(FILE *fp)
{
    char *cmd, *hostname;
    char *str, *str2;
    int port;
    int socket_num = -1;
    char cmdBuff[500];
    char sendBuff[10000];
    char receiveBuff[30000];
    int total = 0;
    
    for (;;)
    {
        printf("Enter Cmd (-buff:  -buffend:  -send:  -connect:host:port: "
               " etc. \n");
        cmdBuff[0] = 0;
        cmd = fgets(cmdBuff, sizeof(cmdBuff), fp);
        if (cmd == NULL) break;   
        if (strlen(cmd) <= 1) break;    
        str = strchr(cmd, ':');
        if (str == NULL) 
        {
            printf("All commands terminate with :\n");
            continue; 
        }
        *str = 0;
        if (strcmp(cmd, "-buff")== 0)
        {
          //buff: logic
           total = 0;
           for (;;)
           {
                str = fgets(&sendBuff[total], sizeof(sendBuff)-total -1, fp);
                if (str == NULL) break;
                if (strncmp("-buffend:", str, strlen("-buffend:")) == 0)
                    break;
                total += strlen(str);                
           }
           sendBuff[total] = 0;
            
        } 
        else if(strcmp(cmd, "-buffnn") == 0)
            {
             //-buffnn: logic
                total = 0;
                str = fgets(sendBuff, sizeof(sendBuff), fp);
                if (str != NULL )
                {
                    str2 = strtok(str, "\r\n");
                    *str2 = 0;
                    total = strlen(str);
                }
            }
        else if (strcmp(cmd,"-send") == 0)
            sendBuffer(socket_num, sendBuff, total);
        else if (strcmp(cmd,"-receive") == 0)
            receive(socket_num, receiveBuff, sizeof(receiveBuff));
        else if (strcmp(cmd, "-receiveUntilDone") == 0)
            receiveUntilDone(socket_num, receiveBuff, sizeof(receiveBuff));                        
        else if (strcmp(cmd, "-connect") == 0)
         {
            hostname = strtok(&str[1], ":\n");
            str2 = strtok(NULL, ":\n");
            if (str2 != NULL)
                port = atoi(str2);
            socket_num = connect_host( hostname, port);
         }
        else if (strcmp(cmd,"-disconnect") == 0)
            disconnect_host(socket_num);
        else
            printf("illegal command: %s\n"
                "LEGAL cmds:\n"
                "-send:\n"
                "-receive:\n"
                "-receiveUntilDone:\n"
                "-buff:   This command is followed by data followed by -buffend:\n"
                "-buffend:\n"
                "-buffnn:  buffer with no newline\n"
                "-connect:hostname:port:\n"
                "-disconnect:\n",
                cmd);

    }
}
 
int  connect_host(char * hostname, int port)
{
     int socket_num = -1;
     struct sockaddr_in sin; 
     struct hostent *hp; 

    hp = gethostbyname(hostname); 
    if (hp == NULL) 
    { 
        fprintf(stderr, "\n%s: unknown host.\n", hostname); 
        return -1; 
    } 

     if ((socket_num  = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        { 
            fprintf(stderr, "socket err: %s\n", strerror(errno)); 
            return socket_num;
        } 
        else 
        {    
            sin.sin_family = AF_INET; 
            sin.sin_port = htons(port); 
            memcpy(&sin.sin_addr, hp->h_addr, hp->h_length); 
            
            if (connect(socket_num  , (struct sockaddr *)&sin, sizeof(sin)) < 0) 
            { 
                fprintf(stderr, "connect err:%s\n", strerror(errno)); 
                return -1;
            } 
            printf("CONNECT Successful  to host %s, port %d\n",
                hostname, port);
        } 
        return socket_num;
}

void disconnect_host(int socket_num)
{
    if (socket_num > 0)
        close(socket_num);
    printf("DISCONNECTED\n");
}

void sendBuffer (int socket_num, char * buff, int numBytes)
{
    int num;

    if (socket_num < 0)
    {
        fprintf(stderr, "Trying to send to a bad socket(%d)\n", socket_num);
        return;
    }
    printf("SENDING:\n%s\n", buff);
    num = write(socket_num, buff, numBytes);
    if (num < numBytes)
    {
        printf("Intended to send %d bytes, but only sent %d bytes\n",
            numBytes, num);
        if (num < 0)
            fprintf(stderr, "send err: %s\n", strerror(errno));
    } 
}

void receive(int socket_num, char * receiveBuff, int size_of_buff)
{
    int numBytes;

    if (socket_num < 0)
    {
        fprintf(stderr, "Trying to receive from a bad socket(%d)\n", socket_num);
        return;
    }
    numBytes = read(socket_num, receiveBuff, size_of_buff-1);
    if (numBytes > 0)
    {
        receiveBuff[numBytes] = 0;
        printf("RECEIVED(%d):\n%s\n",numBytes, receiveBuff);
    }
    else
    {
        fprintf(stderr, "read Err: %s\n", strerror(errno));
    }
}

void receiveUntilDone(int socket_num, char * receiveBuff, int size_of_buff)
{
    int numBytes;
    int total = 0;
    
    if (socket_num < 0)
    {
        fprintf(stderr, "Trying to receive from a bad socket(%d)\n", 
            socket_num);
        return;
    }

    do
    {
        if (total >= size_of_buff -1)
            break;
        numBytes = read(socket_num, &receiveBuff[total], size_of_buff-total-1);
        if (numBytes > 0) total += numBytes;
        else
        {
            fprintf(stderr, "read Err: %s\n", strerror(errno));
            break;
        }

    } while (numBytes > 0);
       
    if (total >= 0)
    {
        receiveBuff[total] = 0;
        printf("RECEIVED-UntilDone(%d):\n%s\n", total, receiveBuff);
    }    

}                        
