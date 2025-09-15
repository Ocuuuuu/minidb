#include "../../include/compiler/Lexer.h"
#include <cctype>    // �����ж��ַ����ͣ���ĸ/����/�ո�
#include <algorithm> // ���ڲ��ҹؼ���
#include <utility>

// ���캯������ʼ���ʷ���������SQL�ַ�����λ�á��кš��кţ���ʼ���ؼ��ּ���
Lexer::Lexer(std::string s) : sql(std::move(s)), pos(0), line(1), column(0) {
    initKeywords();
}

// ��ʼ���ؼ��ּ��ϣ�����SQL���ùؼ��֣����ִ�Сд��
void Lexer::initKeywords() {
    keywords = {
        "SELECT", "FROM", "WHERE", "CREATE", "TABLE",
        "INSERT", "INTO", "VALUES", "INT", "INTEGER",
        "STRING", "VARCHAR", "FLOAT", "DOUBLE",
        "int", "integer", "string", "varchar", "float", "double"
    };
}

// �����հ��ַ����ո��Ʊ�������з������������кź��к�
void Lexer::skipWhitespace() {
    while (pos < sql.size()) {
        char c = sql[pos];
        if (c == ' ' || c == '\t') {
            // �ո���Ʊ����ֻ�����к�
            column++;
            pos++;
        } else if (c == '\n') {
            // ���з����к�+1���к�����Ϊ0
            line++;
            pos++;
            column = 0;
        } else {
            // �ǿհ��ַ���ֹͣ����
            break;
        }
    }
}

// �����ʶ���͹ؼ��֣�����ĸ���»��߿�ͷ�������ɸ���ĸ�����ֻ��»��ߣ�
Token Lexer::handleIdentifierOrKeyword() {
    int startColumn = column;  // ��¼��ʼ�кţ����ڴ���λ��
    size_t startPos = pos;     // ��¼��ʼλ�ã����ڽ�ȡ�ַ�����

    // �����ַ�ֱ����������ĸ/����/�»���
    while (pos < sql.size() && (isalnum(sql[pos]) || sql[pos] == '_')) {
        pos++;
        column++;
    }

    // ��ȡ����ʼλ�õ���ǰλ�õ��ַ���
    std::string word = sql.substr(startPos, pos - startPos);

    // �ж��Ƿ�Ϊ�ؼ��֣��ڹؼ��ּ�������Ϊ�ؼ��֣�����Ϊ��ʶ����
    if (keywords.find(word) != keywords.end()) {
        return Token(TokenType::KEYWORD, word, line, startColumn);
    } else {
        return Token(TokenType::IDENTIFIER, word, line, startColumn);
    }
}

// ����������չ֧�֣�����������������˫�����ַ�����
Token Lexer::handleConstant() {
    int startColumn = column;
    size_t startPos = pos;

    // 1. �����ַ���������֧�ֵ�/˫���ź�ת�壩
    if (sql[pos] == '"' || sql[pos] == '\'') {
        char quoteChar = sql[pos];
        pos++;
        column++;

        while (pos < sql.size()) {
            if (sql[pos] == '\\' && pos + 1 < sql.size()) {
                pos += 2;
                column += 2;
            } else if (sql[pos] != quoteChar) {
                pos++;
                column++;
            } else {
                break;
            }
        }

        if (pos < sql.size() && sql[pos] == quoteChar) {
            pos++;
            column++;
            std::string str = sql.substr(startPos + 1, pos - startPos - 2);

            // ����ת������
            size_t escPos = 0;
            while (escPos < str.size()) {
                if (str[escPos] == '\\' && escPos + 1 < str.size()) {
                    char nextChar = str[escPos + 1];
                    if (nextChar == '"' || nextChar == '\'' || nextChar == '\\') {
                        str.replace(escPos, 2, 1, nextChar);
                        escPos += 1;
                    } else {
                        escPos += 1;
                    }
                } else {
                    escPos += 1;
                }
            }

            return Token(TokenType::CONSTANT, str, line, startColumn);
        } else {
            return Token(TokenType::ERROR, "Unclosed string (missing " + std::string(1, quoteChar) + ")", line, startColumn);
        }
    }

    // 2. ������ֵ������ǿ��������ȡ��
    else if (isdigit(static_cast<unsigned char>(sql[pos])) || sql[pos] == '.') {
        size_t numStart = pos;
        // ��ȡ�������������ֺ�С���㣨����ǰ��ֹ��
        while (pos < sql.size()) {
            char c = sql[pos];
            if (isdigit(static_cast<unsigned char>(c)) || c == '.') {
                pos++;
                column++;
            } else {
                break;
            }
        }

        // ��ȡ��������
        std::string numStr = sql.substr(numStart, pos - numStart);
        int dotCount = std::count(numStr.begin(), numStr.end(), '.');
        bool hasDigit = std::any_of(numStr.begin(), numStr.end(),
                                  [](unsigned char c) { return std::isdigit(c); });

        // �Ϸ����ж�
        if (dotCount > 1 || !hasDigit) {
            return Token(TokenType::ERROR, "Invalid number format (" + numStr + ")", line, startColumn);
        }

        return Token(TokenType::CONSTANT, numStr, line, startColumn);
    }

    // 3. δ֪��������
    return Token(TokenType::ERROR, "Unknown constant type", line, startColumn);
}

