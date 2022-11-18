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
#ifndef LIGHT_BAR_HPP
#define LIGHT_BAR_HPP

#include "utils.hpp"
#include "term.hpp"
#include <string>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <future>

namespace light::bar
{
  class Bar
  {
  protected:
    char padding;
    char progress;
    std::size_t size;
    std::size_t finished_size;
    std::size_t last_addition_size;
    term::TermPos pos;
    bool first;
  public:
    Bar(term::TermPos pos_ = {0, 0}, std::size_t size_ = 36, char progress_ = '>', char padding_ = ' ')
        : padding(padding_), progress(progress_), size(size_), finished_size(0),
          last_addition_size(0), first(true), pos(pos_) {}
    
    Bar &set(std::size_t size_, char progress_, char padding_)
    {
      size = size_;
      progress = progress_;
      padding = padding_;
      return *this;
    }
    
    Bar &update(double achieved_ratio, const std::string addition = "")
    {
      if (first)
      {
        term::mvoutput(pos, "[" + std::string(size, padding) + "]" + add(addition));
        first = false;
      }
  
      std::string output;
      if (finished_size < size)
      {
        finished_size = size * achieved_ratio;
        output += "[";
        output += std::string(finished_size, progress);
        output += std::string(size - finished_size, padding);
        output += "]";
      }
      output += add(addition) + std::string(last_addition_size - addition.size() + 1, ' ');
      term::mvoutput(pos, output);
      return *this;
    }
  
  private:
    std::string add(const std::string &addition)
    {
      std::string opt;
      if (addition.size() < last_addition_size)
        opt += std::string(last_addition_size - addition.size(), ' ');
      last_addition_size = addition.size();
      return addition + opt;
    };
  };
  
  std::string ms_to_string(const unsigned int time)
  {
    int minutes = time / 60000;
    int seconds = (time % 60000) / 1000;
    std::string m = minutes > 9 ? std::to_string(minutes) : "0" + std::to_string(minutes);
    std::string s = seconds > 9 ? std::to_string(seconds) : "0" + std::to_string(seconds);
    return m + ":" + s;
  };
  
  class TimeBar : private Bar
  {
  private:
    std::shared_ptr<std::promise<utils::MusicInfo>> info;
    unsigned int time;
    std::thread th;
    std::atomic<bool> paused;
    std::atomic<double> offset;
  public:
    TimeBar(term::TermPos pos_) : Bar(pos_), time(0), paused(false), offset(0) {}
    
    ~TimeBar() { drain(); }
    
    TimeBar &set(std::size_t size_, char progress_, char padding_)
    {
      set(size_, progress_, padding_);
      return *this;
    }
    
    TimeBar &set_pos(term::TermPos pos_)
    {
      pos = pos_;
      return *this;
    }
    
    TimeBar &set_time(unsigned int time_)
    {
      time = time_;
      return *this;
    }
    
    TimeBar &reset()
    {
      finished_size = 0;
      last_addition_size = 0;
      first = true;
      return *this;
    }
    
    TimeBar &set_info(std::shared_ptr<std::promise<utils::MusicInfo>> info_)
    {
      info = info_;
      return *this;
    }
    
    TimeBar &start(unsigned int pos = 0)
    {
      th = std::thread
          ([this, pos]
           {
             if (info != nullptr)
             {
               time = info->get_future().get().time;
               info = nullptr;
             }
  
             auto begin = std::chrono::steady_clock::now() - std::chrono::milliseconds(pos);
             auto get_time = [&begin, this]() -> double
             {
               std::chrono::duration<double, std::milli> s = std::chrono::steady_clock::now() - begin;
               if (offset >= s.count())
               {
                 offset = s.count();
                 return 0;
               }
               return s.count() - offset;
             };
             auto timestr = ms_to_string(time);
             double ltime = get_time();
             while (ltime < time)
             {
               if (paused)
               {
                 auto b = std::chrono::steady_clock::now();
                 while (paused)
                 {
                   std::this_thread::yield();
                 }
                 std::chrono::duration<double, std::milli> s = std::chrono::steady_clock::now() - b;
                 offset = offset + s.count();
               }
               update(get_time() / time,
                      (" " + ms_to_string(get_time()) + "/" + timestr));
               if (get_time() >= time) return;
               std::this_thread::sleep_for(std::chrono::milliseconds(time / 500));
  
               if (!light_is_running) return;
             }
           });
      return *this;
    }
    
    TimeBar &pause()
    {
      paused = true;
      return *this;
    }
    
    TimeBar &go()
    {
      paused = false;
      return *this;
    }
  
    TimeBar &drain()
    {
      if (th.joinable())
      {
        th.join();
      }
      return *this;
    }
  
    TimeBar &rewind()
    {
      offset = offset + 5000;
      return *this;
    }
  
    TimeBar &skip()
    {
      offset = offset - 5000;
      return *this;
    }
  };
}
#endif