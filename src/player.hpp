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
#ifndef LIGHT_PLAYER_HPP
#define LIGHT_PLAYER_HPP
#include "audio.hpp"
#include "http.hpp"
#include "music.hpp"
#include <memory>
#include <chrono>
#include <deque>
#include <random>
#include <string>
#include <fstream>
#include <filesystem>
namespace light::player
{
  int randint(int a, int b)
  {
    unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
    std::default_random_engine e(seed);
    std::uniform_int_distribution<unsigned> u(a, b);
    return u(e);
  }
  
  class Player
  {
  private:
    class Music
    {
    private:
      std::string music_name;
      std::function<std::shared_ptr<music::Music>()> get_music_file;
    public:
      Music(const std::string &name_,
            const std::function<std::shared_ptr<music::Music>()> &file_)
          : music_name(name_), get_music_file(file_) {}
      
      std::string name() const { return music_name; }
      
      std::shared_ptr<music::Music> get_file()
      {
        return get_music_file();
      }
    };
  
  private:
    audio::Audio audio;
    std::deque<Music> music_list;
    std::size_t index;
    std::string cache_path;
    bool bar;
    bool cache;
  public:
    Player() : index(0), bar(true), cache(false) {}
    
    bool is_paused() const
    {
      return audio.is_paused();
    }
    
    Player &pause()
    {
      audio.pause();
      return *this;
    }
    
    Player &go()
    {
      audio.go();
      return *this;
    }
    
    auto curr()
    {
      return audio.curr();
    }
    
    Player &set_audio_server(const std::string &server)
    {
      audio.init(server);
      return *this;
    }
    
    Player &enable_cache(const std::string &cachepath = "cache/")
    {
      std::filesystem::path p(cachepath);
      if (!(std::filesystem::exists(p) && std::filesystem::is_directory(p)))
      {
        throw error::Error(LIGHT_ERROR_LOCATION, __func__,
                           "No such cache directory '" + cachepath + "'.");
      }
      cache_path = cachepath;
      cache = true;
      return *this;
    };
    
    Player &no_bar()
    {
      bar = false;
      audio.no_bar();
      return *this;
    }
    
    Player &with_bar()
    {
      bar = true;
      audio.with_bar();
      return *this;
    }
    
    Player &push_online(const http::Url &url,
                        const std::string &music_name = "",
                        const std::string &file_name =
                        std::to_string(std::chrono::system_clock::now()
                                           .time_since_epoch().count()))
    {
      std::function<std::shared_ptr<music::Music>()> func;
      if (cache)
      {
        std::string filename = cache_path + "/"
                               + file_name;
        func = [this, url, filename]() -> std::shared_ptr<music::Music>
        {
          http::Http res(url);
          res.set_file(filename).get();
          auto t = res.response.file();
          if (res.response_code != 200 || !t->is_open())
            throw error::Error(LIGHT_ERROR_LOCATION, __func__,
                               "Download music failed");
          t->clear();
          t->seekg(std::ios_base::beg);
          return std::make_shared<music::File>(music::File(t));
        };
      }
      else
      {
        func = [this, url]() -> std::shared_ptr<music::Music>
        {
          std::mutex mtx;
          std::shared_ptr<music::Buffer> buf;
          std::condition_variable cond;
          std::thread th(
              [&]()
              {
                http::Http res(url);
                res.set_buffer();
                mtx.lock();
                buf = res.response.buffer();
                mtx.unlock();
                cond.notify_all();
                res.get();
              });
          th.detach();
          std::unique_lock<std::mutex> lock(mtx);
          cond.wait(lock, [&] { return buf != nullptr; });
          return buf;
        };
      }
      music_list.emplace_back(Music(music_name, func));
      return *this;
    }
    
    Player &push_local(const std::string &filename, const std::string &music_name = "")
    {
      auto func = [filename, this]() -> std::shared_ptr<music::Music>
      {
        if (!std::filesystem::exists(filename))
        {
          throw error::Error(LIGHT_ERROR_LOCATION, __func__,
                             "No such file '" + filename + "'.");
        }
        auto f = std::make_shared<std::fstream>(std::fstream(filename,
                                                             std::ios_base::in | std::ios_base::binary));
        if (!f->is_open())
          throw error::Error(LIGHT_ERROR_LOCATION, __func__, "Open file failed.");
        return std::make_shared<music::File>(music::File(f));
      };
      music_list.emplace_back(Music((music_name != "" ? music_name : filename), func));
      return *this;
    }
    
    Player &pop()
    {
      music_list.pop_front();
      return *this;
    }
    
    Player &ordered_play(int num = -1, int pos = 0)
    {
      if (num == -1) num = music_list.size();
      for (auto i = 0; i < num; i++)
      {
        check_list();
        if (bar) std::cout << music_list[index].name();
        audio.play(music_list[index].get_file(), pos);
        next_line();
        index++;
      }
      return *this;
    }
    
    Player &random_play(int num = -1, int pos = 0)
    {
      if (num == -1) num = music_list.size();
      for (auto i = 0; i < num; i++)
      {
        check_list();
        auto &music = music_list[randint(0, music_list.size() - 1)];
        if (bar) std::cout << music.name();
        audio.play(music.get_file(), pos);
        next_line();
      }
      return *this;
    }
    
    Player &repeated_play(int num = -1, int pos = 0)
    {
      check_list();
      if (num == -1)
      {
        while (true)
        {
          if (bar) std::cout << music_list[index].name();
          audio.play(music_list[index].get_file(), pos);
          next_line();
        }
      }
      else
      {
        for (auto i = 0; i < num; i++)
        {
          if (bar) std::cout << music_list[index].name();
          audio.play(music_list[index].get_file(), pos);
          next_line();
        }
        index++;
      }
      return *this;
    }
  
  private:
    void next_line()
    {
      if (bar)
        std::cout << "\n";
    }
    
    void check_list()
    {
      if (index >= music_list.size())
        throw error::Error(LIGHT_ERROR_LOCATION, __func__, "music list has no music.");
    }
  };
}
#endif