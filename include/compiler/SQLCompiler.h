//
// Created by tang_ on 2025/9/16.
//

#ifndef MINIDB_SQLCOMPILER_H
#define MINIDB_SQLCOMPILER_H

#include "compiler/Lexer.h"
#include "compiler/Parser.h"
#include "compiler/SemanticAnalyzer.h"
#include "compiler/QueryPlanner.h"
#include "compiler/AST.h"
#include "engine/catalog/catalog_manager.h"

namespace minidb
{
    class SQLCompiler {
    public:
        explicit SQLCompiler(CatalogManager& catalog);
        std::unordered_map<std::string, std::string> compile(const std::string& sql);

    private:
        CatalogManager& catalogManager;
    };
}

#endif //MINIDB_SQLCOMPILER_H