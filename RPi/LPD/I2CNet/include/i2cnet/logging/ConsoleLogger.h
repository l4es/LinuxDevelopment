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
#ifndef I2CNET_LOGGING_CONSOLELOGGER_H_
#define I2CNET_LOGGING_CONSOLELOGGER_H_

#include <string>
#include <i2cnet/logging/Logger.h>
#include <mutex>

namespace i2cnet {
namespace logging {

class ConsoleLogger : public Logger {
public:
	ConsoleLogger(std::string const& context, unsigned int& logLevel);
	virtual ~ConsoleLogger();

	virtual void info(std::string const& location, std::string const& format, ...) throw();
	virtual void warn(std::string const& location, std::string const& format, ...) throw();
	virtual void debug(std::string const& location, std::string const& format, ...) throw();
	virtual void error(std::string const& location, std::string const& format, ...) throw();

private:
	std::string context_;
	unsigned int& logLevel_;
	std::string createTimestamp() const;
	std::string generateMessage(std::string const& level, std::string const& location, std::string const& format, va_list args) const;
};

}
}

#endif
