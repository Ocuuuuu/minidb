/*
 * Parser.cpp - ����LL(1)�ķ���SQL�﷨������
 * ֧��SQL������ͣ�
 * 1. SELECT����WHERE�Ӿ䡢���в�ѯ��ͨ���*��
 * 2. CREATE TABLE�����ж��壬֧��STRING/INT���ͣ�
 * 3. INSERT����ֵ���룬���ݴ�����/�������ַ���������
 */

#include "../../include/compiler/Parser.h"
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <memory>

using namespace std;

//���캯��
Parser::Parser(Lexer& l)
    : lexer(l),
      currentToken(TokenType::ERROR, "", -1, -1),
      tokenPos(0) {
    initPredictTable();
    //�﷨ջ��ʼ����ջ��ΪEOF������������ջ��Ϊ��ʼ����Prog
    symStack.push("EOF");
    symStack.push("Prog");
    //�Ӵʷ���������ȡ����Token������
    tokens = lexer.getAllTokens();
    //��ʼ����ǰToken����Token���ǿգ�
    if (!tokens.empty()) {
        currentToken = tokens[tokenPos];
    } else {
        currentToken = Token(TokenType::EOF_TOKEN, "", -1, -1);
    }
}

//Ԥ����ʼ����LL(1)��
void Parser::initPredictTable() {
    // 1. ������ڹ���Prog �� Stmt EOF��һ��SQL��� + ��������
    predictTable["Prog|SELECT"] = {"Stmt", "EOF"};
    predictTable["Prog|CREATE"] = {"Stmt", "EOF"};
    predictTable["Prog|INSERT"] = {"Stmt", "EOF"};

    // 2. ������Stmt �� CreateTable | Insert | Select���ַ���������䣩
    predictTable["Stmt|CREATE"] = {"CreateTable"};
    predictTable["Stmt|INSERT"] = {"Insert"};
    predictTable["Stmt|SELECT"] = {"Select"};

    // 3. CREATE TABLE������CreateTable �� CREATE TABLE ���� ( ���б� ) ;
    predictTable["CreateTable|CREATE"] = {
        "KEYWORD(CREATE)", "KEYWORD(TABLE)", "IDENTIFIER",
        LPAREN, "ColumnList", RPAREN, SEMICOLON
    };

    // 4. ���б����������ݹ飩
    predictTable["ColumnList|IDENTIFIER"] = {"Column", "ColumnList'"};  // ��ʼ�У�������������
    predictTable["ColumnList'|,"] = {COMMA, "Column", "ColumnList'"};   // ���ź������
    predictTable["ColumnList'|)"] = {};                                 // �����Ž������ղ���ʽ��

    // 5. �ж������Column �� ���� ���͹ؼ��֣�IDENTIFIER �� ������KEYWORD(TYPE) �� ���ͣ�
    predictTable["Column|IDENTIFIER"] = {"IDENTIFIER", "KEYWORD(TYPE)"};
    predictTable["KEYWORD(TYPE)|INT"] = {"KEYWORD(INT)"};               // ����ΪINT
    predictTable["KEYWORD(TYPE)|STRING"] = {"KEYWORD(STRING)"};         // ����ΪSTRING

    // 6. INSERT������Insert �� INSERT INTO ���� VALUES ( ֵ�б� ) ;
    predictTable["Insert|INSERT"] = {
        "KEYWORD(INSERT)", "KEYWORD(INTO)", "IDENTIFIER",
        "KEYWORD(VALUES)", LPAREN, "ValueList", RPAREN, SEMICOLON
    };

    // 7. ֵ�б����������ݹ飬�����������ַ�����
    predictTable["ValueList|CONSTANT"] = {"CONSTANT", "ValueList'"};    // �����������������ַ���/���֣�
    predictTable["ValueList|IDENTIFIER"] = {"CONSTANT", "ValueList'"};  // �����������ַ��������ף�
    predictTable["ValueList'|,"] = {COMMA, "CONSTANT", "ValueList'"};   // ���ź����ֵ
    predictTable["ValueList'|)"] = {};                                 // �����Ž������ղ���ʽ��

    // 8. SELECT������Select �� SELECT ��ѯ�� FROM ���� WHERE�Ӿ� ;
    predictTable["Select|SELECT"] = {
        "KEYWORD(SELECT)", "SelectColumns", "KEYWORD(FROM)",
        "IDENTIFIER", "WhereClause", SEMICOLON
    };

    // 9. ��ѯ���б����������ݹ飬֧��ͨ�����
    predictTable["SelectColumns|*"] = {STAR};                           // ͨ���*����ѯ�����У�
    predictTable["SelectColumns|IDENTIFIER"] = {"IDENTIFIER", "SelectColumns'"};  // ��������
    predictTable["SelectColumns'|,"] = {COMMA, "IDENTIFIER", "SelectColumns'"};   // ���ź���²�ѯ��
    predictTable["SelectColumns'|FROM"] = {};                             // ����FROM����
    predictTable["SelectColumns'|;"] = {};                               // �����ֺŽ�������FROM������

    // 10. WHERE�Ӿ���򣨿�ѡ��
    predictTable["WhereClause|WHERE"] = {"KEYWORD(WHERE)", "Condition"};  // ��WHEREʱ��������
    predictTable["WhereClause|;"] = {};                                 // ��WHEREʱֱ�ӽ���

    // 11. �������ʽ����Condition �� ���� ����� ֵ��IDENTIFIER �� ������OPERATOR �� �������CONSTANT �� ֵ��
    predictTable["Condition|IDENTIFIER"] = {"IDENTIFIER", "OPERATOR", "CONSTANT"};
}

