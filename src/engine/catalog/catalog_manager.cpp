#include "../include/engine/catalog/catalog_manager.h"
#include <algorithm> // 用于std::sort排序
#include <cctype>   // 用于字符处理
#include <sstream>  // 用于字符串流处理

namespace minidb {

    /**
     * @brief 创建新表
     * @param table_name 要创建的表名
     * @param schema 表的结构定义
     * @return true-创建成功, false-表已存在
     * @details 检查表名是否已存在，不存在则创建新的TableInfo对象并添加到目录中
     */
    bool CatalogManager::create_table(const std::string& table_name, const Schema& schema) {
        // 首先检查表是否已存在，避免重复创建
        if (table_exists(table_name)) {
            return false; // 表名已存在，创建失败
        }

        // 使用emplace直接在map中构造键值对，避免额外的拷贝操作
        // make_unique创建TableInfo对象，智能指针自动管理内存
        auto result = tables_.emplace(table_name, std::make_unique<TableInfo>(table_name, schema));

        // emplace返回pair<iterator, bool>，second表示是否插入成功
        return result.second;
    }

    /**
     * @brief 删除表
     * @param table_name 要删除的表名
     * @return true-删除成功, false-表不存在
     * @details 从目录中移除表信息，智能指针会自动释放相关内存
     */
    bool CatalogManager::drop_table(const std::string& table_name) {
        // erase方法返回删除的元素数量（0或1），大于0表示删除成功
        // 这种方式比先find再erase更简洁高效
        return tables_.erase(table_name) > 0;
    }

    /**
     * @brief 检查表是否存在
     * @param table_name 要检查的表名
     * @return true-表存在, false-表不存在
     * @details 在unordered_map中查找表名，判断是否存在
     */
    bool CatalogManager::table_exists(const std::string& table_name) const {
        // 使用find方法查找表名，如果找到则返回对应迭代器，否则返回end()
        return tables_.find(table_name) != tables_.end();
    }

