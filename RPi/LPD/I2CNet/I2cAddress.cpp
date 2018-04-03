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
#include <i2cnet/model/I2cAddress.h>
#include <iomanip>
#include <sstream>

namespace i2cnet {
namespace model {

I2cAddress::I2cAddress(unsigned int bus, unsigned int device, unsigned int location) : bus_(bus), device_(device), location_(location) {
}

I2cAddress::~I2cAddress() {
}

unsigned int I2cAddress::getBus() const {
	return bus_;
}

unsigned int I2cAddress::getDevice() const {
	return device_;
}

unsigned int I2cAddress::getLocation() const {
	return location_;
}

std::ostream& operator<<(std::ostream& stream, I2cAddress const& address) {
	std::ostringstream oss;
	oss << std::hex << std::setfill('0') << "I2cAddress(bus = 0x" << std::setw(2) << address.getBus() << ", device = 0x" << std::setw(2) << address.getDevice() << ", location = 0x" << std::setw(2) << address.getLocation() << ")";
	return stream << oss.str();
}

}
}
