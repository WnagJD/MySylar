#include "util.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>

namespace sylar{

pid_t GetThreadId()
{
    return syscall(SYS_gettid);
}


uint64_t GetFiberId()
{
    //还没有实现协程
    return 0;
}

//获取当前启动的毫秒数
uint64_t GetElapsedMS()
{
    struct timespec ts ={0};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec*1000 + ts.tv_nsec/1000000;
}


std::string GetThreadName()
{
    // char thread_name[16]={0};
    // pthread_getname_np(pthread_self(), thread_name, 16);
    // return std::string(thread_name);
    return std::string("waitingName");
}

}