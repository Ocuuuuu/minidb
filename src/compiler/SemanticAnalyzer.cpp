#include "../include/compiler/SemanticAnalyzer.h"
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <cctype>

using namespace minidb;
using namespace std;

/**
 * ���ַ���������ת��ΪTypeIdö��
 * ֧�ֶ������ͱ����ʹ�Сд��ʽ
 */
static TypeId stringToTypeId(const string& type_str) {
    // ת��ΪСд��ʵ�ִ�Сд������ƥ��
    string lower_type;
    transform(type_str.begin(), type_str.end(), back_inserter(lower_type), ::tolower);

    if (lower_type == "int" || lower_type == "integer") {
        return TypeId::INTEGER;
    } else if (lower_type == "string" || lower_type == "varchar") {
        return TypeId::VARCHAR;
    } else {
        return TypeId::INVALID;
    }
}

/**
 * ���������ں���
 * ����AST�ڵ����ͷַ�����Ӧ�ķ�������
 */
void SemanticAnalyzer::analyze(ASTNode* ast) {
    if (!ast) {
        throw SemanticError("AST�ڵ�Ϊ�գ��޷������������");
    }

    // ����AST�ڵ����ͽ��зַ�
    if (auto create_table_ast = dynamic_cast<CreateTableAST*>(ast)) {
        analyzeCreateTable(create_table_ast);
    } else if (auto insert_ast = dynamic_cast<InsertAST*>(ast)) {
        analyzeInsert(insert_ast);
    } else if (auto select_ast = dynamic_cast<SelectAST*>(ast)) {
        analyzeSelect(select_ast);
    } else {
        throw SemanticError("��֧�ֵ�AST�ڵ�����");
    }
}

// ������������أ�δʹ�ã��ɸ�����Ҫʵ�֣�
bool operator==(TypeId lhs, char* str);

/**
 * ����CREATE TABLE���
 * ��飺���Ƿ��Ѵ��ڡ������Ƿ��ظ����������Ƿ�Ϸ�
 */
void SemanticAnalyzer::analyzeCreateTable(CreateTableAST* ast) {
    const string& table_name = ast->tableName;

    // �����Ƿ��Ѵ���
    if (catalog_manager_.table_exists(table_name)) {
        throw SemanticError("�� '" + table_name + "' �Ѵ��ڣ��޷��ظ�����");
    }

    // ��������Ƿ��ظ�
    unordered_map<string, bool> col_name_map;
    for (const auto& col : ast->columns) {
        if (col_name_map.count(col.name)) {
            throw SemanticError("�� '" + table_name + "' �д����ظ�����: " + col.name);
        }
        col_name_map[col.name] = true;

        // ����������Ƿ�Ϸ�
        TypeId col_type = stringToTypeId(col.type);
        if (col_type == TypeId::INVALID) {
            throw SemanticError("�� '" + table_name + "' �д��ڷǷ�������: " +
                                col.type + "����֧��INT/INTEGER/STRING/VARCHAR��");
        }
    }

    // ������ṹ��Ϣ
    vector<MyColumn> schema_columns;
    for (const auto& ast_col : ast->columns) {
        TypeId col_type = stringToTypeId(ast_col.type);
        uint32_t col_length = 0;

        // ������������Ĭ�ϳ���
        if (col_type == TypeId::INTEGER) {
            col_length = 4; // 4�ֽ�����
        } else if (col_type == TypeId::VARCHAR) {
            col_length = 255; // Ĭ��255�ֽ��ַ���
        }

        schema_columns.emplace_back(ast_col.name, col_type, col_length, 0);
    }
    Schema schema(schema_columns);

    // ������
    if (!catalog_manager_.create_table(table_name, schema)) {
        throw SemanticError("������ '" + table_name + "' ʧ��");
    }
}

/**
 * ����INSERT���
 * ��飺���Ƿ���ڡ�����ֵ�������������Ƿ�ƥ�䡢ֵ�������������Ƿ�ƥ��
 */
