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
#include <i2cnet/app/ApplicationThread.h>
#include <deque>
#include <iomanip>
#include <vector>
#include <sstream>
#include <algorithm>
#include <i2cnet/app/PacketSubstitutor.h>

namespace i2cnet {
namespace app {

const int BUFFER_LIMIT = 0xFFFF;
const std::vector<char> startFrame = {'a'};
const std::vector<char> endFrame = {'b'};
const std::vector<char> special = {'z'};
const std::vector<char> specialStartFrame = {'z', 'w'};
const std::vector<char> specialEndFrame = {'z', 'x'};
const std::vector<char> specialSpecial = {'z', 'y'};

ApplicationThread::ApplicationThread(i2cnet::core::I2cEngine& engine, std::unique_ptr<i2cnet::socket::Socket> socket, std::unique_ptr<i2cnet::logging::Logger> logger) : logger_(std::move(logger)), running_(true), engine_(engine), socket_(std::move(socket)) {
}

ApplicationThread::~ApplicationThread() {
	logger_->debug(I2CNET_AT, "Destroying ApplicationThread");
}

std::string const& ApplicationThread::getRemoteEndpoint() const {
	return socket_->getRemoteEndpoint();
}

void ApplicationThread::run() {
	std::vector<char> inputBuffer;

	while (running_) {
		std::vector<char> receiveBuffer;
		socket_->receive(receiveBuffer);

		if (receiveBuffer.size() == 0) {
			running_ = false;
			continue;
		}

		if (inputBuffer.size() + receiveBuffer.size() > BUFFER_LIMIT) {
			logger_->error(I2CNET_AT, "Receive buffer overflow, clearing buffer.");
			inputBuffer.clear();
		}

		inputBuffer.insert(inputBuffer.end(), receiveBuffer.begin(), receiveBuffer.end());
		receiveBuffer.clear();

		auto it = find(inputBuffer.begin(), inputBuffer.end(), 'a');
		if (it == inputBuffer.end()) {
			inputBuffer.clear();
		} else if (it != inputBuffer.begin()) {
			inputBuffer.erase(inputBuffer.begin(), it);
		}

		it = find(inputBuffer.begin(), inputBuffer.end(), 'b');
		while (it != inputBuffer.end()) {
			std::deque<char> packet(inputBuffer.begin() + 1, it);
			inputBuffer.erase(inputBuffer.begin(), it + 1);

			i2cnet::app::PacketSubstitutor::streamToPacket(packet);

			int length = 0;
			int busNumber = packet.at(0);
			char device = packet.at(1);
			char location = packet.at(2);
			char operation = packet.at(3);
			packet.pop_front();
			packet.pop_front();
			packet.pop_front();
			packet.pop_front();

			if (operation == 0) {
				length = packet.at(0);
				packet.pop_front();
			}

			logger_->debug(I2CNET_AT, "Request received on bus %d, device 0x%02X, location 0x%02X, operation %s", busNumber, device, location, (operation == 0) ? "READ" : "WRITE");

			std::deque<char> transmitBuffer;
			i2cnet::model::I2cAddress address(busNumber, device, location);
			i2cnet::model::I2cRequest request(address, length, std::vector<char>(packet.begin(), packet.end()));
			i2cnet::model::I2cResponse response(address);

			engine_.processRequest(request, response);
			if (operation == 0) {
				transmitBuffer.insert(transmitBuffer.end(), response.getData().begin(), response.getData().end());
				i2cnet::app::PacketSubstitutor::packetToStream(transmitBuffer);

				transmitBuffer.push_front('a');
				transmitBuffer.push_back('b');
				std::vector<char> sendBuffer(transmitBuffer.begin(), transmitBuffer.end());
				socket_->send(sendBuffer);
			}
			transmitBuffer.clear();

			it = find(inputBuffer.begin(), inputBuffer.end(), 'b');
		}
	}
	logger_->debug(I2CNET_AT, "Complete ApplicationThread.");
}

}
}
