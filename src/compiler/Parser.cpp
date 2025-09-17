/*
 * Parser.cpp - 基于LL(1)文法的SQL语法解析器
 * 支持SQL语句类型：
 * 1. SELECT（带WHERE子句、多列查询、通配符*）
 * 2. CREATE TABLE（多列定义，支持STRING/INT类型）
 * 3. INSERT（多值插入，兼容带引号/无引号字符串常量）
 * 4. DELETE（删除语句）
 */

#include "../../include/compiler/Parser.h"
#include "../../include/compiler/AST.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <memory>

using namespace std;

// 将TokenType转换为字符串，用于调试输出
static std::string tokenTypeToString(TokenType type) {
    switch (type) {
    case TokenType::KEYWORD: return "KEYWORD";
    case TokenType::IDENTIFIER: return "IDENTIFIER";
    case TokenType::CONSTANT: return "CONSTANT";
    case TokenType::OPERATOR: return "OPERATOR";
    case TokenType::DELIMITER: return "DELIMITER";
    case TokenType::ERROR: return "ERROR";
    case TokenType::EOF_TOKEN: return "EOF";
    default: return "UNKNOWN";
    }
}

// 将Token列表转换为字符串表示，用于调试和测试
static std::string tokensToString(const std::vector<Token>& tokens) {
    if (tokens.empty()) return "Empty token vector";

    std::string result;
    for (const auto& token : tokens) {
        result += "[" + tokenTypeToString(token.type) + ":" + token.value + "]";
    }
    return result;
}

// match函数完整实现
void Parser::match(const string& expectedValue) {
    // 生成当前Token的标识
    string currentTokenKey;
    if (currentToken.type == TokenType::KEYWORD) {
        string keywordValue = currentToken.value;
        transform(keywordValue.begin(), keywordValue.end(), keywordValue.begin(), ::toupper);
        currentTokenKey = "KEYWORD(" + keywordValue + ")";
    } else if (currentToken.type == TokenType::IDENTIFIER) {
        currentTokenKey = "IDENTIFIER";
        // 兼容处理：无引号字符串按CONSTANT匹配（针对INSERT值列表）
        if (expectedValue == "CONSTANT") {
            currentTokenKey = "CONSTANT";
        }
    } else if (currentToken.type == TokenType::CONSTANT) {
        currentTokenKey = "CONSTANT";
    } else if (currentToken.type == TokenType::OPERATOR) {
        if (currentToken.value == "*") {
            currentTokenKey = STAR;
        } else {
            currentTokenKey = "OPERATOR";
        }
    } else if (currentToken.type == TokenType::DELIMITER) {
        if (currentToken.value == "(") {
            currentTokenKey = LPAREN;
        } else if (currentToken.value == ")") {
            currentTokenKey = RPAREN;
        } else if (currentToken.value == ",") {
            currentTokenKey = COMMA;
        } else if (currentToken.value == ";") {
            currentTokenKey = SEMICOLON;
        } else if (currentToken.value == "*") {
            currentTokenKey = STAR;
        } else {
            currentTokenKey = "DELIMITER(" + currentToken.value + ")";
        }
    } else if (currentToken.type == TokenType::EOF_TOKEN) {
        currentTokenKey = "EOF";
    } else {
        throw runtime_error("未知Token类型: " + to_string(static_cast<int>(currentToken.type)));
    }

    // 检查类型关键字匹配
    bool isTypeMatch = false;
    if (expectedValue == "KEYWORD(TYPE)") {
        isTypeMatch = (currentTokenKey == "KEYWORD(INT)" ||
                      currentTokenKey == "KEYWORD(INTEGER)" ||
                      currentTokenKey == "KEYWORD(STRING)" ||
                      currentTokenKey == "KEYWORD(VARCHAR)" ||
                      currentTokenKey == "KEYWORD(FLOAT)" ||
                      currentTokenKey == "KEYWORD(BOOLEAN)");
    } else if (expectedValue == "KEYWORD(INT)" || expectedValue == "KEYWORD(STRING)" ||
               expectedValue == "KEYWORD(FLOAT)" || expectedValue == "KEYWORD(BOOLEAN)" ||
               expectedValue == "KEYWORD(INTEGER)" || expectedValue == "KEYWORD(VARCHAR)") {
        isTypeMatch = (currentTokenKey == expectedValue);
    }

    // 检查匹配
    if (!isTypeMatch && currentTokenKey != expectedValue) {
        stringstream errMsg;
        errMsg << "语法错误：预期'" << expectedValue << "', 实际'" << currentTokenKey
               << "'（Token值: '" << currentToken.value << "', 行: " << currentToken.line
               << ", 列: " << currentToken.column << "）";
        throw runtime_error(errMsg.str());
    }

    // 推进Token流
    tokenPos++;
    if (tokenPos < tokens.size()) {
        currentToken = tokens[tokenPos];
    } else {
        currentToken = Token(TokenType::EOF_TOKEN, "", -1, -1);
    }
}

