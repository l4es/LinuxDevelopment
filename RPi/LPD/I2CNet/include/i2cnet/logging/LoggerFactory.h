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
#ifndef I2CNET_LOGGING_LOGGERFACTORY_H_
#define I2CNET_LOGGING_LOGGERFACTORY_H_
#include <i2cnet/logging/Logger.h>

#include <memory>

#define I2CNET_TO_STRING(x) #x
#define I2CNET_LOCATION(x, y) x ":" I2CNET_TO_STRING(y)
#define I2CNET_LOCATION_LINE(x) I2CNET_TO_STRING(x)

#define I2CNET_AT I2CNET_LOCATION_LINE(__LINE__)
#define I2CNET_FILE I2CNET_LOCATION(__FILE__, __LINE__)

namespace i2cnet {
namespace logging {

class LoggerFactory {
public:
	static std::unique_ptr<Logger> getLogger(std::string const& context);
	static void setLogLevel(unsigned int level);

private:
	static unsigned int logLevel_;

	LoggerFactory();
	LoggerFactory(LoggerFactory const&);
	virtual ~LoggerFactory();

	LoggerFactory const& operator=(LoggerFactory const&);
};

}
}

#endif
