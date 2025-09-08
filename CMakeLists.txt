cmake_minimum_required(VERSION 4.0)
project(minidb)

set(CMAKE_CXX_STANDARD 11)

add_executable(minidb src/main.cpp
        src/lexer/Lexer.cpp
        src/lexer/Lexer.h
        src/engine/Engine.cpp
        src/engine/Engine.h
        src/storage/Storage.cpp
        src/storage/Storage.h
)
