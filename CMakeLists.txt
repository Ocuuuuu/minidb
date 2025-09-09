cmake_minimum_required(VERSION 4.0)
project(minidb)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)


add_executable(minidb src/main.cpp
        src/compiler/Lexer.cpp
        include/compiler/Lexer.h
        src/compiler/Parser.cpp
        include/compiler/Parser.h
        src/compiler/SemanticAnalyzer.cpp
        include/compiler/SemanticAnalyzer.h
        src/compiler/QueryPlanner.cpp
        include/compiler/QueryPlanner.h
        src/storage/Page.cpp
        include/storage/Page.h
        src/storage/BufferManager.cpp
        include/storage/BufferManager.h
        src/storage/DiskManager.cpp
        include/storage/DiskManager.h
        src/storage/FileManager.cpp
        include/storage/FileManager.h
        src/engine/ExecutionEngine.cpp
        include/engine/ExecutionEngine.h
        src/engine/Operators.cpp
        include/engine/Operators.h
        src/engine/Catalog.cpp
        include/engine/Catalog.h
        src/engine/Table.cpp
        include/engine/Table.h
        include/common/Types.h
        include/common/Constants.h
        include/common/Exception.h
)
