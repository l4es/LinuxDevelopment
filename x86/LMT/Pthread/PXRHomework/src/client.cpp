#include "client.h"

using namespace std;

Client::Client() : m_Id(-1), m_Sock (-1), m_IsConnected(false)
{
}

Client::Client(int sock) : m_Id(-1), m_Sock (sock), m_IsConnected(false)
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

bool Client::isConnected() {
  return m_IsConnected;
}

void Client::setConnect() {
	m_IsConnected = true;
}

void Client::setDisconnect() {
	m_IsConnected = false;
}
