#include "server.h"
#include "errno.h"

using namespace std;

//Actually allocate clients
vector<Client> Server::m_Clients;

Server::Server() {

}

Server::Server(int port, char *ip) : m_Port(port), m_IpAddr(ip) {

  //Initialize static mutex from MyThread
  CustomThread::initMutex();

  //For setsock opt (REUSEADDR)
  int opt = 1;

  //Init m_ServerSock and start listening
  m_ServerSock = socket(AF_INET, SOCK_STREAM, 0);
  memset(&m_ServerAddr, 0, sizeof(sockaddr_in));
  m_ServerAddr.sin_family = AF_INET;
  m_ServerAddr.sin_addr.s_addr = inet_addr(m_IpAddr);
  m_ServerAddr.sin_port = htons(m_Port);

//set master socket to allow multiple connections
  if (setsockopt(m_ServerSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0) 
    cerr << "Failed to setsockopt";
  if(bind(m_ServerSock, (struct sockaddr *) &m_ServerAddr, sizeof(sockaddr_in)) < 0)
    cerr << "Failed to bind";

  listen(m_ServerSock, 5);
}


Server::~Server() {

}


void Server::dispatchConnection() {
  
  Client *client;
  CustomThread *cthread;

  socklen_t sockSize = sizeof(sockaddr_in);

  while(1) {
    cout << "New connection ... " << endl;
    client = new Client();
    cthread = new CustomThread();

    int sock = accept(m_ServerSock, (struct sockaddr *) &m_ClientAddr, &sockSize);
    if(sock < 0) {
      cerr << "Error on accept";
    }
    else {
      // Update socket of client and assign a task to handle client via this socket
      client->setSocket(sock); 
      cthread->create((void *) Server::handleClient, client);
    }
    delete cthread;
  }
}

//Static
void *Server::handleClient(void *args) {
  Client *client = (Client *) args;
  char buffer[255], message[255];
  int n;

  //Add client into list
  addClient(client);

  while(1) {
    memset(buffer, 0, sizeof buffer);
    cout << "Waiting client " << client->getId() << " ..." << endl;
    n = recv(client->getSocket(), buffer, sizeof buffer, 0);

    //Client disconnected?
    if(n == 0) {
      cout << "Client " << client->getId() << " diconnected" << endl;
      close(client->getSocket());
      
      //Remove client from list
      removeClient(client); delete client;

      //Send goodbye to others
      snprintf(message, sizeof message, "%s", "BYEBYE"); 
      cout << " ==>:"<< message << endl;
      sendToAll(message);

      // Stop communication with client
      return nullptr;
    }
    else if(n < 0) {
      cerr << "Error ("<< errno  << ") while receiving message from client: " << client->getId() << endl;
    }
    else {
      // Message received, process it
      cout << "<--:" << buffer << endl;
      // First time connected?
      if (client->m_IsConnected != true) {
        client->m_IsConnected = true;
        // First message = "HELLO" ?
        if (strcmp(buffer, "HELLO") == 0) {
          snprintf(message, sizeof message, "%s", "HI"); 
          cout << "-->: " << message << endl;
          int n = send(client->getSocket(), message, strlen(message), 0);
          cout << n << " bytes sent." << endl;
          continue;
        }
      }

      // Client violates the rule, close it
      cout << "Client " << client->getId() << " violates the rule, close it" << endl;
      close(client->getSocket());
      
      //Remove client from list
      removeClient(client); delete client;

      // Stop communication with client
      return nullptr;
    }
  }

  //End thread
  return nullptr;
}

void Server::sendToAll(char *message) {
  int n;

  //Acquire the lock
  CustomThread::lockMutex("'sendToAll()'");
 
    for(size_t i=0; i<m_Clients.size(); i++) {
      cout << "Sending to client " << m_Clients[i].getId() << endl;
      n = send(Server::m_Clients[i].getSocket(), message, strlen(message), 0);
      cout << n << " bytes sent." << endl;
    }
   
  //Release the lock  
  CustomThread::unlockMutex("'sendToAll()'");
}

void Server::addClient(Client *client) {
  //Add client in Static clients <vector> (Critical section!)
  CustomThread::lockMutex("addClient()");
    //Before adding the new client, calculate its id. (Now we have the lock)
    client->setId(Server::m_Clients.size());
//    cout << "Adding client with id: " << client->getId() << endl;
    Server::m_Clients.push_back(*client);

  CustomThread::unlockMutex("addClient()");
}

void Server::removeClient(Client *client) {
  //Remove client in Static clients <vector> (Critical section!)
  CustomThread::lockMutex("removeClient()");

  int index = Server::findClientIndex(client);
//  cout << "Erasing user in position " << index << " whose name id is: "<< Server::m_Clients[index].getId() << endl;
  Server::m_Clients.erase(Server::m_Clients.begin() + index);

  CustomThread::unlockMutex("removeClient()");        
}

/*
  Should be called when vector<Client> clients is locked!
*/
int Server::findClientIndex(Client *client) {
  for(size_t i=0; i<m_Clients.size(); i++) {
    if((Server::m_Clients[i].getId()) == client->getId()) 
      return (int) i;
  }
  cerr << "Client id not found." << endl;
  return -1;
}
