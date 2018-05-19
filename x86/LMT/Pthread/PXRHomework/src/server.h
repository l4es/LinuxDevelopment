#ifndef _SERVER_H_
#define _SERVER_H_

#include <iostream>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
 #include <arpa/inet.h> 

#include "customthread.h"
#include "client.h"

using namespace std;

class Server {

  private:
    static vector<Client> m_Clients;

    //Socket stuff
    int m_Port;
    char *m_IpAddr;
    int m_ServerSock;
    int m_ClientSock;
    struct sockaddr_in m_ServerAddr;
    struct sockaddr_in m_ClientAddr;

  public:
    Server(int port, char * ip);
    virtual ~Server();

    void dispatchConnection();
    static void * handleClient(void *args);

  private:
    Server();

    static void sendToAll(char *message);
    static void addClient(Client *c);
    static void removeClient(Client *c);
    static void listClients();
    static int findClientIndex(Client *c); 
};

#endif
