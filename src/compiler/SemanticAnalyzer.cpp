#include "../include/compiler/SemanticAnalyzer.h"
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <cctype>

using namespace minidb;
using namespace std;

/**
 * 将字符串类型名转换为TypeId枚举
 * 支持多种类型别名和大小写形式
 */
static TypeId stringToTypeId(const string& type_str) {
    // 转换为小写以实现大小写不敏感匹配
    string lower_type;
    transform(type_str.begin(), type_str.end(), back_inserter(lower_type), ::tolower);

    if (lower_type == "int" || lower_type == "integer") {
        return TypeId::INTEGER;
    } else if (lower_type == "string" || lower_type == "varchar") {
        return TypeId::VARCHAR;
    } else {
        return TypeId::INVALID;
    }
}

/**
 * 语义分析入口函数
 * 根据AST节点类型分发到相应的分析函数
 */
void SemanticAnalyzer::analyze(ASTNode* ast) {
    if (!ast) {
        throw SemanticError("AST节点为空，无法进行语义分析");
    }

    // 根据AST节点类型进行分发
    if (auto create_table_ast = dynamic_cast<CreateTableAST*>(ast)) {
        analyzeCreateTable(create_table_ast);
    } else if (auto insert_ast = dynamic_cast<InsertAST*>(ast)) {
        analyzeInsert(insert_ast);
    } else if (auto select_ast = dynamic_cast<SelectAST*>(ast)) {
        analyzeSelect(select_ast);
    } else {
        throw SemanticError("不支持的AST节点类型");
    }
}

// 声明运算符重载（未使用，可根据需要实现）
bool operator==(TypeId lhs, char* str);

/**
 * 分析CREATE TABLE语句
 * 检查：表是否已存在、列名是否重复、列类型是否合法
 */
void SemanticAnalyzer::analyzeCreateTable(CreateTableAST* ast) {
    const string& table_name = ast->tableName;

    // 检查表是否已存在
    if (catalog_manager_.table_exists(table_name)) {
        throw SemanticError("表 '" + table_name + "' 已存在，无法重复创建");
    }

    // 检查列名是否重复
    unordered_map<string, bool> col_name_map;
    for (const auto& col : ast->columns) {
        if (col_name_map.count(col.name)) {
            throw SemanticError("表 '" + table_name + "' 中存在重复列名: " + col.name);
        }
        col_name_map[col.name] = true;

        // 检查列类型是否合法
        TypeId col_type = stringToTypeId(col.type);
        if (col_type == TypeId::INVALID) {
            throw SemanticError("表 '" + table_name + "' 中存在非法列类型: " +
                                col.type + "（仅支持INT/INTEGER/STRING/VARCHAR）");
        }
    }

    // 构建表结构信息
    vector<MyColumn> schema_columns;
    for (const auto& ast_col : ast->columns) {
        TypeId col_type = stringToTypeId(ast_col.type);
        uint32_t col_length = 0;

        // 根据类型设置默认长度
        if (col_type == TypeId::INTEGER) {
            col_length = 4; // 4字节整数
        } else if (col_type == TypeId::VARCHAR) {
            col_length = 255; // 默认255字节字符串
        }

        schema_columns.emplace_back(ast_col.name, col_type, col_length, 0);
    }
    Schema schema(schema_columns);

    // 创建表
    if (!catalog_manager_.create_table(table_name, schema)) {
        throw SemanticError("创建表 '" + table_name + "' 失败");
    }
}

/**
 * 分析INSERT语句
 * 检查：表是否存在、插入值数量与列数量是否匹配、值类型与列类型是否匹配
 */