// ���������
Token Lexer::handleOperator() {
    int startColumn = column;
    char current = sql[pos];

    // �����ж�˫�ַ��������˳�����Ҫ��
    if (pos + 1 < sql.size()) {
        char next = sql[pos + 1];
        std::string op_str;
        op_str += current;
        op_str += next;

        // ��ȷ�г�����˫�ַ��������ȷ������
        const std::unordered_set<std::string> double_ops = {"==", "!=", ">=", "<="};
        if (double_ops.count(op_str)) {
            pos += 2;
            column += 2;
            return Token(TokenType::OPERATOR, op_str, line, startColumn);
        }
    }

    // �����ַ������
    const std::unordered_set<char> single_ops = {'+', '-', '*', '/', '%', '=', '>', '<', '!'};
    if (single_ops.count(current)) {
        pos++;
        column++;
        return Token(TokenType::OPERATOR, std::string(1, current), line, startColumn);
    }

    // ��Ч�����
    pos++;
    column++;
    return Token(TokenType::ERROR, "Unrecognized operator (" + std::string(1, current) + ")", line, startColumn);
}

// �����������š��ֺš����ŵȣ�
Token Lexer::handleDelimiter() {
    int startColumn = column;
    // size_t startPos = pos;

    if (pos < sql.size()) {
        char delim = sql[pos];
        pos++;
        column++;
        std::string delimStr(1, delim);
        return Token(TokenType::DELIMITER, delimStr, line, startColumn);
    }

    // �Ƿ���������ش���
    return Token(TokenType::ERROR, "Invalid delimiter", line, startColumn);
}

Token Lexer::nextToken() {
    skipWhitespace();

    if (pos >= sql.size()) {
        return Token(TokenType::EOF_TOKEN, "", line, column);
    }

    char c = sql[pos];

    if (isalpha(c) || c == '_') {
        return handleIdentifierOrKeyword();
    }
    // �ؼ��޸������� c == '.'��ȷ��С������볣������
    else if (isdigit(c) || c == '.' || c == '"' || c == '\'') {
        return handleConstant();
    }
    else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
             c == '=' || c == '>' || c == '<' || c == '!') {
        return handleOperator();
             }
    else if (c == ',' || c == ';' || c == '(' || c == ')' || c == '{' || c == '}') { // ���� } ���
        return handleDelimiter();
    }
    else {
        // �޸������ġ�Ӣ�Ĵ�����Ϣ
        pos++;
        column++;
        return Token(TokenType::ERROR, "Unrecognized character (" + std::string(1, c) + ")", line, column - 1);
    }
}

// ��ȡ����Token��ֱ������EOF��
vector<Token> Lexer::getAllTokens() {
    vector<Token> tokens;
    // ֱ�ӻ�ȡ��һ��Token������Ĭ�Ϲ���
    Token current_token = nextToken();

    // �������з�EOF��Token����ӵ��б���
    while (current_token.type != TokenType::EOF_TOKEN) {
        tokens.push_back(current_token);
        current_token = nextToken();  // ��ȡ��һ��Token
    }

    // ������EOF_TOKEN�����Token���н���
    tokens.push_back(current_token);
    return tokens;
}