//���Ը�������
// void Parser::debugString(const string& s, const string& label) {
    // cerr << "[Debug] " << label << " - ����: " << s.length() << ", �ַ�: ";
    // for (char c : s) {
    //     cerr << "'" << c << "'(" << static_cast<int>(c) << ") ";
    // }
    // cerr << endl;
// }

// void Parser::debugState() {
    // cerr << "\n[Debug] ��ǰToken: " << currentToken.value
    //      << " (����: " << static_cast<int>(currentToken.type) << ")" << endl;

    // stack<string> tempStack = symStack;
    // cerr << "[Debug] ջ״̬�������ף�: ";
    // while (!tempStack.empty()) {
        // cerr << tempStack.top() << " ";
        // tempStack.pop();
    // }
    // cerr << endl;
// }

//���Ľ�������
// �������ս�������ݵ�ǰToken��Ԥ���ѹ���Ӧ����ʽ����
void Parser::parseNonTerminal(const string& nonTerminal) {
    //1. ���ɵ�ǰToken�ı�ʶ�����ڲ�ѯԤ����� "SELECT"��"IDENTIFIER"��
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
        throw runtime_error("δ֪Token����: " + to_string(static_cast<int>(currentToken.type)));
    }

    //2. ����Ԥ����ѯ������ʽ��"���ս��|��ǰToken��ʶ"��
    string tableKey = nonTerminal + "|" + currentTokenKey;
    if (predictTable.find(tableKey) == predictTable.end()) {
        stringstream errMsg;
        errMsg << "��ƥ�����ʽ: " << tableKey
               << "�����ս��: " << nonTerminal << ", Token: " << currentToken.value << "��";
        throw runtime_error(errMsg.str());
    }
    vector<string> production = predictTable[tableKey];

    //3. ��֤�﷨ջ���Ƿ�Ϊ��ǰ���ս����ȷ�������߼���ȷ�ԣ�
    if (symStack.empty() || symStack.top() != nonTerminal) {
        stringstream errMsg;
        errMsg << "ջ���쳣������'" << nonTerminal << "'ʱ��Ԥ��'" << nonTerminal
               << "', ʵ��'" << (symStack.empty() ? "��" : symStack.top()) << "'";
        throw runtime_error(errMsg.str());
    }

    //4. �������ս��������ѹ�����ʽ���ţ�ջ����ȳ�����֤����˳����ȷ��
    symStack.pop();
    // cerr << "[parseNonTerminal] �������ս��: " << nonTerminal << endl;

    for (auto it = production.rbegin(); it != production.rend(); ++it) {
        if (!it->empty()) {
            symStack.push(*it);
            // cerr << "[parseNonTerminal] ѹ�����: " << *it << "������" << nonTerminal << "��" << endl;
        }
    }
}


