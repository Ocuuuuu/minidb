#include "../../include/compiler/Lexer.h"
#include <cctype>   // �����ж��ַ����ͣ���ĸ/����/�ո�
#include <algorithm> // ���ڲ��ҹؼ���
#include <utility>

//���캯�� ��ʼ���ʷ���������SQL�ַ�����λ�á��кš��кţ���ʼ���ؼ��ּ���
Lexer::Lexer(string s):sql(std::move(s)),pos(0),line(1),column(0)
{
    initKeywords();
}

//��ʼ���ؼ��ּ���
void Lexer::initKeywords()
{
    keywords = {"SELECT", "FROM", "WHERE", "CREATE", "TABLE", "INSERT", "INTO", "VALUES"};
}


//�����հ��ַ����������кź��к�
void Lexer::skipWhitespace()
{
    while (pos<sql.size())
    {
        char c=sql[pos];
        if (c==' '||c=='\t')
        {
            column++;
            pos++;
        }else if (c=='\n')
        {
            line++;
            pos++;
            column=0;
        }else
        {
            break;
        }
    }
}

//�����ʶ���͹ؼ���
Token Lexer::handleIdentifierOrKeyword()
{
    int startColumn=column;
    size_t startPos=pos;
    //�����ַ� ֱ����������ĸ/����/�»���
    while (pos<sql.size()&&(isalnum(sql[pos]) || sql[pos]=='_'))
    {
        pos++;
        column++;
    }
    string word=sql.substr(startPos,pos-startPos);
    //�ж��Ƿ�Ϊ�ؼ���
    if (keywords.find(word)!=keywords.end())
    {
        return Token(TokenType::KEYWORD,word,line,startColumn);
    }else
    {
        return Token(TokenType::IDENTIFIER,word,line,startColumn);
    }
}

//������
Token Lexer::handleConstant()
{
    int startColumn=column;
    size_t startPos=pos;
    if (sql[pos]=='"')//�ַ���������˫���ſ�ʼ
    {
        pos++;
        column++;
        //�ҵ�˫���Ž�β
        while (pos<sql.size()&&(sql[pos]!='"'))
        {
            pos++;
            column++;
        }
        if (pos<sql.size()&&sql[pos]=='"'){
            //��ȷ�պ�
            pos++;
            column++;
            string str=sql.substr(startPos+1,pos-startPos-2);
            return Token(TokenType::CONSTANT,str,line,startColumn);
        }
        //δ��ȷ�պ�
        return Token(TokenType::ERROR,"δ�պ��ַ���",line,startColumn);
    }else if (isdigit(sql[pos]))
    {
        //��������
        while (pos<sql.size()&&isdigit(sql[pos]))
        {
            pos++;
            column++;
        }
        string num=sql.substr(startPos,pos-startPos);
        return Token(TokenType::CONSTANT,num,line,startColumn);
    }
    //�������ͳ���
    return Token(TokenType::IDENTIFIER,sql,line,startColumn);
}

//���������(���������)
Token Lexer::handleOperator()
{
    int startColumn = column;
    size_t startPos = pos;
    if (pos < sql.size()) {
        char op = sql[pos];
        pos++;
        column++;
        string opStr(1, op);
        return Token(TokenType::OPERATOR, opStr, line, startColumn);
    }
    return Token(TokenType::ERROR, "�Ƿ������", line, startColumn);
}


//������
Token Lexer::handleDelimiter()
{
    int startColumn = column;
    size_t startPos = pos;
    if (pos < sql.size()) {
        char delim = sql[pos];
        pos++;
        column++;
        string delimStr(1, delim);
        return Token(TokenType::DELIMITER, delimStr, line, startColumn);
    }
    return Token(TokenType::ERROR, "�Ƿ����", line, startColumn);
}

//��ȡ��һ��Token
Token Lexer::nextToken()
{
    skipWhitespace(); //�������հ��ַ�
    if (pos >= sql.size()) { //�����ַ���ĩβ������ EOF_TOKEN
        return Token(TokenType::EOF_TOKEN, "", line, column);
    }
    char c = sql[pos];
    //���ݵ�ǰ�ַ����ͣ�������Ӧ�Ĵ�����
    if (isalpha(c) || c == '_') {
        return handleIdentifierOrKeyword();
    } else if (isdigit(c) || c == '"') {
        return handleConstant();
    } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '=' || c == '>' || c == '<') {
        return handleOperator();
    } else if (c == ',' || c == ';' || c == '(' || c == ')') {
        return handleDelimiter();
    } else { //�޷�ʶ����ַ�����Ϊ����
        pos++;
        column++;
        return Token(TokenType::ERROR, string(1, c), line, column - 1);
    }
}
//��ȡ����Token ֱ������EOF_Token
vector<Token> Lexer::getAllTokens()
{
    vector<Token> tokens;
    Token token = nextToken();
    while (token.type != TokenType::EOF_TOKEN) {
        tokens.push_back(token);
        token = nextToken();
    }
    tokens.push_back(token); //����EOF_TOKEN
    return tokens;
}

