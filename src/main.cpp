#include <iostream>
#include "lexer/Lexer.h"

// ������������ӡToken�б�������֤�����
void PrintTokens(const std::vector<Token>& tokens) {
    std::cout << "SQL�ʷ��������:\n";
    std::cout << "----------------------------------------\n";
    for (const auto& token : tokens) {
        // ת��TokenTypeΪ�ַ����������Ķ���
        std::string type_str;
        switch (token.type) {
        case TokenType::KEYWORD: type_str = "�ؼ���"; break;
        case TokenType::ID:      type_str = "��ʶ��"; break;
        case TokenType::NUMBER:  type_str = "����";   break;
        case TokenType::OP:      type_str = "�����"; break;
        case TokenType::RANGE:   type_str = "�ָ���"; break;
        default:                 type_str = "δ֪";   break;
        }
        // ���Token��Ϣ
        std::cout << "����: " << type_str
                  << " | ����: " << token.value
                  << " | λ��: ��" << token.line << " ��" << token.column << "\n";
    }
}

int main() {
    // ����SQL���
    std::string test_sql = R"(
        CREATE TABLE Students (name STRING, age INT);
        INSERT INTO Students VALUES ("Tom", 22);
        SELECT name, age FROM Students WHERE age > 20;
    )";

    // ִ�дʷ�����
    Lexer lexer(test_sql);
    std::vector<Token> tokens = lexer.Tokenize();

    // ��ӡ���
    PrintTokens(tokens);

    return 0;
}