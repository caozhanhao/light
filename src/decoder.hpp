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
#ifndef LIGHT_DECODER_HPP
#define LIGHT_DECODER_HPP

#include "bar.hpp"
#include "stream.hpp"
#include "error.hpp"

#include <mad.h>
#include <string.h>

#include <array>
#include <vector>
#include <string>
#include <memory>
#include <future>

namespace light::decoder
{
  struct MusicInfo
  {
    unsigned int time;
  };
  struct Data
  {
    std::shared_ptr<std::promise<MusicInfo>> info;
    std::shared_ptr<stream::InputStream> input_stream;
    std::shared_ptr<stream::OutputStream> output_stream;
    std::array<unsigned char, LIGHT_AUDIO_READ_BUFFER_SIZE> decoder_buffer;
    
    bool pause;
    std::condition_variable cond;
    std::mutex mtx;
  };
  
  short scale(mad_fixed_t sample)
  {
    sample += (1L << (MAD_F_FRACBITS - 16));
    if (sample >= MAD_F_ONE)
    {
      sample = MAD_F_ONE - 1;
    }
    else if (sample < -MAD_F_ONE)
    {
      sample = -MAD_F_ONE;
    }
    return sample >> (MAD_F_FRACBITS + 1 - 16);
  }
  
  enum mad_flow output(void *data, struct mad_header const *header, struct mad_pcm *pcm)
  {
    Data *d = (Data *) data;
    unsigned int nchannels, nsamples, i;
    const mad_fixed_t *channel[2];
    std::vector<short> output;
    output.reserve(8192);
    short sample;
    nchannels = pcm->channels;
    nsamples = pcm->length;
    for (i = 0; i < nchannels; i++)
    {
      channel[i] = pcm->samples[i];
    }
    while (nsamples--)
    {
      for (i = 0; i < nchannels; i++)
      {
        sample = scale(*channel[i]++);
        output.emplace_back(sample);
      }
    }
    d->output_stream->write(&output[0], output.size() * sizeof(short));
    return MAD_FLOW_CONTINUE;
  }
  
  enum mad_flow input(void *data, struct mad_stream *stream)
  {
    Data *d = (Data *) data;
    if (d->input_stream->eof())
    {
      return MAD_FLOW_STOP;
    }
    std::size_t bytes = 0;
    std::size_t length = 0;
    if (stream->next_frame != NULL)
    {
      bytes = stream->bufend - stream->next_frame;
      memcpy(d->decoder_buffer.data(), stream->next_frame, bytes);
    }
    length = d->input_stream->read
        (d->decoder_buffer.data() + bytes,
         LIGHT_AUDIO_READ_BUFFER_SIZE - bytes);
    mad_stream_buffer(stream, d->decoder_buffer.data(), length + bytes);
    return MAD_FLOW_CONTINUE;
  }
  
  enum mad_flow header_func(void *data, struct mad_header const *header)
  {
    Data *d = (Data *) data;
    if (d->info != nullptr)
    {
      d->info->set_value(
          MusicInfo{
              .time = static_cast<unsigned int>
              ((d->input_stream->size() / 8192) * ((8192 * 8) / (header->bitrate / 1000)))
          }
      );
      d->output_stream->set_samplerate(header->samplerate);
      d->info = nullptr;
    }
    if (d->pause)
    {
      std::unique_lock<std::mutex> lock(d->mtx);
      d->cond.wait(lock, [&d] { return !d->pause; });
    }
    return MAD_FLOW_CONTINUE;
  }
  
  enum mad_flow error(void *data,
                      struct mad_stream *stream,
                      struct mad_frame *frame)
  {
    return MAD_FLOW_CONTINUE;
  }
  
  class Decoder
  {
  private:
    Data data;
    bool inited;
  public:
    Decoder() : inited(false) {};
    
    Decoder(const Decoder &b) = delete;
    
    void decode(const std::shared_ptr<stream::InputStream> &in,
                const std::shared_ptr<stream::OutputStream> &out,
                const std::shared_ptr<std::promise<MusicInfo>> &info)
    {
      data.input_stream = in;
      data.output_stream = out;
      data.info = info;
      data.pause = false;
      struct mad_decoder decoder;
      mad_decoder_init(&decoder, &data, input, header_func, 0, output, error, 0);
      mad_decoder_options(&decoder, 0);
      mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
      mad_decoder_finish(&decoder);
    }
    
    bool is_paused() const
    {
      return data.pause;
    }
    
    void pause()
    {
      if (data.pause)return;
      data.mtx.lock();
      data.pause = true;
      data.mtx.unlock();
    }
    
    void go()
    {
      if (!data.pause)return;
      data.mtx.lock();
      data.pause = false;
      data.mtx.unlock();
      data.cond.notify_all();
    }
  };
}
#endif