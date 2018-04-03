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
#include <i2cnet/app/ApplicationConnectionListener.h>
#include <i2cnet/core/I2cBusEngineImpl.h>
#include <i2cnet/core/I2cContext.h>
#include <i2cnet/core/I2cEngine.h>
#include <i2cnet/model/I2cAddress.h>
#include <i2cnet/model/I2cRequest.h>
#include <i2cnet/model/I2cResponse.h>
#include <i2cnet/opt/OptionParser.h>
#include <i2cnet/socket/ServerSocket.h>
#include <i2cnet/socket/TcpServerSocket.h>
#include <i2cnet/logging/LoggerFactory.h>
#include <exception>
#include <iostream>

const std::string USAGE = ""
"Usage: i2cnet [OPTION...]\n"
"i2c network adaptor.\n"
"\n"
"Examples:\n"
"  i2cnet                     # Listen on 0.0.0.0:7337, bus 1 = /dev/i2c-1\n"
"                             # log level 3 (INFO)\n"
"  i2cnet --host=::1          # Listen on the IPv6 loopback.\n"
"\n"
" Options:\n"
"\n"
"  --host=HOST (0.0.0.0)      Specified the address to listen on.\n"
"  --service=SERVICE (7337)   Specifies the service to bind to.\n"
"  --loglevel=LEVEL (3)       0= OFF, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG\n"
"  --bus=ID,PATH              The id-bus binding.  Can specify more than 1.\n"
"                             Default: 1,i2c-1, if this option is specified\n"
"                             the default binding will be removed.";

int main(int argc, char** argv) {
	i2cnet::opt::OptionParser optionsParser;

	try {
		for (int i = 1; i < argc; ++i) {
			optionsParser.processOption(argv[i]);
		}
	} catch (std::exception& e) {
		std::cout << e.what() << std::endl;
		std::cout << USAGE << std::endl;
		return 1;
	}
	i2cnet::logging::LoggerFactory::setLogLevel(optionsParser.getLogLevel());

	i2cnet::core::I2cEngine& engine = i2cnet::core::I2cContext::getEngine();
	auto busMappings = optionsParser.getBusMappings();
	for (auto it = busMappings.begin(); it != busMappings.end(); it++) {
		engine.registerBus(it->first, it->second);
	}

	i2cnet::socket::TcpServerSocket serverSocket(optionsParser.getHost(), optionsParser.getService());
	i2cnet::app::ApplicationConnectionListener connectionListener;
	serverSocket.setConnectionListener(connectionListener);

	std::mutex mainMutex;
	std::condition_variable mainWait;
	std::unique_lock<std::mutex> mainLock(mainMutex);
	mainWait.wait(mainLock);
}
