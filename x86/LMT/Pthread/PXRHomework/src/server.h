#ifndef __SERVER_H__
#define __SERVER_H__

#include <iostream>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>

#include "customthread.h"
#include "client.h"

#define PORT 30666

using namespace std;

class Server {

  private:
    static vector<Client> m_Clients;

    //Socket stuff
    int m_ServerSock;
    int m_ClientSock;
    struct sockaddr_in m_ServerAddr;
    struct sockaddr_in m_ClientAddr;
    char buff[256];

  public:
    Server();
    void acceptAndDispatch();
    static void * handleClient(void *args);

  private:
    static void sendToAll(char *message);
    static void addClient(Client *c);
    static void removeClient(Client *c);
    static void listClients();
    static int findClientIndex(Client *c); 
};

#endif
