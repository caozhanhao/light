//   Copyright 2022 light - caozhanhao
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
#ifndef LIGHT_ERROR_HPP
#define LIGHT_ERROR_HPP
#include <stdexcept>
#include <string>

#define LIGHT_STRINGFY(x) _LIGHT_STRINGFY(x)
#define _LIGHT_STRINGFY(x) #x
#define LIGHT_ERROR_LOCATION  __FILE__ ":" LIGHT_STRINGFY(__LINE__) 
namespace light::error
{
  class Error : public std::logic_error
  {
  public:
    Error(std::string location, std::string func_name, std::string details)
        : logic_error("In File: " + location + ":" + func_name + "(): \n" + details) {}
  };
}
#endif