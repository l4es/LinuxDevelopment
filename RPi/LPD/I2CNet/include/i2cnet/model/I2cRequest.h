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
#ifndef I2CNET_MODEL_I2CREQUEST_H_
#define I2CNET_MODEL_I2CREQUEST_H_
#include <i2cnet/model/I2cAddress.h>
#include <vector>

namespace i2cnet {
namespace model {

class I2cRequest {
public:
	I2cRequest(I2cAddress const& address, unsigned int responseLength, std::vector<char> const& data);
	virtual ~I2cRequest();

	I2cAddress const& getAddress() const;
	std::vector<char> const& getData() const;
	unsigned int getResponseLength() const;

private:
	I2cAddress address_;
	std::vector<char> data_;
	unsigned int responseLength_;
};

}
}

#endif