// ƥ���ս������鵱ǰToken�Ƿ���Ԥ��һ�£�ƥ��ɹ����ƽ�Token��
void Parser::match(const string& expectedValue) {
    //1. ���ɵ�ǰToken�ı�ʶ����Ԥ��ֵ��ʽ���룩
    string currentTokenKey;
    if (currentToken.type == TokenType::KEYWORD) {
        currentTokenKey = "KEYWORD(" + currentToken.value + ")";
    } else if (currentToken.type == TokenType::IDENTIFIER) {
        currentTokenKey = "IDENTIFIER";
        //���ݴ���1���������ַ�����CONSTANTƥ�䣨���INSERTֵ�б�
        if (expectedValue == "CONSTANT") {
            currentTokenKey = "CONSTANT";
        }
        //���ݴ���2�����͹ؼ��֣�INT/STRING����KEYWORDƥ�䣨���CREATE TABLE�����ͣ�
        if (currentToken.value == "INT" || currentToken.value == "STRING") {
            currentTokenKey = "KEYWORD(" + currentToken.value + ")";
        }
    } else if (currentToken.type == TokenType::CONSTANT) {
        currentTokenKey = "CONSTANT";
    } else if (currentToken.type == TokenType::OPERATOR) {
        currentTokenKey = "OPERATOR";
    } else if (currentToken.type == TokenType::DELIMITER) {
        //ӳ��ָ�����ͳһ��������Ԥ������һ�£�
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
        throw runtime_error("δ֪Token����: " + to_string(static_cast<int>(currentToken.type)));
    }

    //2. ����ƥ�䣺���͹ؼ��֣�INT/STRING��ƥ��"KEYWORD(TYPE)"
    bool isTypeMatch = (expectedValue == "KEYWORD(TYPE)" &&
                      (currentTokenKey == "KEYWORD(INT)" || currentTokenKey == "KEYWORD(STRING)"));
    if (!isTypeMatch && currentTokenKey != expectedValue) {
        stringstream errMsg;
        errMsg << "�﷨����Ԥ��'" << expectedValue << "', ʵ��'" << currentTokenKey
               << "'����: " << currentToken.line << ", ��: " << currentToken.column << "��";
        throw runtime_error(errMsg.str());
    }

    //3. �ƽ�Token���������Ƿ�ƥ��ɹ����쳣����ǰ�׳���
    string matchedValue = currentToken.value;
    tokenPos++;
    currentToken = (tokenPos < tokens.size()) ? tokens[tokenPos] : Token(TokenType::EOF_TOKEN, "", -1, -1);

    // //4. ���ƥ����Ϣ�����ڵ��ԣ�������ͨƥ�������ƥ�䣩
    // if (isTypeMatch) {
    //     cerr << "[match] ����ƥ��: " << expectedValue << " <- " << matchedValue << endl;
    // } else {
    //     cerr << "[match] ƥ��ɹ�: " << expectedValue << " <- " << matchedValue << endl;
    // }
}

//����������
/**
 * ����������ڣ�Prog �� Stmt EOF
 * @return AST���ڵ㣨��Ӧһ��SQL����AST��
 */
unique_ptr<ASTNode> Parser::parseProg() {
    parseNonTerminal("Prog");
    unique_ptr<ASTNode> stmtAST = parseStmt();

    //����Stmt��������ܲ����ķ��ս����ȷ��ջ��ΪEOF��
    while (!symStack.empty()) {
        string top = symStack.top();
        if (top != "EOF") {
            //��������EOF�Ĳ������ţ�������ɾEOF��
            symStack.pop();
            cerr << "[parseProg] ����Stmt��������: " << top << endl;
        } else {
            break;
        }
    }

    //ƥ�������EOF��ȷ�����������
    if (!symStack.empty() && symStack.top() == "EOF") {
        match("EOF");
        symStack.pop();
    } else {
        throw runtime_error("����������ջ��ӦΪ'EOF'");
    }

    //����ѹ��EOF��Ϊ��һ����������׼�������У�
    symStack.push("EOF");
    return stmtAST;
}

/**
 * �������ַ���Stmt �� CreateTable | Insert | Select
 * @return ��������AST�ڵ㣨CreateTableAST/InsertAST/SelectAST��
 */
