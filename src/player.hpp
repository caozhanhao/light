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

#include "http.hpp"
#include "tagreader.hpp"
#include "stream.hpp"
#include "decoder.hpp"
#include "utils.hpp"
#include "term.hpp"
#include <memory>
#include <chrono>
#include <deque>
#include <chrono>
#include <algorithm>
#include <string>
#include <fstream>
#include <random>
#include <filesystem>

namespace light::player
{
  class Player
  {
  private:
    class Music
    {
    private:
      std::string music_name;
      std::function<std::shared_ptr<stream::InputStream>()> get_music_file;
    public:
      Music(const std::string &name_,
            const std::function<std::shared_ptr<stream::InputStream>()> &file_)
          : music_name(name_), get_music_file(file_) {}
  
      std::string name() const { return music_name; }
  
      std::shared_ptr<stream::InputStream> get_file()
      {
        return get_music_file();
      }
    };

  private:
    decoder::Decoder decoder;
    std::shared_ptr<encoder::EncodeStream> encode;
    std::deque<Music> music_list;
    std::size_t index;
    std::string cache_path;
    bar::TimeBar timebar;
    bool cache;
  public:
    Player() : timebar({0, 0}), index(0), cache(false),
               encode(std::make_shared<encoder::AudioEncodeStream>()) {}
  
    Player &set_audio_server(const std::string &server)
    {
      auto ptr = std::make_shared<stream::AudioOutputStream>();
      ptr->set_audio_server(server);
      std::dynamic_pointer_cast<encoder::AudioEncodeStream>(encode)->set_out(ptr);
      return *this;
    }
  
    Player &output_to_file(std::string name)
    {
      encode = std::make_shared<encoder::WavEncodeStream>(std::move(name));
      return *this;
    }
  
    Player &enable_cache(const std::string &cachepath = "cache/")
    {
      std::filesystem::path p(cachepath);
      if (!std::filesystem::exists(p))
      {
        std::filesystem::create_directory(p);
      }
      cache_path = cachepath;
      cache = true;
      return *this;
    };
  
    bool is_paused()
    {
      return decoder.is_paused();
    }
  
    Player &pause()
    {
      timebar.pause();
      decoder.pause();
      return *this;
    }
  
    Player &go()
    {
      timebar.go();
      decoder.go();
      return *this;
    }
  
    Player &skip()
    {
      decoder.skip();
      timebar.skip();
      return *this;
    }
  
    Player &rewind()
    {
      decoder.rewind();
      timebar.rewind();
      return *this;
    }
  
    Player &seek()
    {
      timebar.go();
      decoder.go();
      return *this;
    }
  
    Player &push_online(const std::string &url,
                        const std::string &music_name = "online music",
                        const std::string &file_name =
                        std::to_string(std::chrono::system_clock::now()
                                           .time_since_epoch().count()))
    {
      std::function<std::shared_ptr<stream::InputStream>()> func;
      if (cache)
      {
        std::string filename = cache_path + "/"
                               + file_name;
        func = [this, url, filename]() -> std::shared_ptr<stream::InputStream>
        {
          http::Http res(url);
          res.set_file(filename).get();
          auto t = res.response.file();
          if (res.response_code != 200 || !t->is_open())
          {
            throw logger::Error(LIGHT_ERROR_LOCATION, __func__,
                                "Download music failed");
          }
          t->clear();
          t->seekg(std::ios_base::beg);
          return std::make_shared<stream::FileInputStream>(t);
        };
      }
      else
      {
        func = [this, url]() -> std::shared_ptr<stream::InputStream>
        {
          std::mutex mtx;
          std::shared_ptr<stream::InputStream> buf;
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
      auto func = [filename, this]() -> std::shared_ptr<stream::InputStream>
      {
        if (!std::filesystem::exists(filename))
        {
          throw logger::Error(LIGHT_ERROR_LOCATION, __func__,
                              "No such file '" + filename + "'.");
        }
        auto f = std::make_shared<std::fstream>(std::fstream(filename,
                                                             std::ios_base::in | std::ios_base::binary));
        if (!f->is_open())
        {
          throw logger::Error(LIGHT_ERROR_LOCATION, __func__, "Open file failed.");
        }
        return std::make_shared<stream::FileInputStream>(f);
      };
      music_list.emplace_back(Music((music_name != "" ? music_name : filename), func));
      return *this;
    }
  
    Player &output(int num = -1)
    {
      if (num == -1) num = music_list.size();
      for (auto i = 0; i < num; i++)
      {
        check_list();
        if (encode->get_output()->get_mode() == stream::OutputMode::audio)
        {
          term::clear();
          std::size_t ypos = 0;
          term::mv_xcenter_output(ypos++, "light - A simple music player by caozhanhao");
          ypos++;
          term::mvoutput({0, ypos++}, "Music List: ");
          for (std::size_t i = index; i < music_list.size(); ++i)
          {
            if (ypos > term::get_height() - 5) break;
            if (i == index)
            {
              term::mvoutput({0, ypos++}, std::to_string(i + 1) + "| "
                                          + utils::colorify(music_list[i].name(), utils::Color::LIGHT_BLUE) +
                                          " (playing)");
            }
            else
            {
              term::mvoutput({0, ypos++}, std::to_string(i + 1) + "| " + music_list[i].name());
            }
          }
          auto file = music_list[index].get_file();
          term::mvoutput({0, term::get_height() - 4}, "Playing: ");
          auto name = music_list[index].name();
          term::mvoutput({0, term::get_height() - 3}, tagreader::TagInfo(file).common_info());
          term::mvoutput({0, term::get_height() - 2}, name);
          timebar.set_pos({name.size() + 1, term::get_height() - 2});
          play(file);
        }
        else
        {
          decoder.decode(music_list[index].get_file(), encode, nullptr);
        }
        index++;
        if (!light_is_running) return *this;
      }
      return *this;
    }
  
    Player &shuffle()
    {
      std::mt19937 gen(std::random_device{}());
      std::shuffle(music_list.begin(), music_list.end(), gen);
      return *this;
    }

  private:
    void play(const std::shared_ptr<stream::InputStream> &in)
    {
      std::shared_ptr<std::promise<utils::MusicInfo>> info{std::make_shared<std::promise<utils::MusicInfo>>()};
      timebar.set_info(info);
      timebar.start();
      decoder.decode(in, encode, info);
      timebar.drain();
      timebar.reset();
    }
  
    void check_list()
    {
      if (index >= music_list.size())
      {
        throw logger::Error(LIGHT_ERROR_LOCATION, __func__, "music list has no music.");
      }
    }
  };
}
#endif