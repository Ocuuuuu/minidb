//
// Created by lenovo on 2025/9/12.
//

#ifndef MINIDB_LOGGER_H
#define MINIDB_LOGGER_H

#include <iostream>
#include <string>

// 简单的日志宏
#define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl
#define LOG_INFO(msg) std::cout << "[INFO] " << msg << std::endl
#define LOG_WARNING(msg) std::cout << "[WARNING] " << msg << std::endl
#define LOG_ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl

#endif // MINIDB_LOGGER_H