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
#include <i2cnet/logging/Utils.h>

#include <cstdio>
#include <vector>
#include <cstdarg>

namespace i2cnet {
namespace logging {

std::string Utils::format(std::string const& format, ...) {
	va_list args;
	va_start(args, format);
	return Utils::vformat(format, args);
	va_end(args);
}

std::string Utils::vformat(std::string const& format, va_list args) {
	int size = std::vsnprintf(0, 0, format.c_str(), args);
	std::vector<char> buffer(size + 1);
	std::vsnprintf(&buffer[0], size + 1, format.c_str(), args);
	return std::string(buffer.begin(), buffer.end());
}

}
}
