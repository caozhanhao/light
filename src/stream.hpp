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
#ifndef LIGHT_STREAM_HPP
#define LIGHT_STREAM_HPP

#include "audio.hpp"
#include <memory>
#include <fstream>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <cstring>

namespace light::stream
{
  class InputStream
  {
  private:
    struct MP3Header
    {
      std::array<char, 3> header;
      char version;
      char revision;
      char flag;
      std::array<char, 4> size;
    } header;
  
    int total_size = (header.size[0] & 0x7f) * 0x200000
                     + (header.size[1] & 0x7f) * 0x400 + (header.size[2] & 0x7f) * 0x40 + (header.size[3] & 0x7f);
  public:
    InputStream() = default;
  
    virtual std::size_t read(unsigned char *dest, std::size_t n) = 0;
  
    virtual void ignore(std::size_t n) = 0;
  
    virtual bool eof() const = 0;
  
    virtual std::size_t size() const = 0;
  
    virtual std::size_t read_size() const = 0;
  
    virtual void seek(std::size_t) = 0;
  
    virtual void seek_cur_offset(int offset) = 0;
  
  };
  
  class FileInputStream : public InputStream
  {
  private:
    std::shared_ptr<std::fstream> file;
    std::size_t file_size;
  public:
    FileInputStream(std::string fn) : file(std::make_shared<std::fstream>(fn))
    {
      if (!file->good())
      {
        throw logger::Error(LIGHT_ERROR_LOCATION, __func__, "Open file failed.");
      }
      file->ignore(std::numeric_limits<std::streamsize>::max());
      file_size = file->gcount();
      file->clear();
      file->seekg(std::ios_base::beg);
    }
    
    FileInputStream(std::shared_ptr<std::fstream> f) : file(std::move(f))
    {
      if (!file->good())
      {
        throw logger::Error(LIGHT_ERROR_LOCATION, __func__, "Open file failed.");
      }
      file->ignore(std::numeric_limits<std::streamsize>::max());
      file_size = file->gcount();
      file->clear();
      file->seekg(std::ios_base::beg);
    }
    
    std::size_t read(unsigned char *dest, std::size_t n) override
    {
      return file->read(reinterpret_cast<char *>(dest), n).gcount();
    }
    
    void ignore(std::size_t n) override
    {
      file->ignore(n);
    }
    
    bool eof() const override
    {
      return file->eof();
    }
    
    std::size_t size() const override
    {
      return file_size;
    }
  
    std::size_t read_size() const override
    {
      return file->tellg();
    }
  
    void seek(std::size_t size) override
    {
      file->clear();
      file->seekg(size, std::ios_base::beg);
    }
  
    void seek_cur_offset(int offset) override
    {
      if (offset * -1 > file->tellg())
      {
        file->seekg(std::ios::beg);
      }
      else
      {
        file->seekg(offset, std::ios::cur);
      }
    }
  };
  
  class NetInputStream : public InputStream
  {
  private:
    std::atomic<bool> written;
    bool is_end;
    std::size_t total_size;
    
    std::vector<unsigned char> buffer;
    std::size_t readpos;
  public:
    NetInputStream() : written(false), is_end(false), readpos(0) {}
    
    std::size_t size() const override
    {
      return total_size;
    }
    
    bool eof() const override
    {
      return (is_end && unread_size() == 0);
    }
    
    std::size_t read(unsigned char *dest, std::size_t n) override
    {
      while (!is_end && unread_size() < n)
      {
        wait_for_data();
      }
      std::size_t realsize = n;
      if (unread_size() < n)
      {
        realsize = unread_size();
      }
      memcpy(dest, buffer.data() + readpos, realsize);
      readpos += realsize;
      return realsize;
    }
  
    void ignore(std::size_t n) override
    {
      while (!is_end && unread_size() < n)
      {
        wait_for_data();
      }
      std::size_t realsize = n;
      if (unread_size() < n)
      {
        realsize = unread_size();
      }
      readpos += realsize;
    }
  
    std::size_t read_size() const override
    {
      return readpos;
    }
  
    void seek(std::size_t size) override
    {
      while (!is_end && buffer.size() < size)
      {
        wait_for_data();
      }
      readpos = size;
    }
  
    void seek_cur_offset(int offset) override
    {
      if (offset * -1 >= readpos) { readpos = 0; }
      else { readpos += offset; }
    }
  
    auto &get_flag() { return written; }
  
    void set_size(const std::size_t size_) { total_size = size_; }
  
    void set_eof() { is_end = true; };
  
    void write(unsigned char *arr, std::size_t n)
    {
      auto temp = buffer.size();
      buffer.resize(buffer.size() + n);
      memcpy(&buffer[temp], arr, n);
    }

  private:
    void wait_for_data()
    {
      while (!written)
      {
        std::this_thread::yield();
      }
    }
  
    std::size_t unread_size() const
    {
      return buffer.size() - readpos;
    }
  };
  
  enum OutputMode
  {
    file, audio
  };
  
  class OutputStream
  {
  private:
    OutputMode mode;
  public:
    OutputStream(OutputMode mode_) : mode(mode_) {}
    
    virtual void write(const void *data, std::size_t bytes) = 0;
    
    OutputMode get_mode() { return mode; }
  };
  
  class FileOutputStream : public OutputStream
  {
    std::ofstream fs;
  public:
    FileOutputStream(std::string fn) : OutputStream(OutputMode::file), fs(std::move(fn), std::ios::binary)
    {
      if (fs.good())
      {
        throw logger::Error(LIGHT_ERROR_LOCATION, __func__, "Open file failed.");
      }
    }
    
    void write(const void *data, std::size_t bytes) override
    {
      fs.write(reinterpret_cast<const char *>(data), bytes);
    }
    
    std::ofstream &native_handle() { return fs; }
  };
  
  class AudioOutputStream : public OutputStream
  {
  private:
    audio::Audio audio;
  public:
    AudioOutputStream() : OutputStream(OutputMode::audio) {}
    
    void write(const void *data, std::size_t bytes) override
    {
      audio.write(data, bytes);
    }
    
    void set_audio_server(const std::string &server)
    {
      audio.set_server(server);
      audio.init();
    }
    
    void set_samplerate(unsigned int rate)
    {
      audio.set_samplerate(rate);
    }
  };
}
#endif