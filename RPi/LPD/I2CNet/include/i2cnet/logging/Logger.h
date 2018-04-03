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
#ifndef I2CNET_LOGGING_LOGGER_H_
#define I2CNET_LOGGING_LOGGER_H_

#include <string>

namespace i2cnet {
namespace logging {

class Logger {
public:
	virtual ~Logger() {};

	virtual void info(std::string const& location, std::string const& format, ...) throw() = 0;
	virtual void warn(std::string const& location, std::string const& format, ...) throw() = 0;
	virtual void debug(std::string const& location, std::string const& format, ...) throw() = 0;
	virtual void error(std::string const& location, std::string const& format, ...) throw() = 0;
};

}
}

#endif
