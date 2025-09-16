#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "AST.h"
#include "../engine/catalog/catalog_manager.h"
#include "../common/Exception.h"
#include "../common/Types.h"
#include <unordered_set>

#include "engine/catalog/schema.h"

namespace minidb {

//��������쳣��
class SemanticError : public std::runtime_error {
public:
    explicit SemanticError(const std::string& message) : std::runtime_error(message) {}
};


//����������࣬������﷨�������ɵ�AST����������
class SemanticAnalyzer {
public:

    //���캯��
    explicit SemanticAnalyzer(CatalogManager& catalogManager)
           : catalog_manager_(catalogManager) {}

    //���������ں���������AST�ڵ����ͷַ�����Ӧ�ķ�������
    void analyze(ASTNode* ast);

private:
    CatalogManager& catalog_manager_;  // Ŀ¼����������

    //����CREATE TABLE���
    void analyzeCreateTable(CreateTableAST* ast);

    //����INSERT���
    void analyzeInsert(InsertAST* ast);

    //����SELECT���
    void analyzeSelect(SelectAST* ast);

    //����DELETE���
    void analyzeDelete(DeleteAST* ast);

    //����WHERE�������ʽ
    void analyzeCondition(const Condition& cond, const Schema& schema, const std::string& table_name);

    //���������Ƿ�Ϸ�
    bool isValidOperator(const std::string& op);

    //�����������������͵ļ�����
    bool isOperatorCompatibleWithType(const std::string& op, TypeId type);

    //���ֵ�������Ƿ�ƥ��
    bool checkValueMatchType(const std::string& value, const std::string& type);
};

} // namespace minidb

#endif // SEMANTIC_ANALYZER_H