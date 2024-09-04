cmake_minimum_required(VERSION 3.0)
project(sylar)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)

#指定编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++11 -O0 -ggdb -Wall -Werror")

# -rdynamic: 将所有符号都加入到符号表中，便于使用dlopen或者backtrace追踪到符号
# -fPIC: 生成位置无关的代码，便于动态链接
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -fPIC")

#-Wno-unused-function: 不要警告没有使用的函数
#-Wno-builtin-macro-redefined:不要警告内置宏的重定义
#-Wno-depreacted：不要警告过时的特性
#-Wno-deprecated-declarations:不要警告使用带deprecated属性的变量、类型、函数

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated -Wno-deprecated-declarations")

include_directories(.)

option(BUILD_TEST "ON for complie test" ON)

find_package(Boost REQUIRED)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(LIB_SRC 
    sylar/log.cpp
    sylar/config.cpp
    sylar/util.cpp
    sylar/thread.cpp
    sylar/mutex.cpp
    sylar/fiber.cpp
    sylar/scheduler.cpp
    sylar/iomanager.cpp
    sylar/fd_manager.cpp
    sylar/time.cpp
    sylar/hook.cpp
    sylar/bytearry.cpp
    sylar/socket.cpp
    sylar/tcp_server.cpp
    )

add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar)

set(LIBS  sylar yaml-cpp)

if(BUILD_TEST)
sylar_add_executable(test_log "test/test_log.cpp" sylar "${LIBS}")
endif()

# add_executable(test_log test/test_log.cpp )
# force_redefine_file_macro_for_sources(test_log)
# target_link_libraries(test_log sylar yaml-cpp)

include_directories(${PROJECT_SOURCE_DIR}/sylar)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# link_directories(${PROJECT_SOURCE_DIR}/lib)