unique_ptr<ASTNode> Parser::parseStmt() {
    parseNonTerminal("Stmt");
    unique_ptr<ASTNode> resultAST;

    // �����﷨ջ�����ţ��ַ�����Ӧ���Ľ�������
    if (!symStack.empty() && symStack.top() == "CreateTable") {
        resultAST = parseCreateTable();
    } else if (!symStack.empty() && symStack.top() == "Insert") {
        resultAST = parseInsert();
    } else if (!symStack.empty() && symStack.top() == "Select") {
        resultAST = parseSelect();
    } else {
        throw runtime_error("δ֪������ͣ�ջ��Ϊ'" + (symStack.empty() ? "��" : symStack.top()) + "'");
    }
    return resultAST;
}

/**
 * ����SELECT��䣺����SelectAST�ڵ�
 * ֧�֣����в�ѯ��ͨ���*��WHERE�Ӿ�����
 * @return SelectAST�ڵ㣨������ѯ�С�������WHERE������
 */
unique_ptr<SelectAST> Parser::parseSelect() {
    parseNonTerminal("Select");

    //1. ƥ��SELECT�ؼ���
    match("KEYWORD(SELECT)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(SELECT)") {
        symStack.pop();
    }

    //2. ������ѯ���б��� "name, age" �� "*"��
    vector<string> selectCols = parseSelectColumns();

    //3. ƥ��FROM�ؼ��֣�ȷ���﷨�ṹ��ȷ��
    if (symStack.empty() || symStack.top() != "KEYWORD(FROM)") {
        // ����ջ��ʣ���SelectColumns'����
        while (!symStack.empty() && symStack.top() == "SelectColumns'") {
            symStack.pop();
        }
        if (symStack.empty() || symStack.top() != "KEYWORD(FROM)") {
            // debugState();
            throw runtime_error("SELECT����﷨����Ԥ��'FROM'�ؼ���");
        }
    }
    match("KEYWORD(FROM)");
    symStack.pop();

    //4. ������ѯ����
    string tableName = currentToken.value;
    match("IDENTIFIER");
    if (!symStack.empty() && symStack.top() == "IDENTIFIER") {
        symStack.pop();
    }

    //5. ����WHERE�Ӿ䣨��ѡ����WHEREʱ���ؿգ�
    optional<Condition> condition = parseWhereClause();

    //6. ƥ���������ֺ�
    match("SEMICOLON");
    if (!symStack.empty() && symStack.top() == "SEMICOLON") {
        symStack.pop();
    }

    //����SelectAST������
    auto ast = make_unique<SelectAST>();
    ast->columns = selectCols;
    ast->tableName = tableName;
    ast->condition = condition;
    return ast;
}

/**
 * ����INSERT��䣺����InsertAST�ڵ�
 * ֧�֣���ֵ���룬���ݴ�����/�������ַ�������
 * @return InsertAST�ڵ㣨����Ŀ�����������ֵ�б�
 */
unique_ptr<InsertAST> Parser::parseInsert() {
    parseNonTerminal("Insert");

    //1. ƥ��INSERT�ؼ���
    match("KEYWORD(INSERT)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(INSERT)") {
        symStack.pop();
    }

    //2. ƥ��INTO�ؼ���
    match("KEYWORD(INTO)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(INTO)") {
        symStack.pop();
    }

    //3. ����Ŀ�����
    string tableName = currentToken.value;
    match("IDENTIFIER");
    if (!symStack.empty() && symStack.top() == "IDENTIFIER") {
        symStack.pop();
    }

    //4. ƥ��VALUES�ؼ���
    match("KEYWORD(VALUES)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(VALUES)") {
        symStack.pop();
    }

    //5. ƥ�������ţ�ֵ�б�ʼ��
    match(LPAREN);
    if (!symStack.empty() && symStack.top() == LPAREN) {
        symStack.pop();
    }

    //6. ��������ֵ�б��� "'Alice', 20"��
    vector<string> values = parseValueList();

    //7. ƥ�������ţ�ֵ�б������
    match(RPAREN);
    if (!symStack.empty() && symStack.top() == RPAREN) {
        symStack.pop();
    }

    //8. ƥ���������ֺ�
    match("SEMICOLON");
    if (!symStack.empty() && symStack.top() == "SEMICOLON") {
        symStack.pop();
    }

    //����InsertAST������
    auto ast = make_unique<InsertAST>();
    ast->tableName = tableName;
    ast->values = values;
    return ast;
}