ASTNodePtr Parser::parse() {
    try {
        if (tokens.empty()) {
            throw std::runtime_error("输入为空，无法生成 AST");
        }
        // 调用语法分析逻辑
        return parseProg();
    } catch (const std::exception& e) {
        std::cerr << "Parser 错误: " << e.what() << std::endl;
        throw std::runtime_error("Parser error: Failed to generate AST");
    }
}

Parser::Parser(const std::vector<Token>& tokenList)
    : lexer(nullptr),  // lexer设为nullptr，因为不需要词法分析器
      currentToken(TokenType::ERROR, "", -1, -1),
      tokenPos(0) {

    initPredictTable();
    //语法栈初始化：栈底为EOF（结束符），栈顶为开始符号Prog
    symStack.push("EOF");
    symStack.push("Prog");

    // 直接使用传入的token序列
    tokens = tokenList;
    if (tokens.empty()) {
        throw runtime_error("Token序列为空，无法进行语法分析");
    }

    // std::cout << "Cached Tokens: " << tokensToString(tokens) << std::endl;

    //初始化当前Token
    currentToken = tokens[tokenPos];
}

// 原构造函数：接受Lexer引用（修改为使用指针）
Parser::Parser(Lexer& l)
    : lexer(&l),  // 改为指针赋值
      currentToken(TokenType::ERROR, "", -1, -1),
      tokenPos(0) {

    initPredictTable();
    //语法栈初始化：栈底为EOF（结束符），栈顶为开始符号Prog
    symStack.push("EOF");
    symStack.push("Prog");
    //从词法分析器获取所有Token并缓存
    tokens = lexer->getAllTokens();  // 使用->而不是.
    if (tokens.empty()) {
        throw runtime_error("Lexer returned empty token stream");
    }

    std::cout << "Cached Tokens: " << tokensToString(tokens) << std::endl;

    //初始化当前Token
    currentToken = tokens[tokenPos];
}

//预测表初始化（LL(1)）
void Parser::initPredictTable() {
    // 1. 程序入口规则：Prog → Stmt EOF（一条SQL语句 + 结束符）
    predictTable["Prog|KEYWORD(SELECT)"] = {"Stmt", "EOF"};
    predictTable["Prog|KEYWORD(CREATE)"] = {"Stmt", "EOF"};
    predictTable["Prog|KEYWORD(INSERT)"] = {"Stmt", "EOF"};
    predictTable["Prog|KEYWORD(DELETE)"] = {"Stmt", "EOF"};
    predictTable["Prog|EOF"] = {}; // 允许空输入

    // 2. 语句规则：Stmt → CreateTable | Insert | Select | Delete（分发到具体语句）
    predictTable["Stmt|KEYWORD(CREATE)"] = {"CreateTable"};
    predictTable["Stmt|KEYWORD(INSERT)"] = {"Insert"};
    predictTable["Stmt|KEYWORD(SELECT)"] = {"Select"};
    predictTable["Stmt|KEYWORD(DELETE)"] = {"Delete"};

    // 12. DELETE语句规则：Delete → DELETE FROM 表名 WHERE子句 ;
    predictTable["Delete|KEYWORD(DELETE)"] = {
        "KEYWORD(DELETE)", "KEYWORD(FROM)", "IDENTIFIER", "WhereClause", SEMICOLON
    };

    // 3. CREATE TABLE语句规则：CreateTable → CREATE TABLE 表名 ( 列列表 ) ;
    predictTable["CreateTable|KEYWORD(CREATE)"] = {
        "KEYWORD(CREATE)", "KEYWORD(TABLE)", "IDENTIFIER",
        LPAREN, "ColumnList", RPAREN, SEMICOLON
    };

    // 4. 列列表规则（消除左递归）
    predictTable["ColumnList|IDENTIFIER"] = {"Column", "ColumnList'"};  // 初始列（由列名触发）
    predictTable["ColumnList'|,"] = {COMMA, "Column", "ColumnList'"};   // 逗号后接新列
    predictTable["ColumnList'|)"] = {};                                 // 右括号结束（空产生式）

    // 5. 列定义规则：Column → 列名 类型关键字（扩展支持VARCHAR/INTEGER）
    predictTable["Column|IDENTIFIER"] = {"IDENTIFIER", "KEYWORD(TYPE)"};
    // 扩展类型关键字匹配：支持INT/INTEGER、STRING/VARCHAR（兼容大小写）
    predictTable["KEYWORD(TYPE)|KEYWORD(INT)"] = {"KEYWORD(INT)"};
    predictTable["KEYWORD(TYPE)|KEYWORD(INTEGER)"] = {"KEYWORD(INTEGER)"};
    predictTable["KEYWORD(TYPE)|KEYWORD(STRING)"] = {"KEYWORD(STRING)"};
    predictTable["KEYWORD(TYPE)|KEYWORD(VARCHAR)"] = {"KEYWORD(VARCHAR)"};
    predictTable["KEYWORD(TYPE)|KEYWORD(FLOAT)"] = {"KEYWORD(FLOAT)"};
    predictTable["KEYWORD(TYPE)|KEYWORD(BOOLEAN)"] = {"KEYWORD(BOOLEAN)"};

    // 6. INSERT语句规则：Insert → INSERT INTO 表名 VALUES ( 值列表 ) ;
    predictTable["Insert|KEYWORD(INSERT)"] = {
        "KEYWORD(INSERT)", "KEYWORD(INTO)", "IDENTIFIER",
        "KEYWORD(VALUES)", LPAREN, "ValueList", RPAREN, SEMICOLON
    };

    // 7. 值列表规则（消除左递归，兼容无引号字符串）
    predictTable["ValueList|CONSTANT"] = {"CONSTANT", "ValueList'"};    // 正常常量（带引号字符串/数字）
    predictTable["ValueList|IDENTIFIER"] = {"CONSTANT", "ValueList'"};  // 兼容无引号字符串（兜底）
    predictTable["ValueList'|,"] = {COMMA, "CONSTANT", "ValueList'"};   // 逗号后接新值
    predictTable["ValueList'|)"] = {};                                 // 右括号结束（空产生式）

    // 8. SELECT语句规则：Select → SELECT 查询列 FROM 表名 WHERE子句 ;
    predictTable["Select|KEYWORD(SELECT)"] = {
        "KEYWORD(SELECT)", "SelectColumns", "KEYWORD(FROM)",
        "IDENTIFIER", "WhereClause", SEMICOLON
    };

    // 9. 查询列列表规则（消除左递归，支持通配符）
    predictTable["SelectColumns|STAR"] = {STAR};                           // 通配符*（查询所有列）
    predictTable["SelectColumns|IDENTIFIER"] = {"IDENTIFIER", "SelectColumns'"};  // 具体列名
    predictTable["SelectColumns'|,"] = {COMMA, "IDENTIFIER", "SelectColumns'"};   // 逗号后接新查询列
    predictTable["SelectColumns'|KEYWORD(FROM)"] = {};                             // 遇到FROM结束
    predictTable["SelectColumns'|;"] = {};                               // 遇到分号结束（无FROM场景）

    // 10. WHERE子句规则（可选）
    predictTable["WhereClause|KEYWORD(WHERE)"] = {"KEYWORD(WHERE)", "Condition"};  // 有WHERE时解析条件
    predictTable["WhereClause|;"] = {};                                 // 无WHERE时直接结束

    // 11. 条件表达式规则：Condition → 列名 运算符 值（IDENTIFIER → 列名，OPERATOR → 运算符，CONSTANT → 值）
    predictTable["Condition|IDENTIFIER"] = {"IDENTIFIER", "OPERATOR", "CONSTANT"};
}


