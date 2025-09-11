#include <iostream>
#include "../include/compiler/Lexer.h"
#include "../include/common/Token.h"
#include "../include/common/Value.h"
#include "../include/common/Tuple.h"
#include "../include/compiler/Parser.h"
#include "../include/compiler/AST.h"


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



    // //���Դʷ�������
    // //�����õ�SQL���
    // string sql = "SELECT name, age FROM Students WHERE age > 20;";
    // //�����ʷ�������������SQL���
    // Lexer lexer(sql);
    // //��ȡ����Token
    // vector<Token> tokens = lexer.getAllTokens();
    //
    // //��������ӡÿ��Token����Ϣ
    // for (const auto& token : tokens) {
    //     cout << "Type: ";
    //     switch (token.type) {
    //     case TokenType::KEYWORD:
    //         cout << "KEYWORD (1), ";
    //         break;
    //     case TokenType::IDENTIFIER:
    //         cout << "IDENTIFIER (2), ";
    //         break;
    //     case TokenType::CONSTANT:
    //         cout << "CONSTANT (3), ";
    //         break;
    //     case TokenType::OPERATOR:
    //         cout << "OPERATOR (4), ";
    //         break;
    //     case TokenType::DELIMITER:
    //         cout << "DELIMITER (5), ";
    //         break;
    //     case TokenType::ERROR:
    //         cout << "ERROR (6), ";
    //         break;
    //     case TokenType::EOF_TOKEN:
    //         cout << "EOF (7), ";
    //         break;
    //     }
    //     cout << "Value: " << token.value
    //          << ", Line: " << token.line
    //          << ", Column: " << token.column << endl;
    // }



    // ���� SELECT ���
    string sql1 = "SELECT name, age FROM Students WHERE age > 20;";
    Lexer lexer1(sql1);
    Parser parser1(lexer1);
    auto ast1 = parser1.parse();
    cout << "=== Parsing SELECT statement ===" << endl;
    printAST(ast1.get());
    cout << endl;

    // ���� CREATE TABLE ���
    string sql2 = "CREATE TABLE Students(name STRING, age INT);";
    Lexer lexer2(sql2);
    Parser parser2(lexer2);
    auto ast2 = parser2.parse();
    cout << "=== Parsing CREATE TABLE statement ===" << endl;
    printAST(ast2.get());
    cout << endl;

    // ���� INSERT ���
    string sql3 = "INSERT INTO Students VALUES(Alice, 20);";
    Lexer lexer3(sql3);
    Parser parser3(lexer3);
    auto ast3 = parser3.parse();
    cout << "=== Parsing INSERT statement ===" << endl;
    printAST(ast3.get());

    return 0;
}