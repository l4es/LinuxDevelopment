#include "client.h"

using namespace std;

Client::Client() : m_IsConnected(false)
{
  m_pName = (char *) malloc(MAX_NAME_LENGHT+1);
}

Client::~Client() {

}

void Client::setName(const char *name) {
  //Copies at most MAX_NAME_LENGHT + 1 (including '\0')
  snprintf(m_pName, MAX_NAME_LENGHT+1, name);
}

char *Client::getName() {
  return m_pName;
}

void Client::setSocket(int socket) {
	m_Sock = socket;
}

int Client::getSocket() {
	return m_Sock;
}

void Client::setId(int id) {
  m_Id = id;
}

int Client::getId() {
  return m_Id;
}