    /**
     * @brief 获取表信息（非常量版本）
     * @param table_name 表名
     * @return TableInfo指针，如果表不存在则返回nullptr
     * @note 非常量版本允许修改返回的TableInfo，用于需要更新表信息的场景
     */
    TableInfo* CatalogManager::get_table(const std::string& table_name) {
        // 查找表名对应的迭代器
        auto it = tables_.find(table_name);

        // 如果找到，返回对应的TableInfo指针（使用get()获取原始指针）
        // 如果没找到，返回nullptr
        return it != tables_.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief 获取表信息（常量版本）
     * @param table_name 表名
     * @return TableInfo常量指针，如果表不存在则返回nullptr
     * @note 常量版本保证返回的Table信息不会被修改，用于只读操作
     */
    const TableInfo* CatalogManager::get_table(const std::string& table_name) const {
        // 与非常量版本类似，但返回常量指针确保数据不会被修改
        auto it = tables_.find(table_name);
        return it != tables_.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief 获取所有表名的列表
     * @return 按字母顺序排序的表名向量
     * @details 收集所有表名并进行排序，便于显示和处理
     */
    std::vector<std::string> CatalogManager::get_table_names() const {
        std::vector<std::string> names;

        // 预分配足够空间，避免多次重新分配内存
        names.reserve(tables_.size());

        // 使用结构化绑定遍历所有表，[name, _]表示只获取表名，忽略TableInfo指针
        for (const auto& [name, _] : tables_) {
            names.push_back(name); // 将表名添加到向量中
        }

        // 按字母顺序排序表名，便于用户查看
        std::sort(names.begin(), names.end());

        return names;
    }

        // ==================== 新增方法实现：AST集成 ====================
    /**
     * @brief 从AST创建新表
     * @param create_ast CREATE TABLE语句的AST节点
     * @return true-创建成功, false-表已存在或创建失败
     */
    bool CatalogManager::create_table_from_ast(const CreateTableAST& create_ast) {
        // 检查表是否已存在
        if (table_exists(create_ast.tableName)) {
            return false;
        }

        // 将AST中的列定义转换为Schema所需的Column格式
        std::vector<Column> schema_columns;
        schema_columns.reserve(create_ast.columns.size());

        for (const auto& ast_column : create_ast.columns) {
            try {
                // 转换类型字符串为TypeId
                TypeId type_id = convert_ast_type_to_typeid(ast_column.type);

                // 计算列长度
                uint32_t length = 0;
                if (type_id == TypeId::VARCHAR) {
                    length = calculate_varchar_length(ast_column.type);
                } else {
                    // 对于非VARCHAR类型，使用固定长度
                    switch (type_id) {
                        case TypeId::BOOLEAN: length = 1; break;
                        case TypeId::INTEGER: length = 4; break;
                        default: length = 4; // 默认长度
                    }
                }

                // 创建Column对象
                schema_columns.emplace_back(ast_column.name, type_id, length, 0);
            } catch (const std::exception& e) {
                // 类型转换失败，返回创建失败
                return false;
            }
        }

        // 创建Schema并使用原有方法创建表
        Schema schema(schema_columns);
        return create_table(create_ast.tableName, schema);
    }

    /**
     * @brief 验证INSERT语句的AST是否有效
     * @param insert_ast INSERT语句的AST节点
     * @return true-语句有效, false-表不存在或列不匹配
     */
    bool CatalogManager::validate_insert_ast(const InsertAST& insert_ast) const {
        // 检查表是否存在
        const TableInfo* table_info = get_table(insert_ast.tableName);
        if (!table_info) {
            return false;
        }

        // 获取表的Schema
        const Schema& schema = table_info->get_schema();

        // 检查值数量是否与列数量匹配
        if (insert_ast.values.size() != schema.get_column_count()) {
            return false;
        }

        // TODO: 可以添加更详细的类型验证
        return true;
    }

    /**
     * @brief 验证SELECT语句的AST是否有效
     * @param select_ast SELECT语句的AST节点
     * @return true-语句有效, false-表不存在或列不存在
     */
    bool CatalogManager::validate_select_ast(const SelectAST& select_ast) const {
        // 检查表是否存在
        const TableInfo* table_info = get_table(select_ast.tableName);
        if (!table_info) {
            return false;
        }

        // 获取表的Schema
        const Schema& schema = table_info->get_schema();

        // 检查查询的列是否存在
        if (select_ast.columns.size() == 1 && select_ast.columns[0] == "*") {
            // 通配符*，检查所有列
            return true;
        }

        for (const auto& column_name : select_ast.columns) {
            if (!schema.has_column(column_name)) {
                return false;
            }
        }

        // 检查WHERE条件中的列是否存在
        if (select_ast.condition.has_value()) {
            const auto& condition = select_ast.condition.value();
            if (!schema.has_column(condition.column)) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief 获取表的Schema信息（用于AST执行）
     * @param table_name 表名
     * @return Schema的共享指针，如果表不存在则返回nullptr
     */
    std::shared_ptr<Schema> CatalogManager::get_table_schema(const std::string& table_name) const {
        const TableInfo* table_info = get_table(table_name);
        if (!table_info) {
            return nullptr;
        }

        // 创建Schema的共享副本
        return std::make_shared<Schema>(table_info->get_schema());
    }

    /**
     * @brief 将AST中的字符串类型转换为TypeId枚举
     * @param type_str 类型字符串（如 "INT", "STRING", "BOOLEAN"）
     * @return 对应的TypeId枚举值
     * @throw std::invalid_argument 如果类型字符串无效
     */
    TypeId CatalogManager::convert_ast_type_to_typeid(const std::string& type_str) const {
        std::string upper_type;
        for (char c : type_str) {
            upper_type += std::toupper(c);
        }

        // 移除可能存在的长度说明（如 VARCHAR(255) -> VARCHAR）
        std::string base_type;
        size_t paren_pos = upper_type.find('(');
        if (paren_pos != std::string::npos) {
            base_type = upper_type.substr(0, paren_pos);
        } else {
            base_type = upper_type;
        }

        // 映射类型字符串到TypeId
        if (base_type == "INT" || base_type == "INTEGER") {
            return TypeId::INTEGER;
        } else if (base_type == "STRING" || base_type == "VARCHAR" || base_type == "TEXT") {
            return TypeId::VARCHAR;
        } else if (base_type == "BOOL" || base_type == "BOOLEAN") {
            return TypeId::BOOLEAN;
        } else {
            throw std::invalid_argument("Unsupported type: " + type_str);
        }
    }

    /**
     * @brief 计算VARCHAR类型的合适长度
     * @param type_str 类型字符串（如 "STRING" 或 "VARCHAR(255)"）
     * @return 计算得到的长度
     */
    uint32_t CatalogManager::calculate_varchar_length(const std::string& type_str) const {
        // 查找括号位置
        size_t paren_pos = type_str.find('(');
        if (paren_pos == std::string::npos) {
            // 没有指定长度，使用默认值
            return 255; // 默认VARCHAR长度
        }

        size_t close_paren = type_str.find(')', paren_pos);
        if (close_paren == std::string::npos) {
            return 255; // 格式错误，使用默认值
        }

        // 提取长度字符串
        std::string length_str = type_str.substr(paren_pos + 1, close_paren - paren_pos - 1);

        try {
            int length = std::stoi(length_str);
            return static_cast<uint32_t>(std::max(1, std::min(length, 65535))); // 限制在合理范围内
        } catch (...) {
            return 255; // 转换失败，使用默认值
        }
    }
} // namespace minidb