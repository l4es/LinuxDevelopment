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
#ifndef INCLUDE_I2CNET_APP_APPLICATIONCONNECTIONLISTENER_H_
#define INCLUDE_I2CNET_APP_APPLICATIONCONNECTIONLISTENER_H_
#include <i2cnet/logging/LoggerFactory.h>
#include <i2cnet/socket/ConnectionListener.h>

namespace i2cnet {
namespace app {

class ApplicationThread;

class ApplicationConnectionListener : public i2cnet::socket::ConnectionListener {
public:
	ApplicationConnectionListener(std::unique_ptr<i2cnet::logging::Logger> logger = i2cnet::logging::LoggerFactory::getLogger("ApplicationConnectionListener"));
	virtual ~ApplicationConnectionListener();

	virtual void connected(std::unique_ptr<i2cnet::socket::Socket> socket);

private:
	std::unique_ptr<i2cnet::logging::Logger> logger_;

	static void startThread(std::unique_ptr<ApplicationThread> applicaionThread);
};

}
}

#endif
