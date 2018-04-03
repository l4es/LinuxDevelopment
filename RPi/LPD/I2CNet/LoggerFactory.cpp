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
#include <i2cnet/logging/ConsoleLogger.h>
#include <i2cnet/logging/LoggerFactory.h>

namespace i2cnet {
namespace logging {

unsigned int LoggerFactory::logLevel_ = 3;

std::unique_ptr<Logger> LoggerFactory::getLogger(std::string const& context) {
	return std::unique_ptr<Logger>(new ConsoleLogger(context, LoggerFactory::logLevel_));
}

void LoggerFactory::setLogLevel(unsigned int level) {
	if (level > 4) {
		level = 4;
	}
	LoggerFactory::logLevel_ = level;
}

}
}
