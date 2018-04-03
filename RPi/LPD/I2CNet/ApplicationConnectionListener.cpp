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
#include <i2cnet/app/ApplicationConnectionListener.h>
#include <i2cnet/app/ApplicationThread.h>
#include <i2cnet/core/I2cContext.h>
#include <i2cnet/logging/LoggerFactory.h>
#include <thread>

namespace i2cnet {
namespace app {

ApplicationConnectionListener::ApplicationConnectionListener(std::unique_ptr<i2cnet::logging::Logger> logger) : logger_(std::move(logger)) {
}

ApplicationConnectionListener::~ApplicationConnectionListener() {
}

void ApplicationConnectionListener::connected(std::unique_ptr<i2cnet::socket::Socket> socket) {
	logger_->debug(I2CNET_AT, "Creating process thread for %s.", socket->getRemoteEndpoint().c_str());
	std::thread applicationThread(ApplicationConnectionListener::startThread, std::unique_ptr<ApplicationThread>(new ApplicationThread(i2cnet::core::I2cContext::getEngine(), std::move(socket))));
	applicationThread.detach();
}

void ApplicationConnectionListener::startThread(std::unique_ptr<ApplicationThread> applicaionThread) {
	try {
		applicaionThread->run();
	} catch (std::exception& e) {
		i2cnet::logging::LoggerFactory::getLogger("ApplicationConnectionListener")->warn(I2CNET_AT, "Connection to %s has been terminated: %s", applicaionThread->getRemoteEndpoint().c_str(), e.what());
	}
	i2cnet::logging::LoggerFactory::getLogger("ApplicationConnectionListener")->info(I2CNET_AT, "Connection closed: %s", applicaionThread->getRemoteEndpoint().c_str());
}

}
}
