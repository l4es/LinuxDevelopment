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
#include <i2cnet/opt/OptionParser.h>
#include <regex>
#include <exception>

namespace i2cnet {
namespace opt {

OptionParser::OptionParser() : defaultBusMappings_(true), host_("0.0.0.0"), service_("7337"), logLevel_(3), busMappings_() {
	busMappings_[1] = "/dev/i2c-1";
}

OptionParser::~OptionParser() {
}

void OptionParser::processOption(std::string const& option) {
	std::regex hostRegex("--host=(\\S+)");
	std::regex serviceRegex("--service=(\\S+)");
	std::regex logLevel("--loglevel=(\\d+)");
	std::regex busMappingRegex("--bus=(\\d+),(\\S+)");

	std::smatch sm;
	if (std::regex_match(option, sm, hostRegex)) {
		host_ = sm[1];
	} else if (std::regex_match(option, sm, serviceRegex)) {
		service_ = sm[1];
	} else if (std::regex_match(option, sm, logLevel)) {
		logLevel_ = std::stoul(sm[1]);
	} else if (std::regex_match(option, sm, busMappingRegex)) {
		if (defaultBusMappings_) {
			defaultBusMappings_ = false;
			busMappings_.clear();
		}
		busMappings_[std::stoul(sm[1])] = sm[2];
	} else {
		throw std::runtime_error(std::string("Unknown option: ") + option);
	}
}

std::string const& OptionParser::getHost() const {
	return host_;
}

std::string const& OptionParser::getService() const {
	return service_;
}

unsigned int OptionParser::getLogLevel() const {
	return logLevel_;
}

std::map<unsigned int, std::string> const& OptionParser::getBusMappings() const {
	return busMappings_;
}

}
}