//核心解析函数
// 解析非终结符：根据当前Token和预测表，压入对应产生式符号
void Parser::parseNonTerminal(const string& nonTerminal) {
    //1. 生成当前Token的标识（用于查询预测表，如 "KEYWORD(SELECT)"、"IDENTIFIER"）
    string currentTokenKey;
    if (currentToken.type == TokenType::KEYWORD) {
        // 将关键字转换为大写，确保大小写兼容
        string keywordValue = currentToken.value;
        transform(keywordValue.begin(), keywordValue.end(), keywordValue.begin(), ::toupper);
        currentTokenKey = "KEYWORD(" + keywordValue + ")";
    } else if (currentToken.type == TokenType::IDENTIFIER) {
        currentTokenKey = "IDENTIFIER";
    } else if (currentToken.type == TokenType::CONSTANT) {
        currentTokenKey = "CONSTANT";
    } else if (currentToken.type == TokenType::OPERATOR) {
        // 特殊处理通配符*，确保它被正确识别为STAR而不是OPERATOR
        if (currentToken.value == "*") {
            currentTokenKey = STAR;
        } else {
            currentTokenKey = "OPERATOR";
        }
    } else if (currentToken.type == TokenType::DELIMITER) {
        currentTokenKey = currentToken.value;
    } else if (currentToken.type == TokenType::EOF_TOKEN) {
        currentTokenKey = "EOF";
    } else {
        throw runtime_error("未知Token类型: " + to_string(static_cast<int>(currentToken.type)));
    }

    //2. 构建预测表查询键（格式："非终结符|当前Token标识"）
    string tableKey = nonTerminal + "|" + currentTokenKey;
    if (predictTable.find(tableKey) == predictTable.end()) {
        stringstream errMsg;
        errMsg << "无匹配产生式: " << tableKey
               << "（非终结符: " << nonTerminal << ", Token: " << currentToken.value << "）";
        throw runtime_error(errMsg.str());
    }
    vector<string> production = predictTable[tableKey];

    //3. 验证语法栈顶是否为当前非终结符（确保解析逻辑正确性）
    if (symStack.empty() || symStack.top() != nonTerminal) {
        stringstream errMsg;
        errMsg << "栈顶异常：解析'" << nonTerminal << "'时，预期'" << nonTerminal
               << "', 实际'" << (symStack.empty() ? "空" : symStack.top()) << "'";
        throw runtime_error(errMsg.str());
    }

    //4. 弹出非终结符，逆序压入产生式符号（栈后进先出，保证解析顺序正确）
    symStack.pop();

    for (auto it = production.rbegin(); it != production.rend(); ++it) {
        if (!it->empty()) {
            symStack.push(*it);
        }
    }
}


