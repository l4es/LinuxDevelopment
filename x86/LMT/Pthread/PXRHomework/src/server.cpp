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
  
  CustomThread *cthread;

  socklen_t sockSize = sizeof(sockaddr_in);

  while(1) {
    cout << "Waiting new connection ... " << endl;

    int sock = accept(m_ServerSock, (struct sockaddr *) &m_ClientAddr, &sockSize);
    if(sock < 0) {
      cerr << "Error on accept";
    }
    else {
      // Assign new task to handle client via this socket
      cthread = new CustomThread();
      if (cthread != nullptr) {
        cthread->create((void *) Server::handleClient, &sock);
        cthread->detach(); // Avoid memleak detected by Valgrind for pthread_create()
        delete cthread;
	  }
	  else {
		  cerr << "Error on create new CustomThread()" << endl;
	  }
    }
  }
}

//Static handler for client socket
void *Server::handleClient(void *args) {
  char buffer[255], message[255];
  int n;

  // Copy thread argument
  int sock = *(int *) args;

  Client *client = new Client(sock);
  if (client == nullptr) {
	  cerr << "Error on create new Client(sock)" << endl;
	  //End thread
      return nullptr;
  }

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
      removeClient(client); 
      delete client;

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
      if (client->isConnected() != true) {
        client->connect();
        // First message = "HELLO" ?
        if (strcmp(buffer, "HELLO") == 0) {
          snprintf(message, sizeof message, "%s", "HI"); 
          cout << "-->: " << message << endl;
          int n = send(client->getSocket(), message, strlen(message), 0);
          cout << n << " bytes sent." << endl;
 
          // Continue listenning client on the openned socket
          continue;
        }
      }

      // Client violates the rule, close it
      cout << "Client " << client->getId() << " violates the rule, close it" << endl;
      close(client->getSocket());
      
      //Remove client from list
      removeClient(client); 
      delete client;

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
