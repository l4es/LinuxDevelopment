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
#include <i2cnet/core/I2cEngineImpl.h>

namespace i2cnet {
namespace core {

I2cEngineImpl::I2cEngineImpl(std::unique_ptr<i2cnet::logging::Logger> logger) : logger_(std::move(logger)) {
}

I2cEngineImpl::~I2cEngineImpl() {
}

void I2cEngineImpl::registerBus(int busIndex, std::string const& device) {
	if (buses_.count(busIndex) != 0) {
		logger_->warn(I2CNET_AT, "A bus at index %d already exists, overriding.", busIndex);
	}
	buses_[busIndex] = std::unique_ptr<I2cBusEngine>(new I2cBusEngineImpl(device));
}

void I2cEngineImpl::processRequest(i2cnet::model::I2cRequest const& request, i2cnet::model::I2cResponse& response) {
	buses_.at(request.getAddress().getBus())->processRequest(request, response);
}

}
}
