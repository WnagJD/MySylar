#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <stdint.h>
#include <sys/types.h>
#include <string>
#include <cxxabi.h>

namespace sylar
{

pid_t GetThreadId();


uint64_t GetFiberId();

//获取当前启动的毫秒数
uint64_t GetElapsedMS();


std::string GetThreadName();









template<class T>
const char* TypeToName()
{
   static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr); 
   return s_name;
}






}

#endif