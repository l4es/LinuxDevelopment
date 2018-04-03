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
#ifndef INCLUDE_I2CNET_CORE_I2CREQUESTHOLDER_H_
#define INCLUDE_I2CNET_CORE_I2CREQUESTHOLDER_H_

#include <i2cnet/model/I2cRequest.h>
#include <i2cnet/model/I2cResponse.h>
#include <mutex>
#include <condition_variable>

namespace i2cnet {
namespace core {

class I2cRequestHolder {
enum State {
	NOT_HELD,
	HELD,
	UNLOCKED
};

public:
	I2cRequestHolder(i2cnet::model::I2cRequest const& request, i2cnet::model::I2cResponse& response);
	virtual ~I2cRequestHolder();

	void waitForProcess();

	void waitForLock();
	void processComplete();

	i2cnet::model::I2cRequest const& getRequest();
	i2cnet::model::I2cResponse& getResponse();

private:
	State state_;
	std::mutex stateMutex_;
	std::condition_variable stateCondition_;

	bool requestFinished_;
	bool processFinished_;

	i2cnet::model::I2cRequest const& request_;
	i2cnet::model::I2cResponse& response_;
};

}
}

#endif
