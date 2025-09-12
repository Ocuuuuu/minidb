//
// Created by tang_ on 2025/9/9.
//
#pragma once

#include <string>
#include <stdexcept>
#include "compiler/AST.h"
#include "engine/catalog/catalog_manager.h"

using namespace std;

namespace minidb {

    // ��������쳣�࣬�����׳�������������еĴ���
    class SemanticError : public runtime_error {
    public:
        explicit SemanticError(const string& msg) : runtime_error(msg) {}
    };

    // ����������࣬�����AST����������
    class SemanticAnalyzer {
    private:
        // Ŀ¼���������ã����ڻ�ȡ���Ԫ����
        CatalogManager& catalog_manager_;

    public:
        // ���캯������ʼ��Ŀ¼����������
        explicit SemanticAnalyzer(CatalogManager& catalog_manager)
            : catalog_manager_(catalog_manager) {}

        // ���������ڷ���������AST�ڵ����͵��ò�ͬ�ķ�������
        void analyze(ASTNode* ast);

    private:
        // ����CREATE TABLE��������
        void analyzeCreateTable(CreateTableAST* ast);
        // ����INSERT��������
        void analyzeInsert(InsertAST* ast);
        // ����SELECT��������
        void analyzeSelect(SelectAST* ast);
        // ���ֵ���������Ƿ�ƥ��
        bool checkValueMatchType(const string& value, const string& type);
    };

} // namespace minidb