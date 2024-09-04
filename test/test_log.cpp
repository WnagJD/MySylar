#include "log.h"

#include <iostream>

#include <unistd.h>

sylar::Logger::ptr g_logger= SYLAR_LOG_ROOT();

int main(int argc, char* argv[])
{
    // sylar::Logger::ptr logger(new sylar::Logger("tt"));
 
    SYLAR_LOG_INFO(g_logger)<<"info test";

    return 0;

}