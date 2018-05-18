#include "client.h"

using namespace std;

Client::Client() : m_IsConnected(false)
{
}

Client::~Client() {

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
