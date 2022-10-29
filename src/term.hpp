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
#ifndef LIGHT_TERM_HPP
#define LIGHT_TERM_HPP

#include <iostream>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

#include <mutex>
namespace light::term
{
  class TermPos
  {
  private:
    std::size_t x;
    std::size_t y;
  public:
    TermPos(std::size_t x_, std::size_t y_)
        : x(x_), y(y_) {}
    
    [[nodiscard]]std::size_t get_x() const { return x; }
    
    [[nodiscard]]std::size_t get_y() const { return y; }
  };
  
  class KeyBoard
  {
  public:
    struct termios initial_settings, new_settings;
    int peek_character;
    
    KeyBoard()
    {
      tcgetattr(0, &initial_settings);
      new_settings = initial_settings;
      tcgetattr(0, &new_settings);
      new_settings.c_lflag &= (~ICANON & ~ECHO);
      tcsetattr(0, TCSANOW, &new_settings);
      setbuf(stdin, NULL);
    }
    
    ~KeyBoard()
    {
      tcsetattr(0, TCSANOW, &initial_settings);
    }
    
    int kbhit()
    {
      unsigned char ch;
      int nread;
      if (peek_character != -1) return 1;
      new_settings.c_cc[VMIN] = 0;
      tcsetattr(0, TCSANOW, &new_settings);
      nread = read(0, &ch, 1);
      new_settings.c_cc[VMIN] = 1;
      tcsetattr(0, TCSANOW, &new_settings);
  
      if (nread == 1)
      {
        peek_character = ch;
        return 1;
      }
      return 0;
    }
    
    int getch()
    {
      return getchar();
    }
  };
  
  KeyBoard keyboard;
  std::mutex output_mutex;
  
  int getch()
  {
    return keyboard.getch();
  }
  
  bool kbhit()
  {
    return keyboard.kbhit();
  }
  
  std::size_t get_height()
  {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_row;
  }
  
  std::size_t get_width()
  {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
  }

  void mvoutput(const TermPos &pos, const std::string &str)
  {
    std::lock_guard<std::mutex> lck(output_mutex);
    printf("%c[%d;%df", 0x1b, (int) pos.get_y() + 1, (int) pos.get_x() + 1);
    std::cout << str << std::flush;
  }
  
  void mv_xcenter_output(std::size_t y, const std::string &str)
  {
    TermPos pos{get_width() / 2 - str.size() / 2, y};
    mvoutput(pos, str);
  }
  
  void clear()
  {
    printf("\033[2J");
  }
}
#endif