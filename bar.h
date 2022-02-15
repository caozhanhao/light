#pragma once
#include <string>
#include <iostream>
#include <thread>
namespace light::bar
{
  class Bar
  {
  private:
    char padding;
    char progress;
    std::size_t size;
    std::size_t finished_size;
    std::size_t last_addition_size;
    bool first;
  public:
    Bar(std::size_t size_ = 36, char progress_ = '>', char padding_ = ' ')
      : padding(padding_), progress(progress_), size(size_), finished_size(0),
      last_addition_size(0), first(true) {}
    Bar& set(std::size_t size_, char progress_, char padding_)
    {
      size = size_;
      progress = progress_;
      padding = padding_;
      return *this;
    }
    Bar& update(double achieved_ratio, const std::string addition = "")
    {
      if (first)
      {
        std::cout << "[" << std::string(size, padding) << "]"
          << add(addition) << std::flush;
        first = false;
      }
      if (finished_size < size && size * achieved_ratio - finished_size > 0.5)
      {
        finished_size++;
        std::string d(last_addition_size + size + 2, '\b');
        std::string output = "[";
        output += std::string(finished_size, progress);
        output += std::string(size - finished_size, padding);
        output += "]";
        output += add(addition);
        std::cout << d << output << std::flush;
      }
      else
      {
        std::string d(last_addition_size, '\b');
        auto p = add(addition);
        std::cout << d << p << std::flush;
      }
      return *this;
    }
  private:
    std::string add(const std::string& addition)
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
    unsigned int time;
    std::thread th;
  public:
    TimeBar(unsigned int time_) :time(time_) {  }
    TimeBar() :time(0) {  }
    TimeBar& set_time(unsigned int time_) { time = time_; return *this; }
    TimeBar& set(std::size_t size_, char progress_, char padding_)
    {
      set(size_, progress_, padding_);
      return *this;
    }
    TimeBar& start()
    {
      th = std::thread
      ([this]
        {
          auto begin = std::chrono::steady_clock::now();
          auto get_time = [&begin]() -> double
          {
            std::chrono::duration<double, std::milli> s = std::chrono::steady_clock::now() - begin;
            return s.count();
          };
          auto timestr = ms_to_string(time);
          while (get_time() < time)
          {
            update(get_time() / time,
              (" " + ms_to_string(get_time()) + "/" + timestr));
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
          }
        });
      th.detach();
      return *this;
    }
    TimeBar& drain()
    {
      if (th.joinable())
        th.join();
      return *this;
    }
  };
}