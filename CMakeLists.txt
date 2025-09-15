cmake_minimum_required(VERSION 4.0)
project(minidb)

set(CMAKE_CXX_STANDARD 20)

add_executable(minidb
        include/compiler/Lexer.h
        include/compiler/Parser.h
        include/compiler/SemanticAnalyzer.h
        include/compiler/QueryPlanner.h
        include/storage/Page.h
        include/storage/BufferManager.h
        include/storage/DiskManager.h
        include/storage/FileManager.h
        include/engine/ExecutionEngine.h
        include/engine/Operators.h
        include/engine/Catalog.h
        include/engine/Table.h
        include/common/Types.h
        include/common/Constants.h
        include/common/Exception.h
        src/compiler/Lexer.cpp
        src/compiler/Parser.cpp
        src/compiler/SemanticAnalyzer.cpp
        src/compiler/QueryPlanner.cpp
        src/storage/Page.cpp
        src/storage/BufferManager.cpp
        src/storage/DiskManager.cpp
        src/storage/FileManager.cpp
        src/engine/ExecutionEngine.cpp
        src/engine/Operators.cpp
        src/engine/Catalog.cpp
        src/engine/Table.cpp
        src/main.cpp
        src/cli/CommandLine.cpp
        src/cli/CommandLine.h
        include/storage/Pager.h
        src/storage/Pager.cpp
)