// 匹配终结符：检查当前Token是否与预期一致，匹配成功则推进Token流
// void Parser::match(const string& expectedValue) {
//     //1. 生成当前Token的标识（与预期值格式对齐）
//     string currentTokenKey;
//     if (currentToken.type == TokenType::KEYWORD) {
//         string keywordValue = currentToken.value;
//         transform(keywordValue.begin(), keywordValue.end(), keywordValue.begin(), ::toupper);
//         currentTokenKey = "KEYWORD(" + keywordValue + ")";
//     } else if (currentToken.type == TokenType::IDENTIFIER) {
//         currentTokenKey = "IDENTIFIER";
//         //兼容处理1：无引号字符串按CONSTANT匹配（针对INSERT值列表）
//         if (expectedValue == "CONSTANT") {
//             currentTokenKey = "CONSTANT";
//         }
//     } else if (currentToken.type == TokenType::CONSTANT) {
//         currentTokenKey = "CONSTANT";
//     } else if (currentToken.type == TokenType::OPERATOR) {
//         // 特殊处理通配符*，确保它被正确识别为STAR而不是OPERATOR
//         if (currentToken.value == "*") {
//             currentTokenKey = STAR;
//         } else {
//             currentTokenKey = "OPERATOR";
//         }
//     } else if (currentToken.type == TokenType::DELIMITER) {
//         //映射分隔符到统一常量（与预测表符号一致）
//         if (currentToken.value == "(") {
//             currentTokenKey = LPAREN;
//         } else if (currentToken.value == ")") {
//             currentTokenKey = RPAREN;
//         } else if (currentToken.value == ",") {
//             currentTokenKey = COMMA;
//         } else if (currentToken.value == ";") {
//             currentTokenKey = SEMICOLON;
//         } else if (currentToken.value == "*") {
//             currentTokenKey = STAR;
//         } else {
//             currentTokenKey = "DELIMITER(" + currentToken.value + ")";
//         }
//     } else if (currentToken.type == TokenType::EOF_TOKEN) {
//         currentTokenKey = "EOF";
//     } else {
//         throw runtime_error("未知Token类型: " + to_string(static_cast<int>(currentToken.type)));
//     }
//
//     //2. 修改isTypeMatch条件，支持类型关键字的大小写不敏感匹配
//     bool isTypeMatch = false;
//
//     // 检查是否是类型关键字匹配
//     if (expectedValue == "KEYWORD(TYPE)") {
//         isTypeMatch = (currentTokenKey == "KEYWORD(INT)" ||
//                       currentTokenKey == "KEYWORD(INTEGER)" ||
//                       currentTokenKey == "KEYWORD(STRING)" ||
//                       currentTokenKey == "KEYWORD(VARCHAR)" ||
//                       currentTokenKey == "KEYWORD(FLOAT)" ||
//                       currentTokenKey == "KEYWORD(BOOLEAN)");
//     }
//
//     // 额外支持直接的类型关键字匹配（如"KEYWORD(INT)"）
//     else if (expectedValue == "KEYWORD(INT)" || expectedValue == "KEYWORD(STRING)" ||
//              expectedValue == "KEYWORD(FLOAT)" || expectedValue == "KEYWORD(BOOLEAN)" ||
//              expectedValue == "KEYWORD(INTEGER)" || expectedValue == "KEYWORD(VARCHAR)") {
//         isTypeMatch = (currentTokenKey == expectedValue);
//     }
//
//     if (!isTypeMatch && currentTokenKey != expectedValue) {
//         stringstream errMsg;
//         errMsg << "语法错误：预期'" << expectedValue << "', 实际'" << currentTokenKey
//                << "'（行: " << currentToken.line << ", 列: " << currentToken.column << "）";
//         throw runtime_error(errMsg.str());
//     }
//
//     //3. 推进Token流（无论是否匹配成功，异常已提前抛出）
//     tokenPos++;
//     currentToken = (tokenPos < tokens.size()) ? tokens[tokenPos] : Token(TokenType::EOF_TOKEN, "", -1, -1);
// }

//语句解析函数
/**
 * 解析程序入口：Prog → Stmt EOF
 * @return AST根节点（对应一条SQL语句的AST）
 */
unique_ptr<ASTNode> Parser::parseProg() {
    parseNonTerminal("Prog");
    unique_ptr<ASTNode> stmtAST = parseStmt();

    //清理Stmt解析后可能残留的非终结符（确保栈顶为EOF）
    while (!symStack.empty()) {
        string top = symStack.top();
        if (top != "EOF") {
            //仅弹出非EOF的残留符号（避免误删EOF）
            symStack.pop();
        } else {
            break;
        }
    }

    //匹配结束符EOF（确保语句完整）
    if (!symStack.empty() && symStack.top() == "EOF") {
        match("EOF");
        symStack.pop();
    } else {
        throw runtime_error("解析结束后栈顶应为'EOF'");
    }

    //重新压入EOF，为下一条语句解析做准备（若有）
    symStack.push("EOF");
    return stmtAST;
}

