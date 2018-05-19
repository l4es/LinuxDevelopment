#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <iostream>
#include <cstdio>
#include <cstdlib>

using namespace std;

class Client {
  private:
    int m_Id;
    int m_Sock;
    bool m_IsConnected;

  public:
    Client();
    Client(int sock);
    virtual ~Client();
    void setId(int id);
    int getId();    
    void setSocket(int socket);
    int getSocket();
    bool isConnected();
    void setConnect();
    void setDisconnect(); 
};

#endif