void SemanticAnalyzer::analyzeInsert(InsertAST* ast) {
    const string& table_name = ast->tableName;
    const vector<string>& values = ast->values;

    // 检查表是否存在
    if (!catalog_manager_.table_exists(table_name)) {
        throw SemanticError("插入失败：表 '" + table_name + "' 不存在");
    }

    // 获取表信息
    const TableInfo* table_info = catalog_manager_.get_table(table_name);
    if (!table_info) {
        throw SemanticError("获取表 '" + table_name + "' 信息失败");
    }
    const Schema& schema = table_info->get_schema();

    // 检查插入值数量与列数量是否匹配
    if (values.size() != schema.get_column_count()) {
        throw SemanticError("插入失败：表 '" + table_name + "' 有 " +
                            to_string(schema.get_column_count()) + " 列，但插入了 " +
                            to_string(values.size()) + " 个值");
    }

    // 检查每个值的类型是否与对应列的类型匹配
    for (size_t i = 0; i < values.size(); ++i) {
        const MyColumn& col = schema.get_column(static_cast<uint32_t>(i));
        const string& value = values[i];

        // 将TypeId转换为字符串类型名，用于类型检查
        string type_str;
        switch (col.type) {
            case TypeId::INTEGER:
                type_str = "INT";
                break;
            case TypeId::VARCHAR:
                type_str = "STRING";
                break;
            default:
                type_str = "UNKNOWN";
                break;
        }

        if (!checkValueMatchType(value, type_str)) {
            throw SemanticError("插入失败：表 '" + table_name + "' 的列 '" + col.name +
                                "' 类型不匹配，值为 '" + value + "'");
        }
    }
}

/**
 * 分析SELECT语句
 * 检查：表是否存在、查询列是否存在、条件表达式是否合法
 */
void SemanticAnalyzer::analyzeSelect(SelectAST* ast) {
    const string& table_name = ast->tableName;
    const vector<string>& columns = ast->columns;

    // 检查表是否存在
    if (!catalog_manager_.table_exists(table_name)) {
        throw SemanticError("查询失败：表 '" + table_name + "' 不存在");
    }

    // 获取表信息
    const TableInfo* table_info = catalog_manager_.get_table(table_name);
    if (!table_info) {
        throw SemanticError("获取表 '" + table_name + "' 信息失败");
    }
    const Schema& schema = table_info->get_schema();

    // 检查查询的列是否存在（通配符*除外）
    for (const string& col_name : columns) {
        if (col_name == "*") continue;

        try {
            schema.get_column_index(col_name);
        } catch (const out_of_range&) {
            throw SemanticError("查询失败：表 '" + table_name + "' 中不存在列 '" + col_name + "'");
        } catch (const runtime_error&) {
            throw SemanticError("查询失败：表 '" + table_name + "' 中不存在列 '" + col_name + "'");
        }
    }

    // 检查WHERE条件（如果存在）
    if (ast->condition.has_value()) {
        const Condition& cond = ast->condition.value();

        try {
            // 检查条件列是否存在
            uint32_t col_idx = schema.get_column_index(cond.column);
            const   MyColumn& cond_col = schema.get_column(col_idx);

            // 将TypeId转换为字符串类型名，用于类型检查
            string type_str;
            switch (cond_col.type) {
                case TypeId::INTEGER:
                    type_str = "INT";
                    break;
                case TypeId::VARCHAR:
                    type_str = "STRING";
                    break;
                default:
                    type_str = "UNKNOWN";
                    break;
            }

            // 检查条件值与列类型是否匹配
            if (!checkValueMatchType(cond.value, type_str)) {
                throw SemanticError("查询失败：条件列 '" + cond.column + "' 类型不匹配，值为 '" + cond.value + "'");
            }
        } catch (const out_of_range&) {
            throw SemanticError("查询失败：表 '" + table_name + "' 中不存在条件列 '" + cond.column + "'");
        } catch (const runtime_error&) {
            throw SemanticError("查询失败：表 '" + table_name + "' 中不存在条件列 '" + cond.column + "'");
        }
    }
}

/**
 * 检查值与类型是否匹配
 * @param value 要检查的值
 * @param type 目标类型（"INT" 或 "STRING"）
 * @return 匹配返回true，否则返回false
 */
bool SemanticAnalyzer::checkValueMatchType(const string& value, const string& type) {
    if (type == "INT") {
        // 检查是否为合法整数
        if (value.empty()) return false;

        size_t start = 0;
        // 允许正负号
        if (value[0] == '+' || value[0] == '-') {
            start = 1;
            // 不能只有符号
            if (value.size() == 1) return false;
        }

        // 检查剩余字符是否都是数字
        return all_of(value.begin() + start, value.end(), ::isdigit);
    } else if (type == "STRING") {
        // 字符串类型总是匹配（可以在这里添加更严格的检查，如引号）
        return true;
    }
    return false;
}