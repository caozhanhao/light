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
#ifndef LIGHT_AUDIO_HPP
#define LIGHT_AUDIO_HPP
#include "logger.hpp"
#include <pulse/simple.h>
#include <pulse/error.h>
namespace light::audio
{
  class Audio
  {
  private:
    bool inited;
    std::string server;
    pa_simple *s;
    pa_sample_spec ss;
    unsigned int rate;
  public:
    Audio() : inited(false), rate(0) {};
  
    Audio(const Audio &b) = delete;
  
    ~Audio()
    {
      if (s)
      {
        pa_simple_drain(s, NULL);
        pa_simple_free(s);
      }
    }
  
    void set_samplerate(unsigned int rate_)
    {
      if (rate != rate_ || !inited)
      {
        rate = rate_;
        init({.format = PA_SAMPLE_S16LE,
                 .rate = rate,
                 .channels = 2});
      }
    }
    
    void set_server(const std::string &server_) { server = server_; }
    
    void init(pa_sample_spec ss = {.format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2})
    {
      rate = ss.rate;
      if (server == "")
      {
        auto server_env = getenv("PULSE_SERVER");
        if (server_env)
        {
          server = server_env;
        }
        else
        {
          throw logger::Error(LIGHT_ERROR_LOCATION, __func__, "Can not find PULSE_SERVER");
        }
      }
      LIGHT_NOTICE("Initing PulseAudio [with server = '" + server + "'].");
      if (s)
      {
        pa_simple_free(s);
      }
      int err;
      s = pa_simple_new(server.c_str(),
                        "pulseaudio", PA_STREAM_PLAYBACK,
                        NULL, "playback", &ss, NULL, NULL, &err);
      if (!s)
      {
        throw logger::Error(LIGHT_ERROR_LOCATION, __func__,
                            "Error initing PulseAudio: "
                            + std::string(pa_strerror(err)) + "\n");
      }
      inited = true;
      LIGHT_NOTICE("Connected Successfully.");
    }
  
    void write(const void *data, std::size_t bytes)
    {
      if (!inited)
      {
        default_init();
      }
      int err = 0;
      if (pa_simple_write(s, data, bytes, &err) < 0)
      {
        throw logger::Error(LIGHT_ERROR_LOCATION, __func__,
                            "Error writing PulseAudio: "
                            + std::string(pa_strerror(err)) + "\n");
      }
    }
  
  private:
    void default_init()
    {
      auto s = getenv("PULSE_SERVER");
      if (s)
      {
        server = s;
        init();
      }
      else
      {
        throw logger::Error(LIGHT_ERROR_LOCATION, __func__, "Can not find PULSE_SERVER");
      }
    }
  };
}
#endif