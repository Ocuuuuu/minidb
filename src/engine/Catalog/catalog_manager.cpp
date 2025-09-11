//
// Created by 罗飞燕 on 2025/9/10.
//
#include "../include/engine/catalog/catalog_manager.h"
#include <algorithm> // 用于std::sort

namespace minidb {

/**
 * @brief 创建新表
 * @param table_name 要创建的表名
 * @param schema 表的结构定义
 * @return true-创建成功, false-表已存在
 */
    //细看重新确定
bool CatalogManager::create_table(const std::string& table_name, const Schema& schema) {
    if (table_exists(table_name)) {
        return false; // 表名已存在，创建失败
    }

    // 创建新的TableInfo对象并使用智能指针管理
    auto table_info = std::make_unique<TableInfo>(table_name, schema);
    // 将表信息添加到目录中
    tables_[table_name] = std::move(table_info);
    return true;
}

/**
 * @brief 删除表
 * @param table_name 要删除的表名
 * @return true-删除成功, false-表不存在
 */
    //细看重新确定
bool CatalogManager::drop_table(const std::string& table_name) {
    auto it = tables_.find(table_name);
    if (it == tables_.end()) {
        return false; // 表不存在，删除失败
    }

    // 从映射中移除表（智能指针会自动释放内存）
    tables_.erase(it);
    return true;
}

/**
 * @brief 检查表是否存在
 * @param table_name 要检查的表名
 * @return true-表存在, false-表不存在
 */
    //细看重新确定
bool CatalogManager::table_exists(const std::string& table_name) const {
    return tables_.find(table_name) != tables_.end();
}

/**
 * @brief 获取表信息（非常量版本）
 * @param table_name 表名
 * @return TableInfo指针，如果表不存在则返回nullptr
 * @note 非常量版本允许修改返回的TableInfo（如果需要）
 */
    //细看重新确定
TableInfo* CatalogManager::get_table(const std::string& table_name) {
    auto it = tables_.find(table_name);
    return it != tables_.end() ? it->second.get() : nullptr;
}

/**
 * @brief 获取表信息（常量版本）
 * @param table_name 表名
 * @return TableInfo常量指针，如果表不存在则返回nullptr
 * @note 常量版本保证返回的TableInfo不会被修改
 */
    //细看重新确定
const TableInfo* CatalogManager::get_table(const std::string& table_name) const {
    auto it = tables_.find(table_name);
    return it != tables_.end() ? it->second.get() : nullptr;
}

/**
 * @brief 获取所有表名的列表
 * @return 按字母顺序排序的表名向量
 */
    //细看重新确定
std::vector<std::string> CatalogManager::get_table_names() const {
    std::vector<std::string> names;
    names.reserve(tables_.size()); // 预分配空间以提高效率

    // 收集所有表名
    for (const auto& pair : tables_) {
        names.push_back(pair.first);
    }

    // 按字母顺序排序以便于显示和处理
    std::sort(names.begin(), names.end());
    return names;
}

} // namespace minidb