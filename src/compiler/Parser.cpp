/*
 * Parser.cpp - 基于LL(1)文法的SQL语法解析器
 * 支持SQL语句类型：
 * 1. SELECT（带WHERE子句、多列查询、通配符*）
 * 2. CREATE TABLE（多列定义，支持STRING/INT类型）
 * 3. INSERT（多值插入，兼容带引号/无引号字符串常量）
 */

#include "../../include/compiler/Parser.h"
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <memory>

using namespace std;

//构造函数
Parser::Parser(Lexer& l)
    : lexer(l),
      currentToken(TokenType::ERROR, "", -1, -1),
      tokenPos(0) {
    initPredictTable();
    //语法栈初始化：栈底为EOF（结束符），栈顶为开始符号Prog
    symStack.push("EOF");
    symStack.push("Prog");
    //从词法分析器获取所有Token并缓存
    tokens = lexer.getAllTokens();
    //初始化当前Token（若Token流非空）
    if (!tokens.empty()) {
        currentToken = tokens[tokenPos];
    } else {
        currentToken = Token(TokenType::EOF_TOKEN, "", -1, -1);
    }
}

//预测表初始化（LL(1)）
void Parser::initPredictTable() {
    // 1. 程序入口规则：Prog → Stmt EOF（一条SQL语句 + 结束符）
    predictTable["Prog|SELECT"] = {"Stmt", "EOF"};
    predictTable["Prog|CREATE"] = {"Stmt", "EOF"};
    predictTable["Prog|INSERT"] = {"Stmt", "EOF"};

    // 2. 语句规则：Stmt → CreateTable | Insert | Select（分发到具体语句）
    predictTable["Stmt|CREATE"] = {"CreateTable"};
    predictTable["Stmt|INSERT"] = {"Insert"};
    predictTable["Stmt|SELECT"] = {"Select"};

    // 3. CREATE TABLE语句规则：CreateTable → CREATE TABLE 表名 ( 列列表 ) ;
    predictTable["CreateTable|CREATE"] = {
        "KEYWORD(CREATE)", "KEYWORD(TABLE)", "IDENTIFIER",
        LPAREN, "ColumnList", RPAREN, SEMICOLON
    };

    // 4. 列列表规则（消除左递归）
    predictTable["ColumnList|IDENTIFIER"] = {"Column", "ColumnList'"};  // 初始列（由列名触发）
    predictTable["ColumnList'|,"] = {COMMA, "Column", "ColumnList'"};   // 逗号后接新列
    predictTable["ColumnList'|)"] = {};                                 // 右括号结束（空产生式）

    // 5. 列定义规则：Column → 列名 类型关键字（IDENTIFIER → 列名，KEYWORD(TYPE) → 类型）
    predictTable["Column|IDENTIFIER"] = {"IDENTIFIER", "KEYWORD(TYPE)"};
    predictTable["KEYWORD(TYPE)|INT"] = {"KEYWORD(INT)"};               // 类型为INT
    predictTable["KEYWORD(TYPE)|STRING"] = {"KEYWORD(STRING)"};         // 类型为STRING

    // 6. INSERT语句规则：Insert → INSERT INTO 表名 VALUES ( 值列表 ) ;
    predictTable["Insert|INSERT"] = {
        "KEYWORD(INSERT)", "KEYWORD(INTO)", "IDENTIFIER",
        "KEYWORD(VALUES)", LPAREN, "ValueList", RPAREN, SEMICOLON
    };

    // 7. 值列表规则（消除左递归，兼容无引号字符串）
    predictTable["ValueList|CONSTANT"] = {"CONSTANT", "ValueList'"};    // 正常常量（带引号字符串/数字）
    predictTable["ValueList|IDENTIFIER"] = {"CONSTANT", "ValueList'"};  // 兼容无引号字符串（兜底）
    predictTable["ValueList'|,"] = {COMMA, "CONSTANT", "ValueList'"};   // 逗号后接新值
    predictTable["ValueList'|)"] = {};                                 // 右括号结束（空产生式）

    // 8. SELECT语句规则：Select → SELECT 查询列 FROM 表名 WHERE子句 ;
    predictTable["Select|SELECT"] = {
        "KEYWORD(SELECT)", "SelectColumns", "KEYWORD(FROM)",
        "IDENTIFIER", "WhereClause", SEMICOLON
    };

    // 9. 查询列列表规则（消除左递归，支持通配符）
    predictTable["SelectColumns|*"] = {STAR};                           // 通配符*（查询所有列）
    predictTable["SelectColumns|IDENTIFIER"] = {"IDENTIFIER", "SelectColumns'"};  // 具体列名
    predictTable["SelectColumns'|,"] = {COMMA, "IDENTIFIER", "SelectColumns'"};   // 逗号后接新查询列
    predictTable["SelectColumns'|FROM"] = {};                             // 遇到FROM结束
    predictTable["SelectColumns'|;"] = {};                               // 遇到分号结束（无FROM场景）

    // 10. WHERE子句规则（可选）
    predictTable["WhereClause|WHERE"] = {"KEYWORD(WHERE)", "Condition"};  // 有WHERE时解析条件
    predictTable["WhereClause|;"] = {};                                 // 无WHERE时直接结束

    // 11. 条件表达式规则：Condition → 列名 运算符 值（IDENTIFIER → 列名，OPERATOR → 运算符，CONSTANT → 值）
    predictTable["Condition|IDENTIFIER"] = {"IDENTIFIER", "OPERATOR", "CONSTANT"};
}

