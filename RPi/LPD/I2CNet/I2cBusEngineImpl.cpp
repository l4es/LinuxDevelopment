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
#include <i2cnet/core/I2cBusEngineImpl.h>
#include <i2cnet/logging/LoggerFactory.h>
#include <exception>

namespace i2cnet {
namespace core {

I2cBusEngineImpl::I2cBusEngineImpl(std::string const& device, std::unique_ptr<i2cnet::logging::Logger> logger) : logger_(std::move(logger)), device_(device), busDevice_(device), running_(true) {
}

I2cBusEngineImpl::~I2cBusEngineImpl() {
	logger_->debug(I2CNET_AT, "Destroying I2cBusEngineImpl");
	{
		std::unique_lock<std::mutex> queueLock(requestQueueMutex_);
		running_ = false;
		requestQueueCondition_.notify_all();
	}
	if (runThread_.get()) {
		runThread_->join();
	}
	logger_->debug(I2CNET_AT, "Destroyed I2cBusEngineImpl");
}

std::string const& I2cBusEngineImpl::getDevice() const {
	return device_;
}

void I2cBusEngineImpl::run() {
	while (running_) {
		std::unique_ptr<I2cRequestHolder> requestItem;
		{
			std::unique_lock<std::mutex> queueLock(requestQueueMutex_);
			while (requestQueue_.size() == 0 && running_) {
				requestQueueCondition_.wait(queueLock);
			}
			if (!running_) {
				continue;
			}

			requestItem = std::move(requestQueue_.front());
			requestQueue_.pop_front();
			requestItem->waitForLock();

			i2cnet::model::I2cRequest const& request = requestItem->getRequest();
			i2cnet::model::I2cResponse& response = requestItem->getResponse();
			i2cnet::model::I2cAddress const& address = request.getAddress();

			try {
				if (requestItem->getRequest().getData().size() > 0) {
					busDevice_.write(address.getDevice(), address.getLocation(), requestItem->getRequest().getData());
				}
				if (requestItem->getRequest().getResponseLength() > 0) {
					busDevice_.read(address.getDevice(), address.getLocation(), request.getResponseLength(), response.getData());
				}
			} catch (std::exception& e) {
				logger_->error(I2CNET_AT, "I2C Bus Failure: %s", e.what());
			}
			requestItem->processComplete();
		}
	}
}

void I2cBusEngineImpl::processRequest(i2cnet::model::I2cRequest const& request, i2cnet::model::I2cResponse& response) {
	std::unique_lock<std::mutex> queueLock(requestQueueMutex_);
	if (!runThread_.get()) {
		runThread_.reset(new std::thread(executeBus, this));
	}

	requestQueue_.push_back(std::unique_ptr<I2cRequestHolder>(new I2cRequestHolder(request, response)));
	I2cRequestHolder* holder = requestQueue_.back().get();
	requestQueueCondition_.notify_all();
	queueLock.unlock();

	holder->waitForProcess();
}

void executeBus(I2cBusEngineImpl* bus) {
	i2cnet::logging::LoggerFactory::getLogger("I2cBusEngineImpl")->debug(I2CNET_AT, "Executing new bus: %s", bus->getDevice().c_str());
	bus->run();
	i2cnet::logging::LoggerFactory::getLogger("I2cBusEngineImpl")->debug(I2CNET_AT, "Terminating bus: %s", bus->getDevice().c_str());
}

}
}
