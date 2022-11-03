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
#ifndef LIGHT_HTTP_HPP
#define LIGHT_HTTP_HPP
#include "bar.hpp"
#include "logger.hpp"
#include "stream.hpp"
#include "curl/curl.h"
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <utility>
#include <fstream>
#include <thread>
namespace light::http
{
  class Response
  {
  private:
    std::variant<int,
        std::shared_ptr<std::string>,
        std::shared_ptr<std::fstream>,
        std::shared_ptr<stream::NetInputStream>> value;//int EMPTY
  public:
    Response() : value(0) {}
    
    Response(const Response &res) : value(res.value) {}
    
    void set_str()
    {
      value.emplace<std::shared_ptr<std::string>>
          (std::make_shared<std::string>
               (std::string()));
    }
    
    void set_file(const std::string &filename)
    {
      auto f = std::make_shared<std::fstream>
          (std::fstream(filename,
                        std::ios_base::out
                        | std::ios_base::trunc
                        | std::ios_base::in
                        | std::ios_base::binary));
      if (!f->is_open())
      {
        throw logger::Error(LIGHT_ERROR_LOCATION, __func__, "Open file failed.");
      }
      value.emplace<std::shared_ptr<std::fstream>>
          (f);
    }
    
    void set_buffer()
    {
      value.emplace
          <std::shared_ptr<stream::NetInputStream>>
          (std::make_shared<stream::NetInputStream>
               ());
    }
    
    std::string str() { return *std::get<1>(value); }
    
    std::shared_ptr<std::string> strp() { return std::get<1>(value); }
    
    std::shared_ptr<std::fstream> file() { return std::get<2>(value); }
    
    std::shared_ptr<stream::NetInputStream> buffer()
    {
      return std::get<3>(value);
    }
    
    bool is_buffer()
    {
      return value.index() == 3;
    }
    
    bool empty() { return value.index() == 0; }
  };
  
  size_t buffer_progress_callback(void *userp, double dltotal, double dlnow, double ultotal, double ulnow)
  {
    if (dltotal == 0) return 0;
    auto pd = (Response *) userp;
    if (pd->buffer()->size() != dltotal)
    {
      pd->buffer()->set_size(std::size_t(dltotal));
    }
    if (dlnow == dltotal)
    {
      pd->buffer()->set_eof();
    }
    //LIGHT_NOTICE("Downloading : " + std::to_string((double(dlnow) / double(dltotal))* 100) + "%");
    return 0;
  }
  
  std::size_t str_write_callback(void *data, size_t size, size_t nmemb, void *userp)
  {
    auto p = (Response *) userp;
    p->strp()->append((char *) data, size * nmemb);
    return size * nmemb;
  }
  
  std::size_t buffer_write_callback(void *data, size_t size, size_t nmemb, void *userp)
  {
    auto &buffer = *((Response *) userp)->buffer();
    buffer.write((unsigned char *) data, size * nmemb);
    buffer.get_flag() = true;
    return size * nmemb;
  }
  
  std::size_t file_write_callback(void *data, size_t size, size_t nmemb, void *userp)
  {
    auto p = (Response *) userp;
    p->file()->write((char *) data, size * nmemb);
    return size * nmemb;
  }
  
  class Http
  {
  public:
    Response response;
    long int response_code;
  private:
    CURL *curl;
  public:
    Http() : response_code(-1), curl(curl_easy_init())
    {
      init();
    }
  
    Http(const std::string &url_) : response_code(-1), curl(curl_easy_init())
    {
      init();
      set_url(url_);
    }
    
    Http(const Http &http) = delete;
  
    Http &set_url(const std::string &url_)
    {
      auto url = url_;
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      return *this;
    }
  
    Http &set_str()
    {
      response.set_str();
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, str_write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      return *this;
    }
    
    Http &set_file(const std::string &filename)
    {
      response.set_file(filename);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, file_write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      return *this;
    }
    
    Http &set_buffer()
    {
      response.set_buffer();
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
      curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &response);
      curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, buffer_progress_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, buffer_write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      return *this;
    }
    
    Http &get()
    {
      if (response.empty())
        set_str();
      curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
      auto cret = curl_easy_perform(curl);
      if (cret != CURLE_OK)
      {
        throw logger::Error(LIGHT_ERROR_LOCATION, __func__,
                            "curl_easy_perform() failed: '" + std::string(curl_easy_strerror(cret)) + "'.");
      }
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
      return *this;
    }
  
  private:
    void init()
    {
      if (!curl)
      {
        throw logger::Error(LIGHT_ERROR_LOCATION, __func__, "curl_easy_init() failed");
      }
    }
  };
}
#endif