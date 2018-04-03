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
#include <i2cnet/model/I2cRequest.h>

namespace i2cnet {
namespace model {

I2cRequest::I2cRequest(I2cAddress const& address, unsigned int responseLength, std::vector<char> const& data) : address_(address), data_(data), responseLength_(responseLength) {
}

I2cRequest::~I2cRequest() {
}

I2cAddress const& I2cRequest::getAddress() const {
	return address_;
}

std::vector<char> const& I2cRequest::getData() const {
	return data_;
}

unsigned int I2cRequest::getResponseLength() const {
	return responseLength_;
}

}
}
