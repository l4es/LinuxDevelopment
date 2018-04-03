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
#ifndef I2CNET_APP_PACKETSUBSTITUTOR_H_
#define I2CNET_APP_PACKETSUBSTITUTOR_H_
#include <deque>
#include <vector>

namespace i2cnet {
namespace app {

class PacketSubstitutor {
public:
	static void packetToStream(std::deque<char>& packet);
	static void streamToPacket(std::deque<char>& packet);
	static void substitute(const std::vector<char>& target, const std::vector<char>& value, std::deque<char>& packet);

	virtual ~PacketSubstitutor();

private:
	PacketSubstitutor();
};

}
}

#endif
