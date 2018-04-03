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
#include <i2cnet/core/I2cContext.h>
#include <i2cnet/core/I2cEngineImpl.h>

namespace i2cnet {
namespace core {

std::unique_ptr<I2cEngine> I2cContext::engine_(new I2cEngineImpl());

I2cContext::I2cContext() {
}

I2cContext::~I2cContext() {
}

I2cEngine& I2cContext::getEngine() {
	return *engine_.get();
}

}
}
