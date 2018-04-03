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
#include <ctime>
#include <chrono>
#include <thread>
#include <iomanip>
#include <iostream>

#include <i2cnet/logging/ConsoleLogger.h>
#include <i2cnet/logging/Utils.h>
#include <cstdarg>

namespace i2cnet {
namespace logging {

ConsoleLogger::ConsoleLogger(std::string const& context, unsigned int& logLevel) : context_(context), logLevel_(logLevel) {
}

ConsoleLogger::~ConsoleLogger() {
}

void ConsoleLogger::info(std::string const& location, std::string const& format, ...) throw() {
	if (logLevel_ < 3) {
		return;
	}

	va_list args;
	va_start(args, format);
	std::cout << this->generateMessage("INFO", location, format, args) << std::flush;
	va_end(args);
}

void ConsoleLogger::warn(std::string const& location, std::string const& format, ...) throw() {
	if (logLevel_ < 2) {
		return;
	}

	va_list args;
	va_start(args, format);
	std::cout << this->generateMessage("WARN", location, format, args) << std::flush;
	va_end(args);
}

void ConsoleLogger::debug(std::string const& location, std::string const& format, ...) throw() {
	if (logLevel_ < 4) {
		return;
	}

	va_list args;
	va_start(args, format);
	std::cout << this->generateMessage("DEBUG", location, format, args) << std::flush;
	va_end(args);
}

void ConsoleLogger::error(std::string const& location, std::string const& format, ...) throw() {
	if (logLevel_ < 1) {
		return;
	}

	va_list args;
	va_start(args, format);
	std::cout << this->generateMessage("ERROR", location, format, args) << std::flush;
	va_end(args);
}

std::string ConsoleLogger::createTimestamp() const {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

	char timeValue[256];
	std::strftime(timeValue, sizeof(timeValue), "%Y-%m-%d %X", std::gmtime(&in_time_t));

	std::ostringstream os;
//	os << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%d %X") << "." << std::setfill('0') << std::setw(3) << millis;
	os << timeValue << "." << std::setfill('0') << std::setw(3) << millis;
	return os.str();
}

std::string ConsoleLogger::generateMessage(std::string const& level, std::string const& location, std::string const& format, va_list args) const {
	std::ostringstream os;
	os << "[" << this->createTimestamp() << "] " << context_ << "/" << location << " (" << level << "): " << Utils::vformat(format, args) << std::endl;
	return os.str();
}

}
}