/**
 * 解析语句分发：Stmt → CreateTable | Insert | Select
 * @return 具体语句的AST节点（CreateTableAST/InsertAST/SelectAST）
 */
unique_ptr<ASTNode> Parser::parseStmt() {
    parseNonTerminal("Stmt");
    unique_ptr<ASTNode> resultAST;

    // 根据语法栈顶符号，分发到对应语句的解析函数
    if (!symStack.empty() && symStack.top() == "CreateTable") {
        resultAST = parseCreateTable();
    } else if (!symStack.empty() && symStack.top() == "Insert") {
        resultAST = parseInsert();
    } else if (!symStack.empty() && symStack.top() == "Select") {
        resultAST = parseSelect();
    } else if (!symStack.empty() && symStack.top() == "Delete") {
        resultAST = parseDelete();
    } else {
        throw runtime_error("未知语句类型：栈顶为'" + (symStack.empty() ? "空" : symStack.top()) + "'");
    }
    return resultAST;
}

/**
 * 解析SELECT语句：生成SelectAST节点
 * 支持：多列查询、通配符*、WHERE子句条件
 * @return SelectAST节点（包含查询列、表名、WHERE条件）
 */
unique_ptr<SelectAST> Parser::parseSelect() {
    parseNonTerminal("Select");

    //1. 匹配SELECT关键字
    match("KEYWORD(SELECT)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(SELECT)") {
        symStack.pop();
    }

    //2. 解析查询列列表（如 "name, age" 或 "*"）
    vector<string> selectCols = parseSelectColumns();

    //3. 匹配FROM关键字（确保语法结构正确）
    if (symStack.empty() || symStack.top() != "KEYWORD(FROM)") {
        // 清理栈中剩余的SelectColumns'符号
        while (!symStack.empty() && symStack.top() == "SelectColumns'") {
            symStack.pop();
        }
        if (symStack.empty() || symStack.top() != "KEYWORD(FROM)") {
            throw runtime_error("SELECT语句语法错误：预期'FROM'关键字");
        }
    }
    match("KEYWORD(FROM)");
    symStack.pop();

    //4. 解析查询表名
    string tableName = currentToken.value;
    match("IDENTIFIER");
    if (!symStack.empty() && symStack.top() == "IDENTIFIER") {
        symStack.pop();
    }

    //5. 解析WHERE子句（可选，无WHERE时返回空）
    optional<Condition> condition = parseWhereClause();

    //6. 匹配语句结束分号
    match("SEMICOLON");
    if (!symStack.empty() && symStack.top() == "SEMICOLON") {
        symStack.pop();
    }

    //构建SelectAST并返回
    auto ast = make_unique<SelectAST>();
    ast->columns = selectCols;
    ast->tableName = tableName;
    ast->condition = condition;
    return ast;
}

/**
 * 解析INSERT语句：生成InsertAST节点
 * 支持：多值插入，兼容带引号/无引号字符串常量
 * @return InsertAST节点（包含目标表名、插入值列表）
 */
unique_ptr<InsertAST> Parser::parseInsert() {
    parseNonTerminal("Insert");

    // 匹配INSERT关键字
    match("KEYWORD(INSERT)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(INSERT)") {
        symStack.pop();
    }

    // 匹配INTO关键字
    match("KEYWORD(INTO)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(INTO)") {
        symStack.pop();
    }

    // 解析目标表名
    string tableName = currentToken.value;
    match("IDENTIFIER");
    if (!symStack.empty() && symStack.top() == "IDENTIFIER") {
        symStack.pop();
    }

    // 匹配VALUES关键字
    match("KEYWORD(VALUES)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(VALUES)") {
        symStack.pop();
    }

    // 匹配左括号（值列表开始）
    match(LPAREN);
    if (!symStack.empty() && symStack.top() == LPAREN) {
        symStack.pop();
    }

    // 解析插入值列表
    vector<string> values = parseValueList();

    // 匹配右括号（值列表结束）
    match(RPAREN);
    if (!symStack.empty() && symStack.top() == RPAREN) {
        symStack.pop();
    }

    // 匹配语句结束分号
    match("SEMICOLON");
    if (!symStack.empty() && symStack.top() == "SEMICOLON") {
        symStack.pop();
    }

    // 构建InsertAST并返回
    auto ast = make_unique<InsertAST>();
    ast->tableName = tableName;
    ast->values = values;
    return ast;
}

/**
 * 解析CREATE TABLE语句：生成CreateTableAST节点
 * 支持多列定义，列类型为STRING/INT
 * @return CreateTableAST节点（包含表名、列列表）
 */