/**
 * ����CREATE TABLE��䣺����CreateTableAST�ڵ�
 * ֧�ֶ��ж��壬������ΪSTRING/INT
 * @return CreateTableAST�ڵ㣨�������������б�
 */
unique_ptr<CreateTableAST> Parser::parseCreateTable() {
    // cerr << "\n[CREATE TABLE] ��ʼ����CREATE TABLE���" << endl;
    parseNonTerminal("CreateTable");

    // 1. ƥ��CREATE�ؼ���
    match("KEYWORD(CREATE)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(CREATE)") {
        symStack.pop();
    }
    // cerr << "[CREATE TABLE] ���CREATE�ؼ���ƥ�䣬��һ��Token: " << currentToken.value << endl;

    // 2. ƥ��TABLE�ؼ���
    match("KEYWORD(TABLE)");
    if (!symStack.empty() && symStack.top() == "KEYWORD(TABLE)") {
        symStack.pop();
    }
    // cerr << "[CREATE TABLE] ���TABLE�ؼ���ƥ�䣬��һ��Token: " << currentToken.value << endl;

    // 3. ������������ "Students"��
    string tableName = currentToken.value;
    match("IDENTIFIER");
    if (!symStack.empty() && symStack.top() == "IDENTIFIER") {
        symStack.pop();
    }
    // cerr << "[CREATE TABLE] ��ɱ���ƥ��: " << tableName << "����һ��Token: " << currentToken.value << endl;

    // 4. ƥ�������ţ����б�ʼ��
    match(LPAREN);
    if (!symStack.empty() && symStack.top() == LPAREN) {
        symStack.pop();
    }
    // cerr << "[CREATE TABLE] ���������ƥ�䣬��һ��Token: " << currentToken.value << endl;

    // 5. �������б����Ĳ��裺����ջ״̬���ţ�
    vector<Column> columns;
    stack<string> tempStack; // ��ʱջ������ColumnList֮�ϵĸ��ŷ��ţ�RPAREN/SEMICOLON��
    try {
        // ��ʱ�Ƴ�ColumnList֮�ϵķ��ţ�ȷ�����б����ʱջ����ΪColumnList��ط���
        while (!symStack.empty() && symStack.top() != "ColumnList") {
            tempStack.push(symStack.top());
            symStack.pop();
        }
        // �������Ƴ�����ʱ��ӡ��־���������ࣩ
        if (!tempStack.empty()) {
            // cerr << "[CREATE TABLE] ��ʱ�Ƴ����ŷ���: " << tempStack.size() << "��" << endl;
        }

        // �������б��������������͵Ķ�Ӧ��ϵ��
        columns = parseColumnList();
        // cerr << "[CREATE TABLE] ���б������ɣ���" << columns.size() << "��" << endl;

        // �ָ���ʱ�Ƴ��ķ��ţ�RPAREN/SEMICOLON������������ƥ��
        while (!tempStack.empty()) {
            symStack.push(tempStack.top());
            tempStack.pop();
        }
        if (!tempStack.empty()) {
            // cerr << "[CREATE TABLE] �ָ����ź�ջ��: " << symStack.top() << endl;
        }

    } catch (const exception& e) {
        // cerr << "[CREATE TABLE] ���б����ʧ��: " << e.what() << endl;
        throw; // �����׳��쳣�����ϲ㴦��
    }

    // 6. ƥ�������ţ����б������
    match(RPAREN);
    if (!symStack.empty() && symStack.top() == RPAREN) {
        symStack.pop();
    }
    // cerr << "[CREATE TABLE] ���������ƥ��" << endl;

    // 7. ƥ���������ֺ�
    match("SEMICOLON");
    if (!symStack.empty() && symStack.top() == "SEMICOLON") {
        symStack.pop();
    }
    // cerr << "[CREATE TABLE] ��ɷֺ�ƥ��" << endl;

    // �ؼ��޸�������CREATE TABLE��صĲ���ջ���ţ�ȷ��ջ��������EOF��
    while (!symStack.empty()) {
        string top = symStack.top();
        if (top == "CreateTable" || top == "RPAREN" || top == "SEMICOLON") {
            symStack.pop();
            // cerr << "[CREATE TABLE] �����������: " << top << endl;
        } else {
            break;
        }
    }

    // ����CreateTableAST������
    auto ast = make_unique<CreateTableAST>();
    ast->tableName = tableName;
    ast->columns = columns;
    return ast;
}

