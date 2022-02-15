#pragma once
#include <stdexcept>
#include <string>

#define LIGHT_STRINGFY(x) _LIGHT_STRINGFY(x)
#define _LIGHT_STRINGFY(x) #x
#define LIGHT_ERROR_LOCATION  __FILE__ ":" LIGHT_STRINGFY(__LINE__) 
namespace light::error
{
  class Error : public std::logic_error
  {
  public:
    Error(std::string location, std::string func_name, std::string details)
      : logic_error("In File: " + location+ ":" + func_name+ "(): \n" + details)
    {}
  };
}