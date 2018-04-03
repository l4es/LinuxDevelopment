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
#include <sstream>
#include <stdexcept>

#include <fcntl.h>
#include <thread>
#include <i2cnet/io/BusDevice.h>
#include <memory.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

namespace i2cnet {
namespace io {

BusDevice::BusDevice(const std::string& busName) : busName_(busName) {
        fileDescriptor_ = open(busName_.c_str(), O_RDWR);
        if (fileDescriptor_ < 0) {
                std::ostringstream oss;
                oss << "Failed to open I2C bus " << busName_ << ".  Failed on open: " + fileDescriptor_;
                throw std::runtime_error(oss.str());
        }
}

BusDevice::~BusDevice() {
        close(fileDescriptor_);
}

void BusDevice::read(char address, char location, int length, std::vector<char>& data) {
        this->write(address, location, std::vector<char>());
        char buffer[length];
        int readAmount = ::read(fileDescriptor_, buffer, length);
        data.insert(data.end(), &buffer[0], &buffer[length]);
	std::this_thread::yield();
}

void BusDevice::write(char address, char location, const std::vector<char>& data) {
        this->setAddress(address);

        int bufferLength = data.size() + 1;
        char buffer[bufferLength];
        buffer[0] = location;
        memcpy(&buffer[1], &data[0], data.size());

        int rc = ::write(fileDescriptor_, buffer, bufferLength);
        if (rc != bufferLength) {
                std::ostringstream oss;
                oss << "Failed to write to I2C bus " << busName_ << " on address " << (int)address << ".  ";
                if (rc >= 0) {
                        oss << "Expected to write " << data.size() << " bytes but only wrote " << rc << " bytes.";
                } else {
                        oss << "Error code: " << rc;
                }
                throw std::runtime_error(oss.str());
        }
}

void BusDevice::setAddress(char address) {
        int rc = ioctl(fileDescriptor_, I2C_SLAVE, address);
        if (rc < 0) {
                std::ostringstream oss;
                oss << "Failed to select I2C bus " << busName_ << " and address " <<  (int)address << ".  Failed on ioctl: " + rc;
                throw std::runtime_error(oss.str());
        }
}

}
}
