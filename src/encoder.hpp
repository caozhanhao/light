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
#ifndef LIGHT_ENCODER_HPP
#define LIGHT_ENCODER_HPP

#include "logger.hpp"
#include "stream.hpp"
#include "utils.hpp"

namespace light::encoder
{
  class EncodeStream
  {
  protected:
    std::shared_ptr<stream::OutputStream> out;
    utils::MusicInfo info;
  public:
    EncodeStream(std::shared_ptr<stream::OutputStream> o) : out(o) {}
  
    virtual void write(const void *data, std::size_t bytes) = 0;
  
    virtual void set_info(utils::MusicInfo info_)
    {
      info = info_;
    }
  
    auto get_output()
    {
      return out;
    }
  };
  
  class WavEncodeStream : public EncodeStream
  {
  private:
    struct WavHeader
    {
      struct Riff
      {
        std::array<char, 4> riff{'R', 'I', 'F', 'F'};
        uint32_t file_size;
        std::array<char, 4> wave{'W', 'A', 'V', 'E'};
      } riff;
      struct Format
      {
        std::array<char, 4> fmt{'f', 'm', 't', ' '};
        uint32_t blockSize = 16;
        uint16_t formatTag;
        uint16_t channels;
        uint32_t samples_per_sec;
        uint32_t avg_bytes_per_sec;
        uint16_t block_align;
        uint16_t bits_per_sample;
      } format;
      struct Data
      {
        const char data[4] = {'d', 'a', 't', 'a'};
        uint32_t size;
      } data;
      
      WavHeader() {}

      WavHeader(int channels, int samplerate, int bits_per_sample, int data_size)
      {
        riff.file_size = 36 + data_size;
        format.formatTag = 1;
        format.channels = channels;
        format.samples_per_sec = samplerate;
        format.avg_bytes_per_sec = samplerate * channels * bits_per_sample / 8;
        format.block_align = channels * bits_per_sample / 8;
        format.bits_per_sample = bits_per_sample;
        data.size = data_size;
      }
    };
  
    std::size_t bitscount;
  public:
    WavEncodeStream(std::string name) :
        EncodeStream(std::make_shared<stream::FileOutputStream>(name)), bitscount(0) {}
  
    WavEncodeStream() : EncodeStream(nullptr), bitscount(0) {}
  
    void write(const void *data, std::size_t bytes)
    {
      out->write(data, bytes);
      bitscount += bytes;
    }
  
    void set_out(std::shared_ptr<stream::FileOutputStream> a)
    {
      out = a;
    }
    
    void set_info(utils::MusicInfo info_) override
    {
      info = info_;
      auto ptr = std::dynamic_pointer_cast<stream::FileOutputStream>(out);
      ptr->native_handle().seekp(sizeof(WavHeader));
    }
    
    ~WavEncodeStream()
    {
      if (bitscount != 0)
      {
        WavHeader h(info.channels, info.samplerate, 16, bitscount);
        auto ptr = std::dynamic_pointer_cast<stream::FileOutputStream>(out);
        ptr->native_handle().seekp(std::ios::beg);
        ptr->write(&h, sizeof(WavHeader));
      }
    }
  };
  
  class AudioEncodeStream : public EncodeStream
  {
  public:
    AudioEncodeStream() : EncodeStream(std::make_shared<stream::AudioOutputStream>()) {}
    
    void write(const void *data, std::size_t bytes)
    {
      out->write(data, bytes);
    }
    
    void set_out(std::shared_ptr<stream::AudioOutputStream> a)
    {
      out = a;
    }
    
    void set_info(utils::MusicInfo info_) override
    {
      info = info_;
      auto ptr = std::dynamic_pointer_cast<stream::AudioOutputStream>(out);
      ptr->set_samplerate(info.samplerate);
    }
  };
}
#endif