//调试辅助函数
// void Parser::debugString(const string& s, const string& label) {
    // cerr << "[Debug] " << label << " - 长度: " << s.length() << ", 字符: ";
    // for (char c : s) {
    //     cerr << "'" << c << "'(" << static_cast<int>(c) << ") ";
    // }
    // cerr << endl;
// }

// void Parser::debugState() {
    // cerr << "\n[Debug] 当前Token: " << currentToken.value
    //      << " (类型: " << static_cast<int>(currentToken.type) << ")" << endl;

    // stack<string> tempStack = symStack;
    // cerr << "[Debug] 栈状态（顶→底）: ";
    // while (!tempStack.empty()) {
        // cerr << tempStack.top() << " ";
        // tempStack.pop();
    // }
    // cerr << endl;
// }

//核心解析函数
// 解析非终结符：根据当前Token和预测表，压入对应产生式符号
void Parser::parseNonTerminal(const string& nonTerminal) {
    //1. 生成当前Token的标识（用于查询预测表，如 "SELECT"、"IDENTIFIER"）
    string currentTokenKey;
    if (currentToken.type == TokenType::KEYWORD) {
        currentTokenKey = currentToken.value;
    } else if (currentToken.type == TokenType::IDENTIFIER) {
        currentTokenKey = "IDENTIFIER";
    } else if (currentToken.type == TokenType::CONSTANT) {
        currentTokenKey = "CONSTANT";
    } else if (currentToken.type == TokenType::OPERATOR) {
        currentTokenKey = "OPERATOR";
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
    // cerr << "[parseNonTerminal] 弹出非终结符: " << nonTerminal << endl;

    for (auto it = production.rbegin(); it != production.rend(); ++it) {
        if (!it->empty()) {
            symStack.push(*it);
            // cerr << "[parseNonTerminal] 压入符号: " << *it << "（属于" << nonTerminal << "）" << endl;
        }
    }
}


// 匹配终结符：检查当前Token是否与预期一致，匹配成功则推进Token流
void Parser::match(const string& expectedValue) {
    //1. 生成当前Token的标识（与预期值格式对齐）
    string currentTokenKey;
    if (currentToken.type == TokenType::KEYWORD) {
        currentTokenKey = "KEYWORD(" + currentToken.value + ")";
    } else if (currentToken.type == TokenType::IDENTIFIER) {
        currentTokenKey = "IDENTIFIER";
        //兼容处理1：无引号字符串按CONSTANT匹配（针对INSERT值列表）
        if (expectedValue == "CONSTANT") {
            currentTokenKey = "CONSTANT";
        }
        //兼容处理2：类型关键字（INT/STRING）按KEYWORD匹配（针对CREATE TABLE列类型）
        if (currentToken.value == "INT" || currentToken.value == "STRING") {
            currentTokenKey = "KEYWORD(" + currentToken.value + ")";
        }
    } else if (currentToken.type == TokenType::CONSTANT) {
        currentTokenKey = "CONSTANT";
    } else if (currentToken.type == TokenType::OPERATOR) {
        currentTokenKey = "OPERATOR";
    } else if (currentToken.type == TokenType::DELIMITER) {
        //映射分隔符到统一常量（与预测表符号一致）
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

    //2. 特殊匹配：类型关键字（INT/STRING）匹配"KEYWORD(TYPE)"
    bool isTypeMatch = (expectedValue == "KEYWORD(TYPE)" &&
                      (currentTokenKey == "KEYWORD(INT)" || currentTokenKey == "KEYWORD(STRING)"));
    if (!isTypeMatch && currentTokenKey != expectedValue) {
        stringstream errMsg;
        errMsg << "语法错误：预期'" << expectedValue << "', 实际'" << currentTokenKey
               << "'（行: " << currentToken.line << ", 列: " << currentToken.column << "）";
        throw runtime_error(errMsg.str());
    }

    //3. 推进Token流（无论是否匹配成功，异常已提前抛出）
    string matchedValue = currentToken.value;
    tokenPos++;
    currentToken = (tokenPos < tokens.size()) ? tokens[tokenPos] : Token(TokenType::EOF_TOKEN, "", -1, -1);

    // //4. 输出匹配信息（便于调试，区分普通匹配和类型匹配）
    // if (isTypeMatch) {
    //     cerr << "[match] 类型匹配: " << expectedValue << " <- " << matchedValue << endl;
    // } else {
    //     cerr << "[match] 匹配成功: " << expectedValue << " <- " << matchedValue << endl;
    // }
}

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
            cerr << "[parseProg] 清理Stmt残留符号: " << top << endl;
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
            // debugState();
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

    //1. 匹配INSERT关键字
    match("KEYWORD(INSERT)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(INSERT)") {
        symStack.pop();
    }

    //2. 匹配INTO关键字
    match("KEYWORD(INTO)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(INTO)") {
        symStack.pop();
    }

    //3. 解析目标表名
    string tableName = currentToken.value;
    match("IDENTIFIER");
    if (!symStack.empty() && symStack.top() == "IDENTIFIER") {
        symStack.pop();
    }

    //4. 匹配VALUES关键字
    match("KEYWORD(VALUES)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(VALUES)") {
        symStack.pop();
    }

    //5. 匹配左括号（值列表开始）
    match(LPAREN);
    if (!symStack.empty() && symStack.top() == LPAREN) {
        symStack.pop();
    }

    //6. 解析插入值列表（如 "'Alice', 20"）
    vector<string> values = parseValueList();

    //7. 匹配右括号（值列表结束）
    match(RPAREN);
    if (!symStack.empty() && symStack.top() == RPAREN) {
        symStack.pop();
    }

    //8. 匹配语句结束分号
    match("SEMICOLON");
    if (!symStack.empty() && symStack.top() == "SEMICOLON") {
        symStack.pop();
    }

    //构建InsertAST并返回
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
    // cerr << "\n[CREATE TABLE] 开始解析CREATE TABLE语句" << endl;
    parseNonTerminal("CreateTable");

    // 1. 匹配CREATE关键字
    match("KEYWORD(CREATE)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(CREATE)") {
        symStack.pop();
    }
    // cerr << "[CREATE TABLE] 完成CREATE关键字匹配，下一个Token: " << currentToken.value << endl;

    // 2. 匹配TABLE关键字
    match("KEYWORD(TABLE)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(TABLE)") {
        symStack.pop();
    }
    // cerr << "[CREATE TABLE] 完成TABLE关键字匹配，下一个Token: " << currentToken.value << endl;

    // 3. 解析表名（如 "Students"）
    string tableName = currentToken.value;
    match("IDENTIFIER");
    if (!symStack.empty() && symStack.top() == "IDENTIFIER") {
        symStack.pop();
    }
    // cerr << "[CREATE TABLE] 完成表名匹配: " << tableName << "，下一个Token: " << currentToken.value << endl;

    // 4. 匹配左括号（列列表开始）
    match(LPAREN);
    if (!symStack.empty() && symStack.top() == LPAREN) {
        symStack.pop();
    }
    // cerr << "[CREATE TABLE] 完成左括号匹配，下一个Token: " << currentToken.value << endl;

    // 5. 解析列列表（核心步骤：处理栈状态干扰）
    vector<Column> columns;
    stack<string> tempStack; // 临时栈：保存ColumnList之上的干扰符号（RPAREN/SEMICOLON）
    try {
        // 临时移出ColumnList之上的符号，确保列列表解析时栈顶仅为ColumnList相关符号
        while (!symStack.empty() && symStack.top() != "ColumnList") {
            tempStack.push(symStack.top());
            symStack.pop();
        }
        // 仅在有移出符号时打印日志（避免冗余）
        if (!tempStack.empty()) {
            // cerr << "[CREATE TABLE] 临时移出干扰符号: " << tempStack.size() << "个" << endl;
        }

        // 解析列列表（生成列名和类型的对应关系）
        columns = parseColumnList();
        // cerr << "[CREATE TABLE] 列列表解析完成，共" << columns.size() << "列" << endl;

        // 恢复临时移出的符号（RPAREN/SEMICOLON），继续后续匹配
        while (!tempStack.empty()) {
            symStack.push(tempStack.top());
            tempStack.pop();
        }
        if (!tempStack.empty()) {
            // cerr << "[CREATE TABLE] 恢复符号后栈顶: " << symStack.top() << endl;
        }

    } catch (const exception& e) {
        // cerr << "[CREATE TABLE] 列列表解析失败: " << e.what() << endl;
        throw; // 重新抛出异常，让上层处理
    }

    // 6. 匹配右括号（列列表结束）
    match(RPAREN);
    if (!symStack.empty() && symStack.top() == RPAREN) {
        symStack.pop();
    }
    // cerr << "[CREATE TABLE] 完成右括号匹配" << endl;

    // 7. 匹配语句结束分号
    match("SEMICOLON");
    if (!symStack.empty() && symStack.top() == "SEMICOLON") {
        symStack.pop();
    }
    // cerr << "[CREATE TABLE] 完成分号匹配" << endl;

    // 关键修复：清理CREATE TABLE相关的残留栈符号（确保栈顶仅保留EOF）
    while (!symStack.empty()) {
        string top = symStack.top();
        if (top == "CreateTable" || top == "RPAREN" || top == "SEMICOLON") {
            symStack.pop();
            // cerr << "[CREATE TABLE] 清理残留符号: " << top << endl;
        } else {
            break;
        }
    }

    // 构建CreateTableAST并返回
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
        if (currentToken.type == TokenType::KEYWORD && currentToken.value == "FROM") {
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
 * @return 插入值字符串列表（如 ["Alice", "20"]）
 */
vector<string> Parser::parseValueList() {
    parseNonTerminal("ValueList");
    vector<string> values;

    while (!symStack.empty()) {
        string stackTop = symStack.top();
        if (stackTop == "CONSTANT") {
            // 匹配常量（带引号字符串/数字/无引号字符串）
            values.push_back(currentToken.value);
            match("CONSTANT");
            symStack.pop();
        } else if (stackTop == "ValueList'") {
            // 处理递归部分（逗号分隔的后续值）
            symStack.pop();
            parseValueListPrime();
            if (!symStack.empty() && symStack.top() == COMMA) {
                match(COMMA);
                symStack.pop();
            }
        } else {
            // 其他符号（如右括号），结束解析
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
    bool parsingActive = true;

    while (!symStack.empty() && parsingActive) {
        string stackTop = symStack.top();
        // cerr << "[ColumnList] 栈顶符号: " << stackTop << ", 当前Token: " << currentToken.value << endl;

        if (stackTop == "Column") {
            // 解析单个列（列名 + 类型）
            Column currentCol;
            parseNonTerminal("Column");  // 压入 "IDENTIFIER" + "KEYWORD(TYPE)"

            // 匹配列名（如 "name"、"age"）
            currentCol.name = currentToken.value;
            match("IDENTIFIER");
            symStack.pop();
            // cerr << "[ColumnList] 解析列名: " << currentCol.name << endl;

            // 匹配列类型（如 "STRING"、"INT"，由match处理类型匹配）
            currentCol.type = currentToken.value;
            match("KEYWORD(TYPE)");
            symStack.pop();
            // cerr << "[ColumnList] 解析列类型: " << currentCol.type << endl;

            // 将当前列添加到列表
            columns.push_back(currentCol);

        } else if (stackTop == "ColumnList'") {
            // 解析递归部分（处理逗号分隔的后续列）
            parseColumnListPrime();
            if (!symStack.empty() && symStack.top() == COMMA) {
                // 匹配逗号，继续解析下一列
                match(COMMA);
                symStack.pop();
                // cerr << "[ColumnList] 匹配逗号，准备解析下一列" << endl;
            } else {
                // 空产生式（遇到右括号），弹出当前ColumnList'并结束递归
                symStack.pop();
                parsingActive = false;
            }

        } else if (stackTop == "ColumnList") {
            // 防止漏解析，主动弹出ColumnList符号
            symStack.pop();

        } else {
            // 非列相关符号（如右括号），结束解析
            parsingActive = false;
        }
    }

    // 关键修复：强制清理列列表相关残留符号（确保栈中无ColumnList/ColumnList'）
    while (!symStack.empty()) {
        string top = symStack.top();
        if (top == "ColumnList" || top == "ColumnList'") {
            symStack.pop();
            // cerr << "[ColumnList] 清理残留符号: " << top << endl;
        } else {
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
    string currentTokenKey = (currentToken.type == TokenType::DELIMITER && currentToken.value == ",")
        ? "," : (currentToken.type == TokenType::KEYWORD && currentToken.value == "FROM") ? "FROM" : currentToken.value;

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
        // cerr << "[WhereClause] 捕获条件列名: " << cond.column << endl;

        // 2. 匹配比较运算符（如 ">"、"<"）
        cond.op = currentToken.value;
        match("OPERATOR");
        symStack.pop();
        // cerr << "[WhereClause] 捕获运算符: " << cond.op << endl;

        // 3. 匹配条件值（如 "20"、"'Alice'"）
        cond.value = currentToken.value;
        match("CONSTANT");
        symStack.pop();
        // cerr << "[WhereClause] 捕获条件值: " << cond.value << endl;

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
unique_ptr<ASTNode> Parser::parse() {
    try {
        // 验证栈初始化是否正确（栈顶应为开始符号Prog）
        if (symStack.empty() || symStack.top() != "Prog") {
            throw runtime_error("语法栈初始化错误：栈顶应为'Prog'");
        }
        // 开始解析程序入口
        unique_ptr<ASTNode> result = parseProg();

        // 关键优化：刷新cerr缓存，确保调试日志完整输出后再打印AST结果
        // cerr.flush();
        return result;
    } catch (const exception& e) {
        // 捕获解析过程中的异常，打印错误信息和当前状态
        // cerr << "\n[解析错误] " << e.what() << endl;
        // debugState();
        // cerr.flush(); // 异常时也刷新日志
        return nullptr;
    }
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
    } else {
        cout << "未知AST节点类型" << endl;
    }
}