//�б��������
/**
 * ������ѯ���б�SELECT�ã����� "name, age" �� "*"
 * @return ��ѯ�������б��� ["name", "age"] �� ["*"]��
 */
vector<string> Parser::parseSelectColumns() {
    parseNonTerminal("SelectColumns");
    vector<string> columns;
    bool parsingComplete = false;

    while (!symStack.empty() && !parsingComplete) {
        // ����FROM�ؼ���ʱ��������ѯ�н���
        if (currentToken.type == TokenType::KEYWORD && currentToken.value == "FROM") {
            parsingComplete = true;
            break;
        }

        string stackTop = symStack.top();
        if (stackTop == STAR) {
            // ƥ��ͨ���*����ѯ�����У�
            columns.push_back("*");
            match(STAR);
            symStack.pop();
        } else if (stackTop == "IDENTIFIER") {
            // ƥ������������� "name"��"age"��
            columns.push_back(currentToken.value);
            match("IDENTIFIER");
            symStack.pop();
        } else if (stackTop == "SelectColumns'") {
            // ����ݹ鲿�֣����ŷָ��ĺ����У�
            symStack.pop();
            parseSelectColumnsPrime();
            if (!symStack.empty() && symStack.top() == COMMA) {
                match(COMMA);
                symStack.pop();
            }
        } else {
            // �������ţ���ֺţ�����������
            parsingComplete = true;
        }
    }

    // ����ջ��ʣ���SelectColumns'����
    while (!symStack.empty() && symStack.top() == "SelectColumns'") {
        symStack.pop();
    }
    return columns;
}

/**
 * ����ֵ�б�INSERT�ã����� "'Alice', 20"
 * @return ����ֵ�ַ����б��� ["Alice", "20"]��
 */
vector<string> Parser::parseValueList() {
    parseNonTerminal("ValueList");
    vector<string> values;

    while (!symStack.empty()) {
        string stackTop = symStack.top();
        if (stackTop == "CONSTANT") {
            // ƥ�䳣�����������ַ���/����/�������ַ�����
            values.push_back(currentToken.value);
            match("CONSTANT");
            symStack.pop();
        } else if (stackTop == "ValueList'") {
            // ����ݹ鲿�֣����ŷָ��ĺ���ֵ��
            symStack.pop();
            parseValueListPrime();
            if (!symStack.empty() && symStack.top() == COMMA) {
                match(COMMA);
                symStack.pop();
            }
        } else {
            // �������ţ��������ţ�����������
            break;
        }
    }

    return values;
}

/**
 * �������б�CREATE TABLE�ã����� "name STRING, age INT"
 * @return ����Ϣ�б�����ÿ���е����ƺ����ͣ�
 */
vector<Column> Parser::parseColumnList() {
    parseNonTerminal("ColumnList");
    vector<Column> columns;
    bool parsingActive = true;

    while (!symStack.empty() && parsingActive) {
        string stackTop = symStack.top();
        // cerr << "[ColumnList] ջ������: " << stackTop << ", ��ǰToken: " << currentToken.value << endl;

        if (stackTop == "Column") {
            // ���������У����� + ���ͣ�
            Column currentCol;
            parseNonTerminal("Column");  // ѹ�� "IDENTIFIER" + "KEYWORD(TYPE)"

            // ƥ���������� "name"��"age"��
            currentCol.name = currentToken.value;
            match("IDENTIFIER");
            symStack.pop();
            // cerr << "[ColumnList] ��������: " << currentCol.name << endl;

            // ƥ�������ͣ��� "STRING"��"INT"����match��������ƥ�䣩
            currentCol.type = currentToken.value;
            match("KEYWORD(TYPE)");
            symStack.pop();
            // cerr << "[ColumnList] ����������: " << currentCol.type << endl;

            // ����ǰ����ӵ��б�
            columns.push_back(currentCol);

        } else if (stackTop == "ColumnList'") {
            // �����ݹ鲿�֣������ŷָ��ĺ����У�
            parseColumnListPrime();
            if (!symStack.empty() && symStack.top() == COMMA) {
                // ƥ�䶺�ţ�����������һ��
                match(COMMA);
                symStack.pop();
                // cerr << "[ColumnList] ƥ�䶺�ţ�׼��������һ��" << endl;
            } else {
                // �ղ���ʽ�����������ţ���������ǰColumnList'�������ݹ�
                symStack.pop();
                parsingActive = false;
            }

        } else if (stackTop == "ColumnList") {
            // ��ֹ©��������������ColumnList����
            symStack.pop();

        } else {
            // ������ط��ţ��������ţ�����������
            parsingActive = false;
        }
    }

    // �ؼ��޸���ǿ���������б���ز������ţ�ȷ��ջ����ColumnList/ColumnList'��
    while (!symStack.empty()) {
        string top = symStack.top();
        if (top == "ColumnList" || top == "ColumnList'") {
            symStack.pop();
            // cerr << "[ColumnList] �����������: " << top << endl;
        } else {
            break;
        }
    }

    return columns;
}