void SemanticAnalyzer::analyzeInsert(InsertAST* ast) {
    const string& table_name = ast->tableName;
    const vector<string>& values = ast->values;

    // �����Ƿ����
    if (!catalog_manager_.table_exists(table_name)) {
        throw SemanticError("����ʧ�ܣ��� '" + table_name + "' ������");
    }

    // ��ȡ����Ϣ
    const TableInfo* table_info = catalog_manager_.get_table(table_name);
    if (!table_info) {
        throw SemanticError("��ȡ�� '" + table_name + "' ��Ϣʧ��");
    }
    const Schema& schema = table_info->get_schema();

    // ������ֵ�������������Ƿ�ƥ��
    if (values.size() != schema.get_column_count()) {
        throw SemanticError("����ʧ�ܣ��� '" + table_name + "' �� " +
                            to_string(schema.get_column_count()) + " �У��������� " +
                            to_string(values.size()) + " ��ֵ");
    }

    // ���ÿ��ֵ�������Ƿ����Ӧ�е�����ƥ��
    for (size_t i = 0; i < values.size(); ++i) {
        const MyColumn& col = schema.get_column(static_cast<uint32_t>(i));
        const string& value = values[i];

        // ��TypeIdת��Ϊ�ַ������������������ͼ��
        string type_str;
        switch (col.type) {
            case TypeId::INTEGER:
                type_str = "INT";
                break;
            case TypeId::VARCHAR:
                type_str = "STRING";
                break;
            default:
                type_str = "UNKNOWN";
                break;
        }

        if (!checkValueMatchType(value, type_str)) {
            throw SemanticError("����ʧ�ܣ��� '" + table_name + "' ���� '" + col.name +
                                "' ���Ͳ�ƥ�䣬ֵΪ '" + value + "'");
        }
    }
}

/**
 * ����SELECT���
 * ��飺���Ƿ���ڡ���ѯ���Ƿ���ڡ��������ʽ�Ƿ�Ϸ�
 */
void SemanticAnalyzer::analyzeSelect(SelectAST* ast) {
    const string& table_name = ast->tableName;
    const vector<string>& columns = ast->columns;

    // �����Ƿ����
    if (!catalog_manager_.table_exists(table_name)) {
        throw SemanticError("��ѯʧ�ܣ��� '" + table_name + "' ������");
    }

    // ��ȡ����Ϣ
    const TableInfo* table_info = catalog_manager_.get_table(table_name);
    if (!table_info) {
        throw SemanticError("��ȡ�� '" + table_name + "' ��Ϣʧ��");
    }
    const Schema& schema = table_info->get_schema();

    // ����ѯ�����Ƿ���ڣ�ͨ���*���⣩
    for (const string& col_name : columns) {
        if (col_name == "*") continue;

        try {
            schema.get_column_index(col_name);
        } catch (const out_of_range&) {
            throw SemanticError("��ѯʧ�ܣ��� '" + table_name + "' �в������� '" + col_name + "'");
        } catch (const runtime_error&) {
            throw SemanticError("��ѯʧ�ܣ��� '" + table_name + "' �в������� '" + col_name + "'");
        }
    }

    // ���WHERE������������ڣ�
    if (ast->condition.has_value()) {
        const Condition& cond = ast->condition.value();

        try {
            // ����������Ƿ����
            uint32_t col_idx = schema.get_column_index(cond.column);
            const   MyColumn& cond_col = schema.get_column(col_idx);

            // ��TypeIdת��Ϊ�ַ������������������ͼ��
            string type_str;
            switch (cond_col.type) {
                case TypeId::INTEGER:
                    type_str = "INT";
                    break;
                case TypeId::VARCHAR:
                    type_str = "STRING";
                    break;
                default:
                    type_str = "UNKNOWN";
                    break;
            }

            // �������ֵ���������Ƿ�ƥ��
            if (!checkValueMatchType(cond.value, type_str)) {
                throw SemanticError("��ѯʧ�ܣ������� '" + cond.column + "' ���Ͳ�ƥ�䣬ֵΪ '" + cond.value + "'");
            }
        } catch (const out_of_range&) {
            throw SemanticError("��ѯʧ�ܣ��� '" + table_name + "' �в����������� '" + cond.column + "'");
        } catch (const runtime_error&) {
            throw SemanticError("��ѯʧ�ܣ��� '" + table_name + "' �в����������� '" + cond.column + "'");
        }
    }
}

/**
 * ���ֵ�������Ƿ�ƥ��
 * @param value Ҫ����ֵ
 * @param type Ŀ�����ͣ�"INT" �� "STRING"��
 * @return ƥ�䷵��true�����򷵻�false
 */
bool SemanticAnalyzer::checkValueMatchType(const string& value, const string& type) {
    if (type == "INT") {
        // ����Ƿ�Ϊ�Ϸ�����
        if (value.empty()) return false;

        size_t start = 0;
        // ����������
        if (value[0] == '+' || value[0] == '-') {
            start = 1;
            // ����ֻ�з���
            if (value.size() == 1) return false;
        }

        // ���ʣ���ַ��Ƿ�������
        return all_of(value.begin() + start, value.end(), ::isdigit);
    } else if (type == "STRING") {
        // �ַ�����������ƥ�䣨������������Ӹ��ϸ�ļ�飬�����ţ�
        return true;
    }
    return false;
}