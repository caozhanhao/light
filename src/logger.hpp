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
#ifndef LIGHT_LOGGER_HPP
#define LIGHT_LOGGER_HPP

#include "utils.hpp"
#include "term.hpp"
#include <stdexcept>
#include <string>
#include <chrono>

#define LIGHT_STRINGFY(x) _LIGHT_STRINGFY(x)
#define _LIGHT_STRINGFY(x) #x
#define LIGHT_ERROR_LOCATION  __FILE__ ":" LIGHT_STRINGFY(__LINE__)
#define LIGHT_NOTICE(msg) light::logger::logger_output(std::string("N: ") + msg);
namespace light::logger
{
  class Error : public std::logic_error
  {
  public:
    Error(std::string location, std::string func_name, std::string details)
        : logic_error(utils::colorify("Error", utils::Color::RED) +
                      "In File: " + location + ":" + func_name + "(): \n" + details) {}
  };
  
  void logger_output(const std::string &str)
  {
    term::TermPos pos(0, term::get_height() - 1);
    int a = term::get_width() - str.size();
    if (a > 0)
    {
      term::mvoutput(pos, str + std::string(a, ' '));
    }
    else
    {
      term::mvoutput(pos, str);
    }
  }
  
  std::string get_time()
  {
    auto tt = std::chrono::system_clock::to_time_t
        (std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int) ptm->tm_year + 1900, (int) ptm->tm_mon + 1, (int) ptm->tm_mday,
            (int) ptm->tm_hour, (int) ptm->tm_min, (int) ptm->tm_sec);
    return {date};
  }
}
#endif