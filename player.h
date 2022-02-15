#pragma once
#include "audio.h"
#include "http.h"
#include "music.h"
#include <memory>
#include <chrono>
#include <deque>
#include <random>
#include <string>
#include <fstream>
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
      Music(const std::string& name_,
        const std::function<std::shared_ptr<music::Music>()>& file_)
        : music_name(name_), get_music_file(file_){ }
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
    Player() : index(0), bar(true), cache(false) {  }
    Player& set_audio_server(const std::string& server)
    {
      audio.init(server);
      return *this;
    }
    Player& enable_cache(const std::string& cachepath = "cache/")
    {
      cache_path = cachepath;
      cache = true;
      return *this;
    };
    Player& no_bar()
    {
      bar = false;
      audio.no_bar();
      return *this;
    }
    Player& with_bar()
    {
      bar = true;
      audio.with_bar();
      return *this;
    }
    Player& push_online(const http::Url& url)
    {
      std::function<std::shared_ptr<music::Music>()> func;
      if (cache)
      {
        std::string filename = cache_path + "/"
          + std::to_string(std::chrono::system_clock::now()
            .time_since_epoch().count());
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
          cond.wait(lock, [&] {return buf != nullptr; });
          return buf;
        };
      }
      music_list.emplace_back(Music(url.get(), func));
      return *this;
    }
    Player& push_local(const std::string& filename)
    {
      auto func = [filename, this]() -> std::shared_ptr<music::Music>
      {
        auto f = std::make_shared<std::fstream>(std::fstream(filename,
          std::ios_base::in | std::ios_base::binary));
        if(!f->is_open())
          throw error::Error(LIGHT_ERROR_LOCATION, __func__, "Open file failed.");
        return std::make_shared<music::File>(music::File(f));
      };
      music_list.emplace_back(Music(filename, func));
      return *this;
    }
    Player& pop()
    {
      music_list.pop_front();
      return *this;
    }
    Player& ordered_play(const int num = 1)
    {
      for (auto i = 0; i < num; i++)
      {
        check_list();
        if (bar) std::cout << music_list[index].name();
        audio.play(music_list[index].get_file());
        next_line();
        index++;
      }
      return *this;
    }
    Player& random_play(const int num = 1)
    {
      for (auto i = 0; i < num; i++)
      {
        check_list();
        auto& music = music_list[randint(0, music_list.size() - 1)];
        if (bar) std::cout << music.name();
        audio.play(music.get_file()); 
        next_line();
      }
      return *this;
    }
    Player& repeated_play(const int num = -1)
    {
      check_list();
      if (num == -1)
      {
        while (true)
        {
          if (bar) std::cout << music_list[index].name();
          audio.play(music_list[index].get_file());
          next_line();
        }
        index++;
      }
      else
      {
        for (auto i = 0; i < num; i++)
        {
          if (bar) std::cout << music_list[index].name();
          audio.play(music_list[index].get_file());
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