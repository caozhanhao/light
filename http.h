#pragma once

#include "bar.h"
#include "error.h"
#include "music.h"
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
      std::shared_ptr<music::Buffer>> value;//int EMPTY
  public:
    Response() : value(0) {}
    Response(const Response& res) : value(res.value) {  }
    void set_str()
    {
      value.emplace<std::shared_ptr<std::string>>
        (std::make_shared<std::string>
          (std::string()));
    }
    void set_file(const std::string& filename)
    {
      auto f = std::make_shared<std::fstream>
        (std::fstream(filename,
          std::ios_base::out
          | std::ios_base::trunc
          | std::ios_base::in
          | std::ios_base::binary));
      if (!f->is_open())
        throw error::Error(LIGHT_ERROR_LOCATION, __func__, "Open file failed.");
      value.emplace<std::shared_ptr<std::fstream>>
        (f);
    }
    void set_buffer()
    {
      value.emplace
        <std::shared_ptr<music::Buffer>>
        (std::make_shared<music::Buffer>
          ());
    }
    std::string str() { return *std::get<1>(value); }
    std::shared_ptr<std::string> strp() { return std::get<1>(value); }
    std::shared_ptr<std::fstream> file() { return std::get<2>(value); }
    std::shared_ptr<music::Buffer> buffer()
    { 
      return std::get<3>(value);
    }
    bool is_buffer()
    {
      return value.index() == 3;
    }
    bool empty() { return value.index() == 0; }
  };
  class Url
  {
  private:
    class ParsedUrl
    {
    public:
      std::vector<std::pair<std::string, std::string>> params;
      std::vector<std::string> paths;
      std::vector<std::string> domain;
      std::string protocol;
      int port;
    public:
      ParsedUrl() : port(-1) {  }
    private:
      void str_encode(std::string& v)
      {
        CURL* curl = curl_easy_init();
        char* output = curl_easy_escape(curl, v.c_str(), v.size());
        v = output;
        curl_free(output);
        curl_easy_cleanup(curl);
      }
    public:
      void set_protocol(const std::string& v) { protocol = v; }
      void set_port(int v) { port = v; }
      void add_param(const std::pair<std::string, std::string>& v)
      {
        auto s = v;
        str_encode(s.first);
        str_encode(s.second);
        params.emplace_back(s);
      }
      void add_path(const std::string& v)
      {
        auto s = v;
        str_encode(s);
        paths.emplace_back(s);
      }
      void add_domain(const std::string& v)
      {
        auto s = v;
        str_encode(s);
        domain.emplace_back(s);
      }
    };
  private:
    ParsedUrl url;
  public:
    Url(const std::string& url_)
    {
      parse(url_);
    }
    Url(const std::string& url_,
      const std::initializer_list<std::pair<std::string, std::string>>& il)
    {
      parse(url_);
      add_param(il);
    }
    Url(const std::string& url_,
      const std::initializer_list<std::string>& il)
    {
      parse(url_);
      add_path(il);
    }
    Url(const std::string& url_,
      const std::initializer_list<std::string>& il1,
      const std::initializer_list<std::pair<std::string, std::string>>& il2)
    {
      parse(url_);
      add_path(il1);
      add_param(il2);
    }
    std::string get() const { return *total(); }
    Url& add_param(const std::initializer_list<std::pair<std::string, std::string>>& il)
    {
      for (auto& r : il)
        url.add_param(r);
      return *this;
    }
    Url& add_path(const std::initializer_list<std::string>& il)
    {
      for (auto& r : il)
        url.add_path(r);
      return *this;
    }
  private:
    void parse(const std::string& unparsed_url)
    {
      auto it = unparsed_url.cbegin();
      auto check = [this, &unparsed_url]
      (const std::string::const_iterator& it)-> bool
      { return it < unparsed_url.cend(); };
      std::string temp = "";
      while (check(it) && isalpha(*it))
      {
        temp += *it;
        it++;
      }
      url.set_protocol(temp);
      while (check(it) && !isalpha(*it) && !isalnum(*it))
        it++;
      auto add_to_domain = [this, check, &it]()
      {
        std::string temp;
        while (check(it) && *it != '.' && *it != ':' && *it != '/')
        {
          temp += *it;
          it++;
        }
        url.add_domain(temp);
      };
      while (check(it) && *it != '/' && *it != ':')
      {
        if (*it == '.') it++;
        add_to_domain();
      }
      if (!check(it)) return;//no other, just return
      if (*it == ':')
      {
        it++;
        std::string temp;
        while (check(it) && isalnum(*it))
        {
          temp += *it;
          it++;
        }
        url.set_port(std::stoi(temp));
      }
      if (!check(it)) return;//no paths, just return
      auto add_to_paths = [this, check, &it]()
      {
        std::string temp;
        while (check(it) && *it != '/' && *it != '?')
        {
          temp += *it;
          it++;
        }
        url.add_path(temp);
      };
      while (check(it) && *it != '?')
      {
        if (*it == '/') it++;
        add_to_paths();
      }
      if (!check(it)) return;//no params, just return
      it++;//eat '?'
      auto add_to_params = [this, check](std::string::const_iterator& it)
      {
        std::string key;
        std::string value;
        while (check(it) && *it != '&')
        {
          while (check(it) && *it != '=')
          {
            key += *it;
            it++;
          }
          it++;
          while (check(it) && *it != '&')
          {
            value += *it;
            it++;
          }
        }
        url.add_param({ key,value });
      };
      while (check(it))
      {
        if (*it == '&')  it++;;
        add_to_params(it);
      }
    }
    std::unique_ptr<std::string> total() const
    {
      std::unique_ptr<std::string> ret(new std::string(""));
      *ret += url.protocol;
      *ret += "://";
      for (auto& r : url.domain)
      {
        *ret += r;
        *ret += '.';
      }
      ret->pop_back();
      if (url.port != -1)
      {
        *ret += ":";
        *ret += std::to_string(url.port);
      }
      if (!url.paths.empty())
      {
        *ret += "/";
        for (auto& r : url.paths)
        {
          *ret += r;
          *ret += '/';
        }
        ret->pop_back();
      }
      if (!url.params.empty())
      {
        *ret += "?";
        for (auto& r : url.params)
        {
          *ret += r.first;
          *ret += '=';
          *ret += r.second;
          *ret += '&';
        }
        ret->pop_back();
      }
      return ret;
    }
  };
  struct ProcessBarData
  {
    bar::Bar bar;
    bool percent;
  };
  size_t progress_callback(void* userp, double dltotal, double dlnow, double ultotal, double ulnow)
  {
    auto pd = (ProcessBarData*)userp;
    if (dltotal != 0)
    {
      if (pd->percent)
      {
        std::string percent = " " + std::to_string((dlnow / dltotal) * 100) + "%";
        pd->bar.update(dlnow / dltotal, percent);
      }
      else
        pd->bar.update(dlnow / dltotal);
    }
    return 0;
  }
  size_t buffer_no_bar_progress_callback(void* userp, double dltotal, double dlnow, double ultotal, double ulnow)
  {
    if (dltotal == 0) return 0;
    auto pd = (Response*)userp;
    if (pd->buffer()->size() == 0)
      pd->buffer()->set_size(std::size_t(dltotal));
    if (dlnow == dltotal)
      pd->buffer()->set_eof();
    return 0;
  }
  size_t buffer_progress_callback(void* userp, double dltotal, double dlnow, double ultotal, double ulnow)
  {
    buffer_no_bar_progress_callback(userp, dltotal, dlnow, ultotal, ulnow);
    progress_callback(userp, dltotal, dlnow, ultotal, ulnow);
    return 0;
  }
  std::size_t str_write_callback(void* data, size_t size, size_t nmemb, void* userp)
  {
    auto p = (Response*)userp;
    p->strp()->append((char*)data, size * nmemb);
    return size * nmemb;
  }
  std::size_t buffer_write_callback(void* data, size_t size, size_t nmemb, void* userp)
  {
    auto& buffer = *((Response*)userp)->buffer();
    buffer.write((unsigned char*)data, size * nmemb);
    buffer.can_next_buffer() = false;
    std::unique_lock<std::mutex> lock(buffer.get_mutex());
    buffer.get_cond().wait(lock,
      [&buffer]() { return buffer.can_next_buffer(); });
    return size * nmemb;
  }
  std::size_t file_write_callback(void* data, size_t size, size_t nmemb, void* userp)
  {
    auto p = (Response*)userp;
    p->file()->write((char*)data, size * nmemb);
    return size * nmemb;
  }
  class Http
  {
  public:
    Response response;
    long int response_code;
  private:
    CURL* curl;
    ProcessBarData process_bar_data;
  public:
    Http() : response_code(-1), curl(curl_easy_init())
    {
      init();
    }
    Http(const Url& url_) : response_code(-1), curl(curl_easy_init())
    {
      init();
      set_url(url_);
    }
    Http(const Http& http) = delete;
    Http& set_url(const Url& url_)
    {
      auto url = url_;
      curl_easy_setopt(curl, CURLOPT_URL, url.get().c_str());
      return *this;
    }
    Http& set_str()//default
    {
      response.set_str();
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, str_write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      return *this;
    }
    Http& set_file(const std::string& filename)
    {
      response.set_file(filename);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, file_write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      return *this;
    }
    Http& set_buffer()
    {
      response.set_buffer();
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
      curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &response);
      curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, buffer_no_bar_progress_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, buffer_write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      return *this;
    }
    Http& enable_bar(std::size_t size_ = 36, char progress_ = '>', char padding_ = ' ', bool with_percent_ = true)
    {
      process_bar_data.percent = with_percent_;
      process_bar_data.bar.set(size_, progress_, padding_);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
      curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &process_bar_data);
      if (response.empty())
        throw error::Error(LIGHT_ERROR_LOCATION, __func__, "Can not enable the bar before setting response's type.");
      if (response.is_buffer())
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, buffer_progress_callback);
      return *this;
    }
    Http& get()
    {
      if (response.empty())
        set_str();
      curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
      auto cret = curl_easy_perform(curl);
      if (cret != CURLE_OK)
        throw error::Error(LIGHT_ERROR_LOCATION, __func__, "curl_easy_perform() failed: '" + std::string(curl_easy_strerror(cret)) + "'.");
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
      return *this;
    }
  private:
    void init()
    {
      if (!curl)
        throw error::Error(LIGHT_ERROR_LOCATION, __func__, "curl_easy_init() failed");
    }
  };
}