//���������������ݹ鲿��
/**
 * ������ѯ���б�ݹ鲿�֣�SelectColumns' �� , IDENTIFIER SelectColumns' | ��
 */
void Parser::parseSelectColumnsPrime() {
    // ���ɵ�ǰToken�ı�ʶ�����ڲ�ѯԤ���
    string currentTokenKey = (currentToken.type == TokenType::DELIMITER && currentToken.value == ",")
        ? "," : (currentToken.type == TokenType::KEYWORD && currentToken.value == "FROM") ? "FROM" : currentToken.value;

    // ����Ԥ����ѯ��
    string tableKey = "SelectColumns'|" + currentTokenKey;
    if (predictTable.find(tableKey) == predictTable.end()) {
        throw runtime_error("��ƥ���SelectColumns'����ʽ��" + tableKey);
    }
    vector<string> production = predictTable[tableKey];

    // ����ѹ�����ʽ���ţ�ջ����ȳ���
    for (auto it = production.rbegin(); it != production.rend(); ++it) {
        if (!it->empty()) {
            symStack.push(*it);
        }
    }
}

/**
 * ����ֵ�б�ݹ鲿�֣�ValueList' �� , CONSTANT ValueList' | ��
 */
void Parser::parseValueListPrime() {
    // ���ɵ�ǰToken�ı�ʶ�����ڲ�ѯԤ���
    string currentTokenKey = (currentToken.type == TokenType::DELIMITER && currentToken.value == ",")
        ? "," : (currentToken.type == TokenType::DELIMITER && currentToken.value == ")") ? ")" : currentToken.value;

    // ����Ԥ����ѯ��
    string tableKey = "ValueList'|" + currentTokenKey;
    if (predictTable.find(tableKey) == predictTable.end()) {
        throw runtime_error("��ƥ���ValueList'����ʽ��" + tableKey);
    }
    vector<string> production = predictTable[tableKey];

    // ����ѹ�����ʽ���ţ�ջ����ȳ���
    for (auto it = production.rbegin(); it != production.rend(); ++it) {
        if (!it->empty()) {
            symStack.push(*it);
        }
    }
}

/**
 * �������б�ݹ鲿�֣�ColumnList' �� , Column ColumnList' | ��
 */
void Parser::parseColumnListPrime() {
    // ���ɵ�ǰToken�ı�ʶ�����ڲ�ѯԤ���
    string currentTokenKey = (currentToken.type == TokenType::DELIMITER && currentToken.value == ",")
        ? "," : (currentToken.type == TokenType::DELIMITER && currentToken.value == ")") ? ")" : currentToken.value;

    // ����Ԥ����ѯ��
    string tableKey = "ColumnList'|" + currentTokenKey;
    if (predictTable.find(tableKey) == predictTable.end()) {
        throw runtime_error("��ƥ���ColumnList'����ʽ��" + tableKey);
    }
    vector<string> production = predictTable[tableKey];

    // ����ѹ�����ʽ���ţ�ջ����ȳ���
    for (auto it = production.rbegin(); it != production.rend(); ++it) {
        if (!it->empty()) {
            symStack.push(*it);
        }
    }
}

/**
 * ����WHERE�Ӿ䣺WhereClause �� WHERE Condition | ��
 * @return ��ѡ��Condition������WHEREʱ�������������򷵻ؿգ�
 */
