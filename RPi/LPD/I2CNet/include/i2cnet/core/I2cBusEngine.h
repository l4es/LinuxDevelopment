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
#ifndef I2CNET_CORE_I2CBUSENGINE_H_
#define I2CNET_CORE_I2CBUSENGINE_H_
#include <i2cnet/model/I2cRequest.h>
#include <i2cnet/model/I2cResponse.h>

namespace i2cnet {
namespace core {

class I2cBusEngine {
public:
	I2cBusEngine() {}
	virtual ~I2cBusEngine() {}

	virtual void processRequest(i2cnet::model::I2cRequest const& request, i2cnet::model::I2cResponse& response) = 0;

private:
	I2cBusEngine(I2cBusEngine const& other);
	I2cBusEngine const& operator=(I2cBusEngine const& other);
};

}
}

#endif
