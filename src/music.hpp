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
#ifndef LIGHT_MUSIC_HPP
#define LIGHT_MUSIC_HPP
#include <memory>
#include <fstream>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <cstring>
namespace light::music
{
  class Music
  {
  public:
    Music() = default;
    virtual std::size_t read(unsigned char* dest, std::size_t n) = 0;
    virtual bool eof() const = 0;
    virtual std::size_t size() const = 0;
    virtual std::size_t read_size() const = 0;
    virtual void seek(std::size_t) = 0;
  };
  class File : public Music
  {
  private:
    std::shared_ptr<std::fstream> file;
    std::size_t file_size;
  public:
    File(const std::shared_ptr<std::fstream>& f) :file(f)
    {
      file->ignore(std::numeric_limits<std::streamsize>::max());
      file_size = file->gcount();
      file->clear();
      file->seekg(std::ios_base::beg);
    }
    std::size_t read(unsigned char* dest, std::size_t n)  override
    {
      return file->read(reinterpret_cast<char*>(dest), n).gcount();
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
  };
}
namespace light::http
{
  size_t buffer_no_bar_progress_callback(void* userp, double dltotal, double dlnow, double ultotal, double ulnow);
  std::size_t buffer_write_callback(void* data, size_t size, size_t nmemb, void* userp);
}
namespace light::music
{
  class Buffer : public Music
  {
    friend size_t
    http::buffer_no_bar_progress_callback(void *userp, double dltotal, double dlnow, double ultotal, double ulnow);
    
    friend std::size_t http::buffer_write_callback(void *data, size_t size, size_t nmemb, void *userp);
  
  private:
    std::condition_variable cond;
    std::mutex mtx;
    bool can_next;
    bool is_end;
    std::size_t total_size;
    
    std::vector<unsigned char> buffer;
    std::size_t readpos;
  public:
    Buffer() : can_next(false), is_end(false), readpos(0) {}
    
    std::size_t size() const override
    {
      return total_size;
    }
    
    bool eof() const override
    {
      return is_end;
    }
    
    std::size_t read(unsigned char *dest, std::size_t n) override
    {
      while (!eof() && unread_size() < n)
        next();
      std::size_t realsize = n;
      if (unread_size() < n)
        realsize = unread_size();
      memcpy(dest, buffer.data() + readpos, realsize);
      readpos += realsize;
      prepare(n);
      return realsize;
    }
    
    void prepare(std::size_t n)
    {
      if (eof() || unread_size() >= n)
        return;
      next();
    }
    
    std::size_t read_size() const override
    {
      return readpos;
    }
    
    void seek(std::size_t size) override
    {
      while (!eof() && unread_size() < size)
        next();
      readpos = buffer.size();
    }
  
  private:
    void write(unsigned char *arr, std::size_t n)
    {
      auto temp = buffer.size();
      buffer.resize(buffer.size() + n);
      memcpy(&buffer[temp], arr, n);
    }
    
    void next()
    {
      mtx.lock();
      can_next = true;
      mtx.unlock();
      cond.notify_all();
    }
    
    unsigned char *unread_data()
    {
      return buffer.data() + readpos;
    }
    
    std::size_t unread_size() const
    {
      return buffer.size() - readpos;
    }
    
    void set_size(const std::size_t size_) { total_size = size_; }
    
    std::condition_variable &get_cond() { return cond; }
    
    std::mutex &get_mutex() { return mtx; }
    
    bool &can_next_buffer() { return can_next; }
    
    void set_eof() { is_end = true; };
  };
}
#endif