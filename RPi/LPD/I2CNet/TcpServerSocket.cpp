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
#include <stdlib.h>
#include <unistd.h>
#include <i2cnet/logging/Utils.h>
#include <i2cnet/socket/TcpServerSocket.h>
#include <i2cnet/socket/TcpSocket.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <thread>

namespace i2cnet {
namespace socket {

class GaiServicePimpl {
public:
	GaiServicePimpl(std::string const& host, std::string const& port) {
		struct addrinfo hints;

		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = AF_UNSPEC;
		int rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &results);

		if (rc != 0) {
			freeaddrinfo(results);
			throw std::runtime_error(gai_strerror(rc));
		}
	}

	~GaiServicePimpl() {
		if (results) {
			freeaddrinfo(results);
		}
		results = 0;
	}

	struct addrinfo* getResult() {
		return results;
	}

private:
	struct addrinfo *results = 0;
};

class TcpServerSocketPimpl {
public:
	TcpServerSocketPimpl(std::string const& host, std::string const& port, i2cnet::logging::Logger& logger) : socket_(0), running_(true), logger_(logger), connectionListener_(0) {
		GaiServicePimpl gai(host, port);
		auto addressInformation = gai.getResult();

		socket_ = ::socket(addressInformation->ai_family, addressInformation->ai_socktype, addressInformation->ai_protocol);
		if (socket_ == -1) {
			throw std::runtime_error(i2cnet::logging::Utils::format("Failed to create socket: %s", strerror(errno)));
		}

		int rc = bind(socket_, addressInformation->ai_addr, addressInformation->ai_addrlen);
		if (rc != 0) {
			throw std::runtime_error(i2cnet::logging::Utils::format("Failed to bind socket: %d", rc));
		}

		rc = listen(socket_, 1);
		if (rc != 0) {
			throw std::runtime_error(i2cnet::logging::Utils::format("Failed to listen on socket: %d", rc));
		}
	}

	void stop() {
		running_ = false;
		close(socket_);
		socket_ = 0;
	}

	void run() {
		char clientHost[256];
		char clientService[256];
		struct sockaddr clientAddress;
		socklen_t clientAddressLength = sizeof(clientAddress);

		while (running_) {
			if (!connectionListener_) {
				std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_PERIOD));
				continue;
			}
			int clientSocket = accept(socket_, &clientAddress, &clientAddressLength);

			if (clientSocket == -1 && running_) {
				throw std::runtime_error("Failed to accept socket.");
			} else if (!running_) {
				return;
			}

			std::string endpoint("Unknown");
			if (getnameinfo(&clientAddress, clientAddressLength, clientHost, sizeof(clientHost), clientService, sizeof(clientService), NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
				logger_.warn(I2CNET_AT, "Unknwon connection received.");
			} else {
				endpoint = std::string(clientHost) + ":" + clientService;
				logger_.info(I2CNET_AT, "Connection received from: %s:%s", clientHost, clientService);
			}

			if (connectionListener_) {
				connectionListener_->connected(std::unique_ptr<Socket>(new TcpSocket(endpoint, clientSocket)));
			}
		}
	}

	~TcpServerSocketPimpl() {
		running_ = false;
		if (socket_) {
			close(socket_);
		}
	}

	void setConnectionListener(ConnectionListener& connectionListener) {
		connectionListener_ = &connectionListener;
	}

private:
	int socket_;
	volatile bool running_;
	i2cnet::logging::Logger& logger_;
	ConnectionListener* connectionListener_;

	const int RETRY_PERIOD = 100;
};

void startThread(TcpServerSocketPimpl* pimpl) {
	pimpl->run();
}

TcpServerSocket::TcpServerSocket(std::string const& host, std::string const& port, std::unique_ptr<i2cnet::logging::Logger> logger) : logger_(std::move(logger)) {
	pimpl_.reset(new TcpServerSocketPimpl(host, port, *(logger_.get())));
	serverThread_.reset(new std::thread(startThread, pimpl_.get()));
}

TcpServerSocket::~TcpServerSocket() {
	pimpl_->stop();
	serverThread_->join();
}

void TcpServerSocket::setConnectionListener(ConnectionListener& connectionListener) {
	pimpl_->setConnectionListener(connectionListener);
}

}
}

