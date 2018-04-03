/**
 * Copyright 2016 Graeme Farquharson
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <i2cnet/logging/Utils.h>
#include <i2cnet/socket/TcpSocket.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <stdexcept>
#include <errno.h>
#include <string.h>
#include <unistd.h>

namespace i2cnet {
namespace socket {

TcpSocket::TcpSocket(std::string const& endpoint, int socket) : socket_(socket), endpoint_(endpoint) {
}

TcpSocket::~TcpSocket() {
	::close(socket_);
}

std::string const& TcpSocket::getRemoteEndpoint() const {
	return endpoint_;
}

void TcpSocket::send(std::vector<char>& data) {
	int remaining = data.size();
	while (remaining > 0) {
		int size = ::send(socket_, &data[data.size() - remaining], remaining, 0);
		if (size == -1) {
			throw std::runtime_error(i2cnet::logging::Utils::format("Failed to write to socket: %s", strerror(errno)));
		}
		remaining -= size;
	}
}

void TcpSocket::receive(std::vector<char>& data) {
	char buffer[1024];
	int size = ::recv(socket_, buffer, sizeof(buffer), 0);
	if (size == 0) {
		return;
	} else if (size == -1) {
		throw std::runtime_error(i2cnet::logging::Utils::format("Failed to read from socket: %s", strerror(errno)));
	}
	data.insert(data.end(), buffer, buffer + size);
}

void TcpSocket::close() {
	::close(socket_);
	socket_ = 0;
}

}
}

