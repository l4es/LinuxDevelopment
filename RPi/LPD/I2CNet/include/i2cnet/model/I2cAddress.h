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
#ifndef I2CNET_MODEL_I2CADDRESS_H_
#define I2CNET_MODEL_I2CADDRESS_H_
#include <ostream>

namespace i2cnet {
namespace model {

class I2cAddress {
public:
	I2cAddress(unsigned int bus, unsigned int device, unsigned int location);
	virtual ~I2cAddress();

	unsigned int getBus() const;
	unsigned int getDevice() const;
	unsigned int getLocation() const;

private:
	unsigned int bus_;
	unsigned int device_;
	unsigned int location_;
};

std::ostream& operator<<(std::ostream& stream, I2cAddress const& address);

}
}

#endif
