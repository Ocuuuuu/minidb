#include "../include/engine/catalog/catalog_manager.h"
#include <algorithm> // 用于std::sort排序

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

} // namespace minidb