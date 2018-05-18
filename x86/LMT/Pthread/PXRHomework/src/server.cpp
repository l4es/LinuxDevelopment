#include "server.h"

using namespace std;

//Actually allocate clients
vector<Client> Server::m_Clients;

Server::Server() {

  //Initialize static mutex from MyThread
  CustomThread::initMutex();

  //For setsock opt (REUSEADDR)
  int opt = 1;

  //Init m_ServerSock and start listening
  m_ServerSock = socket(AF_INET, SOCK_STREAM, 0);
  memset(&m_ServerAddr, 0, sizeof(sockaddr_in));
  m_ServerAddr.sin_family = AF_INET;
  m_ServerAddr.sin_addr.s_addr = INADDR_ANY;
  m_ServerAddr.sin_port = htons(PORT);

//set master socket to allow multiple connections
  if (setsockopt(m_ServerSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0) 
    cerr << "Failed to setsockopt";
  if(bind(m_ServerSock, (struct sockaddr *) &m_ServerAddr, sizeof(sockaddr_in)) < 0)
    cerr << "Failed to bind";

  listen(m_ServerSock, 5);
}

/*
	AcceptAndDispatch();

	Main loop:
		Blocks at accept(), until a new connection arrives.
		When it happens, create a new thread to handle the new client.
*/
void Server::acceptAndDispatch() {
  
  Client *c;
  CustomThread *t;

  socklen_t cliSize = sizeof(sockaddr_in);

  while(1) {
    c = new Client();
	t = new CustomThread();

	//Blocks here;
    int sock = accept(m_ServerSock, (struct sockaddr *) &m_ClientAddr, &cliSize);
	if(sock < 0) {
	  cerr << "Error on accept";
	}
	else {
      c->setSocket(sock);
	  t->create((void *) Server::handleClient, c);
	}
  }
}

//Static
void *Server::handleClient(void *args) {
  Client *c = (Client *) args;
  char buffer[256-25], message[256];
  int n;

  //Add client in Static clients <vector> (Critical section!)
  addClient(c);

  while(1) {
    memset(buffer, 0, sizeof buffer);
    n = recv(c->getSocket(), buffer, sizeof buffer, 0);

    //Client disconnected?
    if(n == 0) {
      cout << "Client " << c->getName() << " diconnected" << endl;
      close(c->getSocket());
      
      //Remove client in Static clients <vector> (Critical section!)
      removeClient(c);

      //Send goodbye to others
      snprintf(message, sizeof message, "%s", "BYEBYE"); 
      cout << "Will send "<< message << " to all" << endl;
      sendToAll(message);
      break;
    }
    else if(n < 0) {
      cerr << "Error while receiving message from client: " << c->getName() << endl;
      break;
    }
    else {
      //Message received, process it
      cout << "Receive ["<< buffer << "]" << endl;

      if (c->m_IsConnected != true) {
        c->m_IsConnected = true;
        if (strcmp(buffer, "HELLO") == 0) {
          snprintf(message, sizeof message, "%s", "HI"); 
          cout << "Will send "<< message << " to " << c->getName() << endl;
          int n = send(c->getSocket(), message, strlen(message), 0);
          cout << n << " bytes sent." << endl;
          break;
        }
      }

      // Client violates the rule, close it
      cout << "Client " << c->getName() << " violates the rule" << endl;
      close(c->getSocket());
      
      //Remove client in Static clients <vector> (Critical section!)
      removeClient(c);
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
      n = send(Server::m_Clients[i].getSocket(), message, strlen(message), 0);
      cout << n << " bytes sent." << endl;
    }
   
  //Release the lock  
  CustomThread::unlockMutex("'sendToAll()'");
}
void Server::addClient(Client *c) {
  char buffer[255];
  //Add client in Static clients <vector> (Critical section!)
  CustomThread::lockMutex("addClient()");
    //Before adding the new client, calculate its id. (Now we have the lock)
    c->setId(Server::m_Clients.size());
    sprintf(buffer, "Client n.%d", c->getId());
    c->setName(buffer);
    cout << "Adding client with id: " << c->getId() << endl;
    Server::m_Clients.push_back(*c);

  CustomThread::unlockMutex("addClient()");
}

void Server::removeClient(Client *c) {
  //Remove client in Static clients <vector> (Critical section!)
  CustomThread::lockMutex("removeClient()");

  int index = Server::findClientIndex(c);
  cout << "Erasing user in position " << index << " whose name id is: "<< Server::m_Clients[index].getId() << endl;
  Server::m_Clients.erase(Server::m_Clients.begin() + index);

  CustomThread::unlockMutex("removeClient()");        
}

void Server::listClients() {
  for(size_t i=0; i<m_Clients.size(); i++) {
    cout << m_Clients.at(i).getName() << endl;
  }
}

/*
  Should be called when vector<Client> clients is locked!
*/
int Server::findClientIndex(Client *c) {
  for(size_t i=0; i<m_Clients.size(); i++) {
    if((Server::m_Clients[i].getId()) == c->getId()) 
      return (int) i;
  }
  cerr << "Client id not found." << endl;
  return -1;
}