unique_ptr<CreateTableAST> Parser::parseCreateTable() {
    parseNonTerminal("CreateTable");

    // 匹配CREATE关键字
    match("KEYWORD(CREATE)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(CREATE)") {
        symStack.pop();
    }

    // 匹配TABLE关键字
    match("KEYWORD(TABLE)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(TABLE)") {
        symStack.pop();
    }

    // 获取表名
    string tableName = currentToken.value;
    match("IDENTIFIER");
    if (!symStack.empty() && symStack.top() == "IDENTIFIER") {
        symStack.pop();
    }

    // 匹配左括号
    match(LPAREN);
    if (!symStack.empty() && symStack.top() == LPAREN) {
        symStack.pop();
    }

    // 解析列列表
    vector<Column> columns = parseColumnList();

    // 匹配右括号
    match(RPAREN);
    if (!symStack.empty() && symStack.top() == RPAREN) {
        symStack.pop();
    }

    // 匹配分号
    match("SEMICOLON");
    if (!symStack.empty() && symStack.top() == "SEMICOLON") {
        symStack.pop();
    }

    // 创建AST节点
    auto ast = make_unique<CreateTableAST>();
    ast->tableName = tableName;
    ast->columns = columns;
    return ast;
}

//列表解析函数
/**
 * 解析查询列列表（SELECT用）：如 "name, age" 或 "*"
 * @return 查询列名称列表（如 ["name", "age"] 或 ["*"]）
 */
vector<string> Parser::parseSelectColumns() {
    parseNonTerminal("SelectColumns");
    vector<string> columns;
    bool parsingComplete = false;

    while (!symStack.empty() && !parsingComplete) {
        // 遇到FROM关键字时，结束查询列解析
        if (currentToken.type == TokenType::KEYWORD &&
            (currentToken.value == "FROM" || currentToken.value == "from")) {
            parsingComplete = true;
            break;
        }

        string stackTop = symStack.top();
        if (stackTop == STAR) {
            // 匹配通配符*（查询所有列）
            columns.push_back("*");
            match(STAR);
            symStack.pop();
        } else if (stackTop == "IDENTIFIER") {
            // 匹配具体列名（如 "name"、"age"）
            columns.push_back(currentToken.value);
            match("IDENTIFIER");
            symStack.pop();
        } else if (stackTop == "SelectColumns'") {
            // 处理递归部分（逗号分隔的后续列）
            symStack.pop();
            parseSelectColumnsPrime();
            if (!symStack.empty() && symStack.top() == COMMA) {
                match(COMMA);
                symStack.pop();
            }
        } else {
            // 其他符号（如分号），结束解析
            parsingComplete = true;
        }
    }

    // 清理栈中剩余的SelectColumns'符号
    while (!symStack.empty() && symStack.top() == "SelectColumns'") {
        symStack.pop();
    }
    return columns;
}

/**
 * 解析值列表（INSERT用）：如 "'Alice', 20"
 * @return 插入值字符串列表（如 ["'Alice'", "20"]）
 */
vector<string> Parser::parseValueList() {
    parseNonTerminal("ValueList");
    vector<string> values;

    while (!symStack.empty()) {
        string stackTop = symStack.top();

        if (stackTop == "CONSTANT") {
            // 在match之前先保存当前Token的值，因为match会推进token流
            string value = currentToken.value;

            // 确保字符串常量保留引号
            // 检查是否是字符串常量（词法分析器应该已经加上引号）
            bool isString = !value.empty() && ((value.front() == '"' && value.back() == '"') ||
                                             (value.front() == '\'' && value.back() == '\''));

            // 如果不是已带引号的字符串，但词法分析器识别为CONSTANT，需要判断是否添加引号
            if (!isString && currentToken.type == TokenType::CONSTANT) {
                // 检查是否为纯数字（包括小数）
                bool isNumeric = true;
                bool hasDot = false;
                for (size_t i = 0; i < value.length(); ++i) {
                    char c = value[i];
                    if (c == '.' && !hasDot) {
                        hasDot = true;
                    } else if (c == '-' && i == 0) {
                        // 负号在开头是允许的
                    } else if (!isdigit(c)) {
                        isNumeric = false;
                        break;
                    }
                }

                // 如果不是数字，则认为是字符串常量，需要添加引号
                if (!isNumeric) {
                    value = "'" + value + "'";
                }
                // 如果是数字，保持原样不添加引号
            }

            values.push_back(value);

            match("CONSTANT");
            if (!symStack.empty() && symStack.top() == "CONSTANT") {
                symStack.pop();
            }
        }
        else if (stackTop == "ValueList'") {
            parseNonTerminal("ValueList'");

            // 检查是否有逗号分隔的更多值
            if (!symStack.empty() && symStack.top() == COMMA) {
                match(COMMA);
                symStack.pop();
            } else {
                // 没有更多值，结束解析
                break;
            }
        }
        else {
            // 遇到其他符号，结束解析
            break;
        }
    }

    return values;
}

/**
 * 解析列列表（CREATE TABLE用）：如 "name STRING, age INT"
 * @return 列信息列表（包含每个列的名称和类型）
 */
