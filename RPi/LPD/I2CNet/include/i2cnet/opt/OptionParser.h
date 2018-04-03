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
#ifndef I2CNET_OPT_OPTIONPARSER_H_
#define I2CNET_OPT_OPTIONPARSER_H_
#include <map>
#include <string>

namespace i2cnet {
namespace opt {

class OptionParser {
public:
	OptionParser();
	virtual ~OptionParser();

	void processOption(std::string const& option);

	std::string const& getHost() const;
	std::string const& getService() const;
	unsigned int getLogLevel() const;
	std::map<unsigned int, std::string> const& getBusMappings() const;

private:
	bool defaultBusMappings_;

	std::string host_;
	std::string service_;
	unsigned int logLevel_;
	std::map<unsigned int, std::string> busMappings_;
};

}
}

#endif
