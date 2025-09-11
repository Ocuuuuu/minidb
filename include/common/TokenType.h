//
// Created by tang_ on 2025/9/10.
//

#ifndef MINIDB_TOKENTYPE_H
#define MINIDB_TOKENTYPE_H

// 单词符号类型枚举，对应种别码
enum class TokenType {
    KEYWORD = 1,      // 关键字，种别码 1
    IDENTIFIER = 2,   // 标识符，种别码 2
    CONSTANT = 3,     // 常数，种别码 3
    OPERATOR = 4,     // 运算符，种别码 4
    DELIMITER = 5,    // 界符，种别码 5
    ERROR = 6,        // 错误 Token，用于识别失败的情况
    EOF_TOKEN = 7     // 文件结束 Token，标识 SQL 字符串处理完毕
};

#endif //MINIDB_TOKENTYPE_H