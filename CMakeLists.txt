cmake_minimum_required(VERSION 3.20)
project(minidb)

set(CMAKE_CXX_STANDARD 20)
# 强制要求编译器必须支持指定的C++标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 使用GLOB自动查找所有源文件（推荐，更简洁）
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 包含头文件目录
include_directories(include)

# 创建主可执行文件
add_executable(minidb ${SOURCES})


# 创建库目标用于测试
add_library(minidb_lib ${SOURCES})
target_include_directories(minidb_lib PUBLIC include)

# 启用测试
enable_testing()

# 包含测试目录
add_subdirectory(tests)

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(minidb PRIVATE -Wall -Wextra)
    target_compile_options(minidb_lib PRIVATE -Wall -Wextra)
endif()