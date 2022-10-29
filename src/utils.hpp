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
#ifndef LIGHT_UTILS_HPP
#define LIGHT_UTILS_HPP

#include "logger.hpp"

namespace light::utils
{
  struct MusicInfo
  {
    unsigned int time;
    unsigned int samplerate;
    unsigned long bitrate;
    unsigned int channels;
    std::size_t size;
  };
  
  enum class Color
  {
    BLUE, LIGHT_BLUE, GREEN, PURPLE, YELLOW, WHITE, RED
  };
  
  std::string colorify(const std::string &str, Color color)
  {
    switch (color)
    {
      case Color::PURPLE:
        return "\033[35m" + str + "\033[0m";
      case Color::LIGHT_BLUE:
        return "\033[36m" + str + "\033[0m";
      case Color::BLUE:
        return "\033[34m" + str + "\033[0m";
      case Color::GREEN:
        return "\033[32m" + str + "\033[0m";
      case Color::YELLOW:
        return "\033[33m" + str + "\033[0m";
      case Color::WHITE:
        return "\033[37m" + str + "\033[0m";
      case Color::RED:
        return "\033[31m" + str + "\033[0m";
      default:
        throw logger::Error(LIGHT_ERROR_LOCATION, __func__, "Unexpected color");
    }
  }
}
#endif