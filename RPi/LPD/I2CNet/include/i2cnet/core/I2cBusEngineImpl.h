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
#ifndef I2CNET_CORE_I2CBUSENGINEIMPL_H_
#define I2CNET_CORE_I2CBUSENGINEIMPL_H_
#include <i2cnet/core/I2cBusEngine.h>
#include <i2cnet/core/I2cRequestHolder.h>
#include <i2cnet/io/BusDevice.h>
#include <i2cnet/logging/LoggerFactory.h>
#include <i2cnet/model/I2cRequest.h>
#include <i2cnet/model/I2cResponse.h>
#include <condition_variable>
#include <string>
#include <deque>
#include <memory>
#include <thread>

namespace i2cnet {
namespace core {

class I2cBusEngineImpl : public I2cBusEngine {
public:
	I2cBusEngineImpl(std::string const& device, std::unique_ptr<i2cnet::logging::Logger> logger = i2cnet::logging::LoggerFactory::getLogger("I2cBusEngineImpl"));
	virtual ~I2cBusEngineImpl();

	virtual std::string const& getDevice() const;

	virtual void run();
	virtual void processRequest(i2cnet::model::I2cRequest const& request, i2cnet::model::I2cResponse& response);

private:
	std::unique_ptr<i2cnet::logging::Logger> logger_;

	std::string device_;
	i2cnet::io::BusDevice busDevice_;

	volatile bool running_;
	std::mutex requestQueueMutex_;
	std::unique_ptr<std::thread> runThread_;
	std::condition_variable requestQueueCondition_;
	std::deque<std::unique_ptr<I2cRequestHolder>> requestQueue_;

};

void executeBus(I2cBusEngineImpl* bus);

}
}

#endif
