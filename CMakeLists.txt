cmake_minimum_required(VERSION 3.28)
project(jsrt)

set(CMAKE_CXX_STANDARD 20)

add_executable(jsrt src/main.cpp
        src/file/read.cpp
        src/file/read.h
        src/runtime/error.cpp
        src/runtime/error.h
        src/runtime/console.cpp
        src/runtime/console.h
        src/runtime/module.cpp
        src/runtime/module.h)

target_include_directories(jsrt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(jsrt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libv8_monolith.a
        pthread
        dl
        m
)
