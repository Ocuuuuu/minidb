#include "Lexer.h"
#include <cctype>   // �����ж��ַ����ͣ���ĸ/����/�ո�
#include <algorithm> // ���ڲ��ҹؼ���

std::vector<Token> Lexer::Tokenize() {
    std::vector<Token> tokens;  // �洢����Token�б�
    int pos = 0;                // ��ǰ�ַ�λ��
    int line = 1;               // ��ǰ�к�
    int column = 1;             // ��ǰ�к�
    int sql_len = sql_.size();  // SQL��䳤��

    while (pos < sql_len) {
        char c = sql_[pos];

        // 1. �����ո�/���У������кź��кţ�
        if (isspace(c)) {
            if (c == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            pos++;
            continue;
        }

        // 2. ʶ���ʶ��/�ؼ��֣���ĸ/�»��߿�ͷ��
        if (isalpha(c) || c == '_') {
            int start_pos = pos;
            // ��ȡ��������ĸ/����/�»���
            while (pos < sql_len && (isalnum(sql_[pos]) || sql_[pos] == '_')) {
                pos++;
                column++;
            }
            // ��ȡ�ַ������ж��Ƿ�Ϊ�ؼ���
            std::string value = sql_.substr(start_pos, pos - start_pos);
            TokenType type = TokenType::ID;
            if (std::find(keywords_.begin(), keywords_.end(), value) != keywords_.end()) {
                type = TokenType::KEYWORD;
            }
            // ��ӵ�Token�б�
            tokens.emplace_back(type, value, line, column - (pos - start_pos));
            continue;
        }

        // 3. ʶ�����ֳ�����0-9��ͷ��
        if (isdigit(c)) {
            int start_pos = pos;
            // ��ȡ����������
            while (pos < sql_len && isdigit(sql_[pos])) {
                pos++;
                column++;
            }
            std::string value = sql_.substr(start_pos, pos - start_pos);
            tokens.emplace_back(TokenType::NUMBER, value, line, column - (pos - start_pos));
            continue;
        }

        // 4. ʶ���������>��<��=��!=��>=��<=��
        if (c == '>' || c == '<' || c == '=' || c == '!') {
            std::string value(1, c);
            // ����Ƿ�Ϊ˫�ַ����������>=��!=��
            if (pos + 1 < sql_len && sql_[pos + 1] == '=') {
                value += '=';
                pos++;
                column++;
            }
            tokens.emplace_back(TokenType::OP, value, line, column);
            pos++;
            column++;
            continue;
        }

        // 5. ʶ��ָ�����,��;��.��
        if (c == ',' || c == ';' || c == '.') {
            tokens.emplace_back(TokenType::RANGE, std::string(1, c), line, column);
            pos++;
            column++;
            continue;
        }

        // 6. δ֪���ţ���#��@��
        tokens.emplace_back(TokenType::UNKNOWN, std::string(1, c), line, column);
        pos++;
        column++;
    }

    return tokens;
}