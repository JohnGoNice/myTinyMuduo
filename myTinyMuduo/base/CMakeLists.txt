cmake_minimum_required(VERSION 3.10)
project(my_tinymuduo_base)
set(CMAKE_CXX_FLAGS "-g -Wall")
set(CMAKE_CXX_STANDARD 14)
include_directories(../)
set(base_SRCS
        CountDownLatch.cpp
        Logging.cpp
        LogStream.cpp
        Timestamp.cpp
        Thread.cpp
        ThreadPool.cpp
        FileUtil.cpp
        LogFile.cpp
        AsyncLogging.cpp
        )
add_library(muduo_base ${base_SRCS})

target_link_libraries(muduo_base pthread rt)