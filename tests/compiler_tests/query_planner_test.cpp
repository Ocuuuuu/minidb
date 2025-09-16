//
// Created by tang_ on 2025/9/16.
//

#include <iostream>
#include "../catch2/catch_amalgamated.hpp"
#include "../../include/compiler/AST.h"
#include "../../include/compiler/QueryPlanner.h"

using json = nlohmann::json;
using namespace minidb;

// 辅助函数：打印 JSON 计划
void printPlan(const json& plan) {
    std::cout << "Generated plan:\n" << plan.dump(2) << std::endl;
}

TEST_CASE("QueryPlanner - CREATE TABLE", "[query_planner][create]") {
    QueryPlanner planner;

    SECTION("Simple CREATE TABLE") {
        CreateTableAST createTableAst;
        createTableAst.tableName = "students";
        createTableAst.columns = {{"id", "int"}, {"name", "varchar"}};

        json plan = planner.generatePlan(&createTableAst);
        printPlan(plan);

        REQUIRE(plan["type"] == "CreateTable");
        REQUIRE(plan["tableName"] == "students");
        REQUIRE(plan["columns"].size() == 2);
        REQUIRE(plan["columns"][0]["name"] == "id");
        REQUIRE(plan["columns"][0]["type"] == "int");
    }
}

TEST_CASE("QueryPlanner - INSERT", "[query_planner][insert]") {
    QueryPlanner planner;

    SECTION("Simple INSERT") {
        InsertAST insertAst;
        insertAst.tableName = "students";
        insertAst.values = {"1", "'Alice'"};

        json plan = planner.generatePlan(&insertAst);
        printPlan(plan);

        REQUIRE(plan["type"] == "Insert");
        REQUIRE(plan["tableName"] == "students");
        REQUIRE(plan["values"].size() == 2);
        REQUIRE(plan["values"][0] == "1");
    }
}

TEST_CASE("QueryPlanner - SELECT", "[query_planner][select]") {
    QueryPlanner planner;

    SECTION("SELECT without WHERE") {
        SelectAST selectAst;
        selectAst.tableName = "students";
        selectAst.columns = {"id", "name"};

        json plan = planner.generatePlan(&selectAst);
        printPlan(plan);

        REQUIRE(plan["type"] == "Project");
        REQUIRE(plan["columns"] == std::vector<std::string>{"id", "name"});
        REQUIRE(plan["input"]["type"] == "SeqScan");
        REQUIRE(plan["input"]["tableName"] == "students");
    }

    SECTION("SELECT with WHERE") {
        SelectAST selectAst;
        selectAst.tableName = "students";
        selectAst.columns = {"id"};
        selectAst.condition = Condition{"id", "=", "1"};

        json plan = planner.generatePlan(&selectAst);
        printPlan(plan);

        REQUIRE(plan["type"] == "Project");
        REQUIRE(plan["input"]["type"] == "Filter");
        REQUIRE(plan["input"]["condition"]["column"] == "id");
        REQUIRE(plan["input"]["condition"]["op"] == "=");
        REQUIRE(plan["input"]["condition"]["value"] == "1");
    }
}

TEST_CASE("QueryPlanner - DELETE", "[query_planner][delete]") {
    QueryPlanner planner;

    SECTION("DELETE with WHERE") {
        DeleteAST deleteAst;
        deleteAst.tableName = "students";
        deleteAst.condition = Condition{"id", "=", "1"};

        json plan = planner.generatePlan(&deleteAst);
        printPlan(plan);

        REQUIRE(plan["type"] == "Delete");
        REQUIRE(plan["input"]["type"] == "Filter");
        REQUIRE(plan["input"]["condition"]["column"] == "id");
        REQUIRE(plan["input"]["condition"]["op"] == "=");
        REQUIRE(plan["input"]["condition"]["value"] == "1");
    }

    SECTION("DELETE without WHERE (should throw)") {
        DeleteAST deleteAst;
        deleteAst.tableName = "students";

        REQUIRE_THROWS_AS(planner.generatePlan(&deleteAst), std::runtime_error);
    }
}

TEST_CASE("QueryPlanner - Invalid AST", "[query_planner][error]") {
    QueryPlanner planner;

    SECTION("Null AST") {
        REQUIRE_THROWS_AS(planner.generatePlan(nullptr), std::runtime_error);
    }

    SECTION("Unsupported AST type") {
        // 假设有一个未支持的 AST 类型
        struct UnsupportedAST : public ASTNode {};
        UnsupportedAST unsupportedAst;

        REQUIRE_THROWS_AS(planner.generatePlan(&unsupportedAst), std::runtime_error);
    }
}

