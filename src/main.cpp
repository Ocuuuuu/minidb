#include <iostream>
#include "../include/compiler/Lexer.h"
#include "../include/common/Token.h"
#include "../include/common/Value.h"
#include "../include/common/Tuple.h"

using namespace std;

int main() {
    // // ����Value��
    // minidb::Value int_val(42);
    // minidb::Value str_val("Hello");
    // minidb::Value bool_val(true);
    //
    // std::cout << "Integer: " << int_val << std::endl;
    // std::cout << "String: " << str_val << std::endl;
    // std::cout << "Boolean: " << bool_val << std::endl;
    //
    // // ����Tuple��
    // std::vector<minidb::Value> values = {int_val, str_val, bool_val};
    // minidb::Tuple tuple(values);
    //
    // std::cout << "Tuple: " << tuple << std::endl;
    // std::cout << "First value: " << tuple.getValue(0) << std::endl;


    //�����õ�SQL���
    string sql = "SELECT name, age FROM Students WHERE age > 20;";
    //�����ʷ�������������SQL���
    Lexer lexer(sql);
    //��ȡ����Token
    vector<Token> tokens = lexer.getAllTokens();

    //��������ӡÿ��Token����Ϣ
    for (const auto& token : tokens) {
        cout << "Type: ";
        switch (token.type) {
        case TokenType::KEYWORD:
            cout << "KEYWORD (1), ";
            break;
        case TokenType::IDENTIFIER:
            cout << "IDENTIFIER (2), ";
            break;
        case TokenType::CONSTANT:
            cout << "CONSTANT (3), ";
            break;
        case TokenType::OPERATOR:
            cout << "OPERATOR (4), ";
            break;
        case TokenType::DELIMITER:
            cout << "DELIMITER (5), ";
            break;
        case TokenType::ERROR:
            cout << "ERROR (6), ";
            break;
        case TokenType::EOF_TOKEN:
            cout << "EOF (7), ";
            break;
        }
        cout << "Value: " << token.value
             << ", Line: " << token.line
             << ", Column: " << token.column << endl;
    }

    return 0;
}