vector<Column> Parser::parseColumnList() {
    parseNonTerminal("ColumnList");
    vector<Column> columns;

    while (!symStack.empty()) {
        string stackTop = symStack.top();

        if (stackTop == "Column") {
            // 解析列定义
            Column col;

            // 弹出Column，解析其产生式
            symStack.pop();

            // 获取列名 - 当前Token应该是IDENTIFIER类型
            if (currentToken.type == TokenType::IDENTIFIER) {
                col.name = currentToken.value;
                match("IDENTIFIER");
                if (!symStack.empty() && symStack.top() == "IDENTIFIER") {
                    symStack.pop();
                }
            }

            // 获取列类型 - 当前Token应该是类型关键字
            if (currentToken.type == TokenType::KEYWORD) {
                // 保存类型值（在match之前）
                string typeValue = currentToken.value;

                // 将值转换为大写进行比较，确保大小写不敏感
                string upperTypeValue = typeValue;
                transform(upperTypeValue.begin(), upperTypeValue.end(), upperTypeValue.begin(), ::toupper);

                // 使用大写形式的类型
                col.type = upperTypeValue;

                // 匹配类型关键字
                match("KEYWORD(TYPE)");

                if (!symStack.empty() && symStack.top() == "KEYWORD(TYPE)") {
                    symStack.pop();
                }
            }

            columns.push_back(col);
        }
        else if (stackTop == "ColumnList'") {
            // 处理递归部分（逗号分隔的后续列）
            symStack.pop();
            parseColumnListPrime();

            // 检查并匹配逗号
            if (!symStack.empty() && symStack.top() == COMMA) {
                match(COMMA);
                symStack.pop();
            }
        }
        else if (stackTop == "ColumnList") {
            // 弹出已处理的ColumnList
            symStack.pop();
        }
        else if (stackTop == RPAREN) {
            // 遇到右括号，结束解析
            break;
        }
        else {
            // 遇到其他符号，结束解析
            break;
        }
    }

    return columns;
}

//辅助解析函数（递归部分
/**
 * 解析查询列列表递归部分：SelectColumns' → , IDENTIFIER SelectColumns' | ε
 */
void Parser::parseSelectColumnsPrime() {
    // 生成当前Token的标识（用于查询预测表）
    string currentTokenKey;
    if (currentToken.type == TokenType::DELIMITER && currentToken.value == ",") {
        currentTokenKey = ",";
    } else if (currentToken.type == TokenType::KEYWORD &&
              (currentToken.value == "FROM" || currentToken.value == "from")) {
        currentTokenKey = "KEYWORD(FROM)";
    } else {
        currentTokenKey = currentToken.value;
    }

    // 构建预测表查询键
    string tableKey = "SelectColumns'|" + currentTokenKey;
    if (predictTable.find(tableKey) == predictTable.end()) {
        throw runtime_error("无匹配的SelectColumns'产生式：" + tableKey);
    }
    vector<string> production = predictTable[tableKey];

    // 逆序压入产生式符号（栈后进先出）
    for (auto it = production.rbegin(); it != production.rend(); ++it) {
        if (!it->empty()) {
            symStack.push(*it);
        }
    }
}

/**
 * 解析值列表递归部分：ValueList' → , CONSTANT ValueList' | ε
 */
void Parser::parseValueListPrime() {
    // 生成当前Token的标识（用于查询预测表）
    string currentTokenKey = (currentToken.type == TokenType::DELIMITER && currentToken.value == ",")
        ? "," : (currentToken.type == TokenType::DELIMITER && currentToken.value == ")") ? ")" : currentToken.value;

    // 构建预测表查询键
    string tableKey = "ValueList'|" + currentTokenKey;
    if (predictTable.find(tableKey) == predictTable.end()) {
        throw runtime_error("无匹配的ValueList'产生式：" + tableKey);
    }
    vector<string> production = predictTable[tableKey];

    // 逆序压入产生式符号（栈后进先出）
    for (auto it = production.rbegin(); it != production.rend(); ++it) {
        if (!it->empty()) {
            symStack.push(*it);
        }
    }
}

/**
 * 解析列列表递归部分：ColumnList' → , Column ColumnList' | ε
 */
void Parser::parseColumnListPrime() {
    // 生成当前Token的标识（用于查询预测表）
    string currentTokenKey = (currentToken.type == TokenType::DELIMITER && currentToken.value == ",")
        ? "," : (currentToken.type == TokenType::DELIMITER && currentToken.value == ")") ? ")" : currentToken.value;

    // 构建预测表查询键
    string tableKey = "ColumnList'|" + currentTokenKey;
    if (predictTable.find(tableKey) == predictTable.end()) {
        throw runtime_error("无匹配的ColumnList'产生式：" + tableKey);
    }
    vector<string> production = predictTable[tableKey];

    // 逆序压入产生式符号（栈后进先出）
    for (auto it = production.rbegin(); it != production.rend(); ++it) {
        if (!it->empty()) {
            symStack.push(*it);
        }
    }
}

/**
 * 解析WHERE子句：WhereClause → WHERE Condition | ε
 * @return 可选的Condition对象（有WHERE时返回条件，否则返回空）
 */
