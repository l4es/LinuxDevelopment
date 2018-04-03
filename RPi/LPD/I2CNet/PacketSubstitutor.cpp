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
#include <i2cnet/app/PacketSubstitutor.h>
#include <deque>
#include <iomanip>
#include <vector>
#include <sstream>
#include <algorithm>

namespace i2cnet {
namespace app {

const std::vector<char> startFrame = {'a'};
const std::vector<char> endFrame = {'b'};
const std::vector<char> special = {'z'};
const std::vector<char> specialStartFrame = {'z', 'w'};
const std::vector<char> specialEndFrame = {'z', 'x'};
const std::vector<char> specialSpecial = {'z', 'y'};

void PacketSubstitutor::streamToPacket(std::deque<char>& packet) {
	PacketSubstitutor::substitute(specialStartFrame, startFrame, packet);
	PacketSubstitutor::substitute(specialEndFrame, endFrame, packet);
	PacketSubstitutor::substitute(specialSpecial, special, packet);
}

void PacketSubstitutor::packetToStream(std::deque<char>& packet) {
	PacketSubstitutor::substitute(special, specialSpecial, packet);
	PacketSubstitutor::substitute(startFrame, specialStartFrame, packet);
	PacketSubstitutor::substitute(endFrame, specialEndFrame, packet);
}

void PacketSubstitutor::substitute(const std::vector<char>& target, const std::vector<char>& value, std::deque<char>& packet) {
	auto it = std::search(packet.begin(), packet.end(), target.begin(), target.end());
	while (it != packet.end()) {
		it = packet.erase(it, std::next(it, target.size()));
		it = packet.insert(it, value.begin(), value.end());
		it = std::next(it, value.size());
		it = std::search(it, packet.end(), target.begin(), target.end());
	}
}

}
}

