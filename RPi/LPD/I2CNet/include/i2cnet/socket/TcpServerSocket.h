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
#ifndef I2CNET_SOCKET_TCPSERVERSOCKET_H_
#define I2CNET_SOCKET_TCPSERVERSOCKET_H_

#include <memory>
#include <string>
#include <thread>
#include <i2cnet/socket/ServerSocket.h>
#include <i2cnet/logging/LoggerFactory.h>

namespace i2cnet {
namespace socket {

class TcpServerSocketPimpl;

class TcpServerSocket : public ServerSocket {
public:
	TcpServerSocket(std::string const& host, std::string const& port, std::unique_ptr<i2cnet::logging::Logger> logger = i2cnet::logging::LoggerFactory::getLogger("TcpServerSocket"));
	virtual ~TcpServerSocket();

	virtual void setConnectionListener(ConnectionListener& connectionListener);

private:
	std::unique_ptr<std::thread> serverThread_;
	std::unique_ptr<TcpServerSocketPimpl> pimpl_;
	std::unique_ptr<i2cnet::logging::Logger> logger_;
};

}
}

#endif
