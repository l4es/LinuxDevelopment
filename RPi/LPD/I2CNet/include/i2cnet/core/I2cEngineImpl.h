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
#ifndef I2CNET_CORE_I2CENGINEIMPL_H_
#define I2CNET_CORE_I2CENGINEIMPL_H_
#include <i2cnet/core/I2cBusEngine.h>
#include <i2cnet/core/I2cEngine.h>
#include <i2cnet/logging/LoggerFactory.h>
#include <map>
#include <memory>

namespace i2cnet {
namespace core {

class I2cEngineImpl : public I2cEngine {
public:
	I2cEngineImpl(std::unique_ptr<i2cnet::logging::Logger> logger = i2cnet::logging::LoggerFactory::getLogger("I2cEngineImpl"));
	virtual ~I2cEngineImpl();

	virtual void registerBus(int busIndex, std::string const& device);
	virtual void processRequest(i2cnet::model::I2cRequest const& request, i2cnet::model::I2cResponse& response);

private:
	std::unique_ptr<i2cnet::logging::Logger> logger_;
	std::map<int, std::unique_ptr<I2cBusEngine>> buses_;
};

}
}

#endif
