#include "../include/compiler/SemanticAnalyzer.h"
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <cctype>

using namespace minidb;
using namespace std;


//将字符串类型名转换为TypeId枚举，支持多种类型别名和大小写形式
static TypeId stringToTypeId(const string& type_str) {
    // 转换为小写以实现大小写不敏感匹配
    string lower_type;
    transform(type_str.begin(), type_str.end(), back_inserter(lower_type), ::tolower);

    if (lower_type == "int" || lower_type == "integer") {
        return TypeId::INTEGER;
    } else if (lower_type == "string" || lower_type == "varchar") {
        return TypeId::VARCHAR;
    } else if (lower_type == "float") {
        return TypeId::FLOAT;
    } else if (lower_type == "boolean") {
        return TypeId::BOOLEAN;
    } else {
        return TypeId::INVALID;
    }
}


//语义分析入口函数，根据AST节点类型分发到相应的分析函数
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
    } else if (auto delete_ast = dynamic_cast<DeleteAST*>(ast)) {
        analyzeDelete(delete_ast);
    } else {
        throw SemanticError("不支持的AST节点类型");
    }
}

// 声明运算符重载（未使用，可根据需要实现）
bool operator==(TypeId lhs, char* str);

//分析CREATE TABLE语句，表是否已存在、列名是否重复、列类型是否合法
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
                                col.type + "（仅支持INT/INTEGER/STRING/VARCHAR/FLOAT/BOOLEAN）");
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
        } else if (col_type == TypeId::FLOAT) {
            col_length = 4; // 4字节浮点数
        } else if (col_type == TypeId::BOOLEAN) {
            col_length = 1; // 1字节布尔值
        }

        schema_columns.emplace_back(ast_col.name, col_type, col_length, 0);
    }
    Schema schema(schema_columns);

    // 创建表
    if (!catalog_manager_.create_table(table_name, schema)) {
        throw SemanticError("创建表 '" + table_name + "' 失败");
    }
}

//分析INSERT语句
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
            case TypeId::FLOAT:
                type_str = "FLOAT";
                break;
            case TypeId::BOOLEAN:
                type_str = "BOOLEAN";
                break;
            default:
                type_str = "UNKNOWN";
                break;
        }

        if (!checkValueMatchType(value, type_str)) {
            throw SemanticError("插入失败：表 '" + table_name + "' 的列 '" + col.name +
                                "' 类型不匹配，期望类型 " + type_str + "，值为 '" + value + "'");
        }
    }
}

//分析SELECT语句
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
        analyzeCondition(ast->condition.value(), schema, table_name);
    }
}

//分析DELETE语句
void SemanticAnalyzer::analyzeDelete(DeleteAST* ast) {
    const string& table_name = ast->tableName;

    // 检查表是否存在
    if (!catalog_manager_.table_exists(table_name)) {
        throw SemanticError("删除失败：表 '" + table_name + "' 不存在");
    }

    // 获取表信息
    const TableInfo* table_info = catalog_manager_.get_table(table_name);
    if (!table_info) {
        throw SemanticError("获取表 '" + table_name + "' 信息失败");
    }
    const Schema& schema = table_info->get_schema();

    // 检查WHERE条件（如果存在）
    if (ast->condition.has_value()) {
        analyzeCondition(ast->condition.value(), schema, table_name);
    } else {
        // 警告：无WHERE条件的DELETE将删除所有记录
        // 可以根据需要决定是否允许这种操作
        // 这里我们允许，但可以添加日志记录
    }
}

//分析WHERE条件表达式
void SemanticAnalyzer::analyzeCondition(const Condition& cond, const Schema& schema, const string& table_name) {
    try {
        // 检查条件列是否存在
        uint32_t col_idx = schema.get_column_index(cond.column);
        const MyColumn& cond_col = schema.get_column(col_idx);

        // 检查运算符是否合法
        if (!isValidOperator(cond.op)) {
            throw SemanticError("条件检查失败：不支持的运算符 '" + cond.op + "'");
        }

        // 将TypeId转换为字符串类型名，用于类型检查
        string type_str;
        switch (cond_col.type) {
            case TypeId::INTEGER:
                type_str = "INT";
                break;
            case TypeId::VARCHAR:
                type_str = "STRING";
                break;
            case TypeId::FLOAT:
                type_str = "FLOAT";
                break;
            case TypeId::BOOLEAN:
                type_str = "BOOLEAN";
                break;
            default:
                type_str = "UNKNOWN";
                break;
        }

        // 检查条件值与列类型是否匹配
        if (!checkValueMatchType(cond.value, type_str)) {
            throw SemanticError("条件检查失败：条件列 '" + cond.column + "' 类型不匹配，期望类型 " +
                                type_str + "，值为 '" + cond.value + "'");
        }

        // 检查运算符与数据类型的兼容性
        if (!isOperatorCompatibleWithType(cond.op, cond_col.type)) {
            throw SemanticError("条件检查失败：运算符 '" + cond.op + "' 不适用于类型 " + type_str);
        }

    } catch (const out_of_range&) {
        throw SemanticError("条件检查失败：表 '" + table_name + "' 中不存在条件列 '" + cond.column + "'");
    } catch (const runtime_error&) {
        throw SemanticError("条件检查失败：表 '" + table_name + "' 中不存在条件列 '" + cond.column + "'");
    }
}


//检查运算符是否合法
bool SemanticAnalyzer::isValidOperator(const string& op) {
    static const unordered_set<string> valid_operators = {
        "=", "!=", "<>", "<", "<=", ">", ">=", "LIKE", "NOT LIKE"
    };
    return valid_operators.count(op) > 0;
}

//检查运算符与数据类型的兼容性
bool SemanticAnalyzer::isOperatorCompatibleWithType(const string& op, TypeId type) {
    // 所有类型都支持等于和不等于比较
    if (op == "=" || op == "!=" || op == "<>") {
        return true;
    }

    // 大小比较运算符只适用于数值类型
    if (op == "<" || op == "<=" || op == ">" || op == ">=") {
        return type == TypeId::INTEGER || type == TypeId::FLOAT;
    }

    // LIKE运算符只适用于字符串类型
    if (op == "LIKE" || op == "NOT LIKE") {
        return type == TypeId::VARCHAR;
    }

    return false;
}

//检查值与类型是否匹配
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

    } else if (type == "FLOAT") {
        // 检查是否为合法浮点数
        if (value.empty()) return false;

        try {
            size_t pos;
            stof(value, &pos);
            // 确保整个字符串都被解析
            return pos == value.length();
        } catch (const invalid_argument&) {
            return false;
        } catch (const out_of_range&) {
            return false;
        }

    } else if (type == "BOOLEAN") {
        // 检查是否为合法布尔值
        string lower_value;
        transform(value.begin(), value.end(), back_inserter(lower_value), ::tolower);

        // 移除可能的引号
        if (lower_value.size() >= 2 &&
            ((lower_value.front() == '"' && lower_value.back() == '"') ||
             (lower_value.front() == '\'' && lower_value.back() == '\''))) {
            lower_value = lower_value.substr(1, lower_value.length() - 2);
        }

        return lower_value == "true" || lower_value == "false" ||
               lower_value == "1" || lower_value == "0";

    } else if (type == "STRING") {
        // 字符串类型总是匹配（可以在这里添加更严格的检查，如引号、长度限制等）
        return true;
    }

    return false;
}