optional<Condition> Parser::parseWhereClause() {
    parseNonTerminal("WhereClause");

    // ƥ��WHERE�ؼ��֣������ڣ�
    if (!symStack.empty() && symStack.top() == "KEYWORD(WHERE)") {
        match("KEYWORD(WHERE)");
        symStack.pop();
    }

    // �����������ʽ��Condition �� IDENTIFIER OPERATOR CONSTANT��
    if (!symStack.empty() && symStack.top() == "Condition") {
        Condition cond;
        parseNonTerminal("Condition");

        // 1. ƥ�������������������� "age"��
        cond.column = currentToken.value;
        match("IDENTIFIER");
        symStack.pop();
        // cerr << "[WhereClause] ������������: " << cond.column << endl;

        // 2. ƥ��Ƚ���������� ">"��"<"��
        cond.op = currentToken.value;
        match("OPERATOR");
        symStack.pop();
        // cerr << "[WhereClause] ���������: " << cond.op << endl;

        // 3. ƥ������ֵ���� "20"��"'Alice'"��
        cond.value = currentToken.value;
        match("CONSTANT");
        symStack.pop();
        // cerr << "[WhereClause] ��������ֵ: " << cond.value << endl;

        return cond;
    }

    // ��WHERE�Ӿ䣬���ؿ�
    return nullopt;
}

//����ӿ�
/**
 * ���Ľ����ӿڣ�ִ���﷨����������AST
 * @return AST���ڵ㣨�ɹ�ʱΪ��������AST��ʧ��ʱΪnullptr��
 */
unique_ptr<ASTNode> Parser::parse() {
    try {
        // ��֤ջ��ʼ���Ƿ���ȷ��ջ��ӦΪ��ʼ����Prog��
        if (symStack.empty() || symStack.top() != "Prog") {
            throw runtime_error("�﷨ջ��ʼ������ջ��ӦΪ'Prog'");
        }
        // ��ʼ�����������
        unique_ptr<ASTNode> result = parseProg();

        // �ؼ��Ż���ˢ��cerr���棬ȷ��������־����������ٴ�ӡAST���
        // cerr.flush();
        return result;
    } catch (const exception& e) {
        // ������������е��쳣����ӡ������Ϣ�͵�ǰ״̬
        // cerr << "\n[��������] " << e.what() << endl;
        // debugState();
        // cerr.flush(); // �쳣ʱҲˢ����־
        return nullptr;
    }
}

//AST��ӡ��������������
void printAST(ASTNode* ast) {
    if (!ast) {
        cout << "AST�ڵ�Ϊ�գ�����ʧ�ܣ�" << endl;
        return;
    }

    // ����AST�ڵ����ͣ���ӡ��Ӧ�ṹ
    if (auto selectAst = dynamic_cast<SelectAST*>(ast)) {
        cout << "SelectAST��SELECT��䣩:" << endl;
        cout << "  ��ѯ��: ";
        for (size_t i = 0; i < selectAst->columns.size(); ++i) {
            if (i > 0) cout << " ";
            cout << selectAst->columns[i];
        }
        cout << endl;
        cout << "  Ŀ���: " << selectAst->tableName << endl;
        if (selectAst->condition.has_value()) {
            auto& cond = selectAst->condition.value();
            cout << "  WHERE����: " << cond.column << " " << cond.op << " " << cond.value << endl;
        } else {
            cout << "  WHERE����: ��" << endl;
        }
    } else if (auto insertAst = dynamic_cast<InsertAST*>(ast)) {
        cout << "InsertAST��INSERT��䣩:" << endl;
        cout << "  Ŀ���: " << insertAst->tableName << endl;
        cout << "  ����ֵ: " << endl;
        for (const auto& val : insertAst->values) {
            cout << "    - " << val << endl;
        }
    } else if (auto createAst = dynamic_cast<CreateTableAST*>(ast)) {
        cout << "CreateTableAST��CREATE TABLE��䣩:" << endl;
        cout << "  ����: " << createAst->tableName << endl;
        cout << "  �ж���: " << endl;
        for (const auto& col : createAst->columns) {
            cout << "    - ����: " << col.name << ", ����: " << col.type << endl;
        }
    } else {
        cout << "δ֪AST�ڵ�����" << endl;
    }
}