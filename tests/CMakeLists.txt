# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动查找所有测试文件（包括 test_main.cpp）
file(GLOB_RECURSE TEST_SOURCES
        "*.cpp"                    # 这会包含 tests/ 根目录下的 test_main.cpp
        "unit_tests/*.cpp"
        "integration_tests/*.cpp"
)

# 添加 Catch2 的实现文件（这是解决链接错误的关键！）
list(APPEND TEST_SOURCES "catch2/catch_amalgamated.cpp")

if(TEST_SOURCES)
    # 创建测试可执行文件
    add_executable(minidb_tests ${TEST_SOURCES}
            unit_tests/test_schema.cpp
            unit_tests/test_table_info.cpp
            unit_tests/test_catalog_manager.cpp
            integration_tests/test_catalog.cpp
    )

    # 链接主库
    target_link_libraries(minidb_tests minidb_lib)

    # 包含头文件目录
    target_include_directories(minidb_tests PRIVATE
            ${CMAKE_SOURCE_DIR}/include        # 主项目的头文件
            ${CMAKE_CURRENT_SOURCE_DIR}        # 当前测试目录的头文件
    )

    # 添加测试
    add_test(NAME MinidbTests COMMAND minidb_tests)

    # 编译选项
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(minidb_tests PRIVATE -Wall -Wextra)
    endif()
else()
    message(STATUS "No test files found in tests directory")
endif()