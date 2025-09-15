#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "AST.h"
#include "../engine/catalog/catalog_manager.h"
#include "../common/Exception.h"
#include "../common/Types.h"
#include <unordered_set>

#include "engine/catalog/schema.h"

namespace minidb {

/**
 * ��������쳣��
 */
class SemanticError : public std::runtime_error {
public:
    explicit SemanticError(const std::string& message) : std::runtime_error(message) {}
};

/**
 * �����������
 * ������﷨�������ɵ�AST����������
 * ���������ͼ�顢���ƽ��������������
 */
class SemanticAnalyzer {
public:
    /**
     * ���캯��
     * @param catalog_manager Ŀ¼���������ã����ڲ�ѯ�������Ϣ
     */
    explicit SemanticAnalyzer(CatalogManager& catalog_manager) 
        : catalog_manager_(catalog_manager) {}

    /**
     * ���������ں���
     * ����AST�ڵ����ͷַ�����Ӧ�ķ�������
     * @param ast AST���ڵ�
     * @throws SemanticError �������ʧ��ʱ�׳�
     */
    void analyze(ASTNode* ast);



private:
    CatalogManager& catalog_manager_;  // Ŀ¼����������

    /**
     * ����CREATE TABLE���
     * ��飺���Ƿ��Ѵ��ڡ������Ƿ��ظ����������Ƿ�Ϸ�
     * @param ast CreateTableAST�ڵ�
     * @throws SemanticError ����ʧ��ʱ�׳�
     */
    void analyzeCreateTable(CreateTableAST* ast);

    /**
     * ����INSERT���
     * ��飺���Ƿ���ڡ�����ֵ�������������Ƿ�ƥ�䡢ֵ�������������Ƿ�ƥ��
     * @param ast InsertAST�ڵ�
     * @throws SemanticError ����ʧ��ʱ�׳�
     */
    void analyzeInsert(InsertAST* ast);

    /**
     * ����SELECT���
     * ��飺���Ƿ���ڡ���ѯ���Ƿ���ڡ��������ʽ�Ƿ�Ϸ�
     * @param ast SelectAST�ڵ�
     * @throws SemanticError ����ʧ��ʱ�׳�
     */
    void analyzeSelect(SelectAST* ast);

    /**
     * ����DELETE���
     * ��飺���Ƿ���ڡ�WHERE�����Ƿ�Ϸ�
     * @param ast DeleteAST�ڵ�
     * @throws SemanticError ����ʧ��ʱ�׳�
     */
    void analyzeDelete(DeleteAST* ast);

    /**
     * ����WHERE�������ʽ
     * ��飺�������Ƿ���ڡ�����ֵ�����Ƿ���������ƥ�䡢������Ƿ�Ϸ�
     * @param cond ��������
     * @param schema ��ṹ
     * @param table_name ���������ڴ�����Ϣ��
     * @throws SemanticError ����ʧ��ʱ�׳�
     */
    void analyzeCondition(const Condition& cond, const Schema& schema, const std::string& table_name);

    /**
     * ���������Ƿ�Ϸ�
     * @param op �����
     * @return �Ϸ�����true�����򷵻�false
     */
    bool isValidOperator(const std::string& op);

    /**
     * �����������������͵ļ�����
     * @param op �����
     * @param type ��������
     * @return ���ݷ���true�����򷵻�false
     */
    bool isOperatorCompatibleWithType(const std::string& op, TypeId type);

    /**
     * ���ֵ�������Ƿ�ƥ��
     * @param value Ҫ����ֵ
     * @param type Ŀ�����ͣ�"INT"��"STRING"��"FLOAT"��"BOOLEAN"��
     * @return ƥ�䷵��true�����򷵻�false
     */
    bool checkValueMatchType(const std::string& value, const std::string& type);
};

} // namespace minidb

#endif // SEMANTIC_ANALYZER_H