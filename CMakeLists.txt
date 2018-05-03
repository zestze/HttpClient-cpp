cmake_minimum_required(VERSION 3.10)
project(HttpClient_cpp)

set(CMAKE_CXX_STANDARD 17)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(src)

add_executable(HttpClient_cpp
        src/ByteRange.h
        src/Client.cpp
        src/Client.h
        src/HttpRequest.h
        src/Main.cpp
        src/Shared.cpp
        src/Shared.h)

target_link_libraries(HttpClient_cpp Threads::Threads OpenSSL::Crypto)