optional<Condition> Parser::parseWhereClause() {
    parseNonTerminal("WhereClause");

    // 匹配WHERE关键字（若存在）
    if (!symStack.empty() && symStack.top() == "KEYWORD(WHERE)") {
        match("KEYWORD(WHERE)");
        symStack.pop();
    }

    // 解析条件表达式（Condition → IDENTIFIER OPERATOR CONSTANT）
    if (!symStack.empty() && symStack.top() == "Condition") {
        Condition cond;
        parseNonTerminal("Condition");

        // 1. 匹配条件关联的列名（如 "age"）
        cond.column = currentToken.value;
        match("IDENTIFIER");
        symStack.pop();

        // 2. 匹配比较运算符（如 ">"、"<"）
        cond.op = currentToken.value;
        match("OPERATOR");
        symStack.pop();

        // 3. 匹配条件值（如 "20"、"'Alice'"）
        cond.value = currentToken.value;
        match("CONSTANT");
        symStack.pop();

        return cond;
    }

    // 无WHERE子句，返回空
    return nullopt;
}

//对外接口
/**
 * 核心解析接口：执行语法分析，生成AST
 * @return AST根节点（成功时为具体语句的AST，失败时为nullptr）
 */
// unique_ptr<ASTNode> Parser::parse() {
//     try {
//         // 验证栈初始化是否正确（栈顶应为开始符号Prog）
//         if (symStack.empty() || symStack.top() != "Prog") {
//             throw runtime_error("语法栈初始化错误：栈顶应为'Prog'");
//         }
//         // 开始解析程序入口
//         unique_ptr<ASTNode> result = parseProg();
//         return result;
//     } catch (const exception& e) {
//         // 捕获解析过程中的异常，打印错误信息和当前状态
//         return nullptr;
//     }
// }

/**
 * 解析DELETE语句：生成DeleteAST节点
 * 支持：带WHERE条件的删除操作
 * @return DeleteAST节点（包含目标表名、WHERE条件）
 */
unique_ptr<DeleteAST> Parser::parseDelete() {
    parseNonTerminal("Delete");

    // 匹配DELETE关键字
    match("KEYWORD(DELETE)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(DELETE)") {
        symStack.pop();
    }

    // 匹配FROM关键字
    match("KEYWORD(FROM)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(FROM)") {
        symStack.pop();
    }

    // 解析目标表名
    string tableName = currentToken.value;
    match("IDENTIFIER");
    if (!symStack.empty() && symStack.top() == "IDENTIFIER") {
        symStack.pop();
    }

    // 解析WHERE子句（可选，无WHERE时返回空）
    optional<Condition> condition = parseWhereClause();

    // 匹配语句结束分号
    match("SEMICOLON");
    if (!symStack.empty() && symStack.top() == "SEMICOLON") {
        symStack.pop();
    }

    //构建DeleteAST并返回
    auto ast = make_unique<DeleteAST>();
    ast->tableName = tableName;
    ast->condition = condition;
    return ast;
}

//AST打印函数（辅助调试
void printAST(ASTNode* ast) {
    if (!ast) {
        cout << "AST节点为空（解析失败）" << endl;
        return;
    }

    // 根据AST节点类型，打印对应结构
    if (auto selectAst = dynamic_cast<SelectAST*>(ast)) {
        cout << "SelectAST（SELECT语句）:" << endl;
        cout << "  查询列: ";
        for (size_t i = 0; i < selectAst->columns.size(); ++i) {
            if (i > 0) cout << " ";
            cout << selectAst->columns[i];
        }
        cout << endl;
        cout << "  目标表: " << selectAst->tableName << endl;
        if (selectAst->condition.has_value()) {
            auto& cond = selectAst->condition.value();
            cout << "  WHERE条件: " << cond.column << " " << cond.op << " " << cond.value << endl;
        } else {
            cout << "  WHERE条件: 无" << endl;
        }
    } else if (auto insertAst = dynamic_cast<InsertAST*>(ast)) {
        cout << "InsertAST（INSERT语句）:" << endl;
        cout << "  目标表: " << insertAst->tableName << endl;
        cout << "  插入值: " << endl;
        for (const auto& val : insertAst->values) {
            cout << "    - " << val << endl;
        }
    } else if (auto createAst = dynamic_cast<CreateTableAST*>(ast)) {
        cout << "CreateTableAST（CREATE TABLE语句）:" << endl;
        cout << "  表名: " << createAst->tableName << endl;
        cout << "  列定义: " << endl;
        for (const auto& col : createAst->columns) {
            cout << "    - 列名: " << col.name << ", 类型: " << col.type << endl;
        }
    } else if (auto deleteAst = dynamic_cast<DeleteAST*>(ast)) {
        cout << "DeleteAST（DELETE语句）:" << endl;
        cout << "  目标表: " << deleteAst->tableName << endl;
        if (deleteAst->condition.has_value()) {
            auto& cond = deleteAst->condition.value();
            cout << "  WHERE条件: " << cond.column << " " << cond.op << " " << cond.value << endl;
        } else {
            cout << "  WHERE条件: 无" << endl;
        }
    } else {
        cout << "未知AST节点类型" << endl;
    }
}