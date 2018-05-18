#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <iostream>
#include <cstdio>
#include <cstdlib>

#define MAX_NAME_LENGHT 20

using namespace std;

class Client {
  private:
    char *m_pName;
    int m_Id;
    int m_Sock;

  public:
    Client();
    virtual ~Client();
    void setId(int id);
    int getId();    
    void setSocket(int socket);
    int getSocket();
 
    bool m_IsConnected;
};

#endif