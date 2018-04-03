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
#ifndef I2CNET_SOCKET_SOCKET_H_
#define I2CNET_SOCKET_SOCKET_H_

#include <string>
#include <vector>

namespace i2cnet {
namespace socket {

class Socket {
public:
	virtual ~Socket() {}

	virtual std::string const& getRemoteEndpoint() const = 0;

	virtual void send(std::vector<char>& data) = 0;
	virtual void receive(std::vector<char>& data) = 0;

	virtual void close() = 0;
};

}
}

#endif
