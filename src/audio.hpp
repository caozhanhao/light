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
#include "bar.hpp"
#include "music.hpp"
#include "error.hpp"

#include <pulse/simple.h>
#include <pulse/error.h>

#include <mad.h>
#include <string.h>

#include <array>
#include <vector>
#include <string>
#include <memory>
#include <condition_variable>
#include <mutex>
namespace light::audio
{
  unsigned int size_to_time(std::size_t size, int bitrate)
  {
    return (size / 8192) * ((8192 * 8) / (bitrate / 1000));
  }
  std::size_t time_to_size(unsigned int time, int bitrate)
  {
    return time * 8192 / ((8192 * 8) / (bitrate / 1000));
  }
  class AudioPos
  {
  private:
    unsigned int pos;
    int bitrate;
  public:
    AudioPos(unsigned int pos_): pos(pos_), bitrate(-1){}
    AudioPos() : pos(0){}
    void set_bitrate(int b){bitrate = b;}
    int get_bitrate() const
    {
      if(bitrate == -1)
        throw error::Error(LIGHT_ERROR_LOCATION, __func__, "bitrate has not defined.");
      return bitrate;
    }
    unsigned int as_uint() const {return pos;}
    std::size_t as_size() const
    {
      if(bitrate == -1)
        throw error::Error(LIGHT_ERROR_LOCATION, __func__, "bitrate has not defined.");
      return time_to_size(pos, bitrate);
    }
  };
  
  struct Data
  {
    //input
    std::shared_ptr<music::Music> music;
    std::array<unsigned char, LIGHT_AUDIO_READ_BUFFER_SIZE> decoder_buffer;
    //pulseaudio
    std::string server;
    pa_simple* s;
    pa_sample_spec ss;
    //input output bar
    bool started;
    bool pause;
    std::condition_variable cond;
    std::mutex mtx;
    //bar
    AudioPos pos;
    bool with_bar;
    std::unique_ptr<bar::TimeBar> bar;
    
    Data() :s(NULL), with_bar(true) {  }
    ~Data()
    {
      if (s)
        pa_simple_free(s);
    }
    void bar_drain() { if (with_bar)bar->drain(); }
  };
  short scale(mad_fixed_t sample)
  {
    sample += (1L << (MAD_F_FRACBITS - 16));
    if (sample >= MAD_F_ONE)
      sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
      sample = -MAD_F_ONE;
    return sample >> (MAD_F_FRACBITS + 1 - 16);
  }
  enum mad_flow output(void* data, struct mad_header const* header, struct mad_pcm* pcm)
  {
    Data* d = (Data*)data;
    unsigned int nchannels, nsamples, i;
    const mad_fixed_t* channel[2];
    std::vector<short> output;
    output.reserve(8192);
    short sample;
    nchannels = pcm->channels;
    nsamples = pcm->length;
    for (i = 0; i < nchannels; i++)
      channel[i] = pcm->samples[i];
    while (nsamples--)
    {
      for (i = 0; i < nchannels; i++)
      {
        sample = scale(*channel[i]++);
        output.emplace_back(sample);
      }
    }
    if (d->pause)
    {
      std::unique_lock<std::mutex> lock(d->mtx);
      d->cond.wait(lock, [&d] {return !d->pause; });
    }
    pa_simple_write(d->s, &output[0], output.size() * sizeof(short), NULL);
    return MAD_FLOW_CONTINUE;
  }
  enum mad_flow input(void* data, struct mad_stream* stream)
  {
    Data* d = (Data*)data;
    if (d->music->eof())
      return MAD_FLOW_STOP;
    std::size_t bytes = 0;
    std::size_t length = 0;
    if (stream->next_frame != NULL)
    {
      bytes = stream->bufend - stream->next_frame;
      memcpy(d->decoder_buffer.data(), stream->next_frame, bytes);
    }
    length = d->music->read
        (d->decoder_buffer.data() + bytes,
         LIGHT_AUDIO_READ_BUFFER_SIZE - bytes);
    if (!d->started && length < 1024)
    {
      throw error::Error(LIGHT_ERROR_LOCATION, __func__,
                         "The data is too small. Please check the URL or path");
    }
    mad_stream_buffer(stream, d->decoder_buffer.data(), length + bytes);
    return MAD_FLOW_CONTINUE;
  }
  enum mad_flow header_func(void* data, struct mad_header const* header)
  {
    Data* d = (Data*)data;
    if(!d->started)
    {
      d->pos.set_bitrate(header->bitrate);
      if(d->pos.as_uint() != 0)
      {
        d->music->seek(d->pos.as_size());
        d->decoder_buffer.fill(0);
      }
    }
    if (!d->started)
    {
      d->started = true;
      int err;
      d->ss = { .format = PA_SAMPLE_S16LE,
          .rate = header->samplerate,
          .channels = 2 };
      if (d->s)
        pa_simple_free(d->s);
      d->s = pa_simple_new(d->server.c_str(),
                           "pulseaudio", PA_STREAM_PLAYBACK,
                           NULL, "playback", &d->ss, NULL, NULL, &err);
      if (!d->s)
      {
        throw error::Error(LIGHT_ERROR_LOCATION, __func__,
                           "Error initing PulseAudio: "
                           + std::string(pa_strerror(err)) + "\n");
      }
      if (d->with_bar)
      {
        d->bar = std::make_unique<bar::TimeBar>();
        d->bar->set_time(size_to_time(d->music->size(), header->bitrate));
        d->bar->start(d->pos.as_uint());
      }
    }
    return MAD_FLOW_CONTINUE;
  }
  enum mad_flow error(void* data,
                      struct mad_stream* stream,
                      struct mad_frame* frame)
  {
    return MAD_FLOW_CONTINUE;
  }
  
  class Audio
  {
  private:
    Data data;
    bool inited;
  public:
    Audio() : inited(false){ };
    Audio(const Audio& b) = delete;
    void no_bar() { data.with_bar = false; }
    void with_bar() { data.with_bar = true; }
    void init(const std::string& server)
    {
      data.server = server;
      inited = true;
    }
    void play(const std::shared_ptr<music::Music>& m, const AudioPos& pos_)
    {
      if (!inited)
        default_init();
      data.music = m;
      data.started = false;
      data.pause = false;
      data.pos = pos_;
      struct mad_decoder decoder;
      mad_decoder_init(&decoder, &data, input, header_func, 0, output, error, 0);
      mad_decoder_options(&decoder, 0);
      mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
      mad_decoder_finish(&decoder);
      
      pa_simple_drain(data.s, NULL);
      data.bar_drain();
    }
    AudioPos curr()
    {
      return AudioPos(size_to_time(data.music->read_size(), data.pos.get_bitrate()));
    }
    void pause()
    {
      if (data.pause)return;
      data.mtx.lock();
      data.pause = true;
      data.mtx.unlock();
      data.bar->pause();
    }
    void go()
    {
      if (!data.pause)return;
      data.mtx.lock();
      data.pause = false;
      data.mtx.unlock();
      data.cond.notify_all();
      data.bar->go();
    }
    bool is_paused() const
    {
      return data.pause;
    }
  private:
    void default_init()
    {
      auto s = getenv("PULSE_SERVER");
      if (s)
        init(getenv("PULSE_SERVER"));
      else
        throw error::Error(LIGHT_ERROR_LOCATION, __func__, "Can not find PULSE_SERVER");
    }
  };
}
#endif