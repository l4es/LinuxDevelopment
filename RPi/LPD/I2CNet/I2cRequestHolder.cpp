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
#include <i2cnet/core/I2cRequestHolder.h>

namespace i2cnet {
namespace core {

I2cRequestHolder::I2cRequestHolder(i2cnet::model::I2cRequest const& request, i2cnet::model::I2cResponse& response) : state_(I2cRequestHolder::State::NOT_HELD), requestFinished_(false), processFinished_(false), request_(request), response_(response) {
}

I2cRequestHolder::~I2cRequestHolder() {
	std::unique_lock<std::mutex> destroyLock(stateMutex_);
	while (!(requestFinished_ && processFinished_)) {
		stateCondition_.wait(destroyLock);
	}
}

void I2cRequestHolder::waitForProcess() {
	std::unique_lock<std::mutex> requestLock(stateMutex_);
	state_ = I2cRequestHolder::HELD;
	stateCondition_.notify_all();
	while (state_ != I2cRequestHolder::UNLOCKED) {
		stateCondition_.wait(requestLock);
	}
	requestFinished_ = true;
	stateCondition_.notify_all();
}

void I2cRequestHolder::waitForLock() {
	std::unique_lock<std::mutex> busLock(stateMutex_);
	while (state_ != I2cRequestHolder::HELD) {
		stateCondition_.wait(busLock);
	}
}

void I2cRequestHolder::processComplete() {
	std::unique_lock<std::mutex> busLock(stateMutex_);
	state_ = I2cRequestHolder::UNLOCKED;
	processFinished_ = true;
	stateCondition_.notify_all();
}

i2cnet::model::I2cRequest const& I2cRequestHolder::getRequest() {
	return request_;
}

i2cnet::model::I2cResponse& I2cRequestHolder::getResponse() {
	return response_;
}

}
}
