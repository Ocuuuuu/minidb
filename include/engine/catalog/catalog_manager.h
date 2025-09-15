// 管理所有表的元信息

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include "table_info.h"
#include "compiler/AST.h"

namespace minidb {

    /**
     * @brief 目录管理器类
     *
     * 数据库系统的元数据管理中心，负责管理所有表的创建、删除和查询。
     * 其他模块通过此类获取表的元信息来正确操作数据。
     * 采用单例模式设计，确保元数据的一致性。
     */
    class CatalogManager {
    public:
        CatalogManager() = default;  ///< 默认构造函数
        ~CatalogManager() = default; ///< 默认析构函数

        // 表管理接口
        /**
         * @brief 创建新表
         * @param table_name 表名，必须唯一
         * @param schema 表的模式定义（列信息、约束等）
         * @return true-创建成功, false-表已存在或创建失败
         */
        bool create_table(const std::string& table_name, const Schema& schema);

        /**
         * @brief 删除表
         * @param table_name 要删除的表名
         * @return true-删除成功, false-表不存在
         */
        bool drop_table(const std::string& table_name);

        /**
         * @brief 检查表是否存在
         * @param table_name 要检查的表名
         * @return true-表存在, false-表不存在
         */
        bool table_exists(const std::string& table_name) const;

        // 表查询接口（核心接口）
        /**
         * @brief 获取表信息（非常量版本）
         * @param table_name 表名
         * @return TableInfo指针，如果表不存在则返回nullptr
         * @note 允许通过返回的指针修改表信息，使用时需谨慎
         */
        TableInfo* get_table(const std::string& table_name);

        /**
         * @brief 获取表信息（常量版本）
         * @param table_name 表名
         * @return TableInfo常量指针，如果表不存在则返回nullptr
         * @note 保证返回的表信息不会被修改，用于只读操作
         */
        const TableInfo* get_table(const std::string& table_name) const;

        // 辅助接口
        /**
         * @brief 获取所有表名的列表
         * @return 按字母顺序排序的表名向量
         * @note 主要用于显示数据库中的所有表
         */
        std::vector<std::string> get_table_names() const;

        /**
         * @brief 获取表数量
         * @return 当前管理的表总数
         */
        uint32_t get_table_count() const { return tables_.size(); }

        // ==================== 鏂板鎺ュ彛锛欰ST闆嗘垚 ====================
        /**
         * @brief 浠嶢ST鍒涘缓鏂拌〃
         * @param create_ast CREATE TABLE璇彞鐨凙ST鑺傜偣
         * @return true-鍒涘缓鎴愬姛, false-琛ㄥ凡瀛樺湪鎴栧垱寤哄け璐?
         */
        bool create_table_from_ast(const CreateTableAST& create_ast);

        /**
         * @brief 楠岃瘉INSERT璇彞鐨凙ST鏄惁鏈夋晥
         * @param insert_ast INSERT璇彞鐨凙ST鑺傜偣
         * @return true-璇彞鏈夋晥, false-琛ㄤ笉瀛樺湪鎴栧垪涓嶅尮閰?
         */
        bool validate_insert_ast(const InsertAST& insert_ast) const;

        /**
         * @brief 楠岃瘉SELECT璇彞鐨凙ST鏄惁鏈夋晥
         * @param select_ast SELECT璇彞鐨凙ST鑺傜偣
         * @return true-璇彞鏈夋晥, false-琛ㄤ笉瀛樺湪鎴栧垪涓嶅瓨鍦?
         */
        bool validate_select_ast(const SelectAST& select_ast) const;

        /**
         * @brief 鑾峰彇琛ㄧ殑Schema淇℃伅锛堢敤浜嶢ST鎵ц锛?
         * @param table_name 琛ㄥ悕
         * @return Schema鐨勫叡浜寚閽堬紝濡傛灉琛ㄤ笉瀛樺湪鍒欒繑鍥瀗ullptr
         */
        std::shared_ptr<Schema> get_table_schema(const std::string& table_name) const;

    private:
        /// 表信息存储容器：表名到TableInfo的映射
        /// 使用unique_ptr智能指针自动管理内存，避免内存泄漏
        /// unordered_map提供O(1)时间复杂度的查找操作
        std::unordered_map<std::string, std::unique_ptr<TableInfo>> tables_;

        // ==================== 鏂板绉佹湁鏂规硶 ====================
        /**
         * @brief 灏咥ST涓殑瀛楃涓茬被鍨嬭浆鎹负TypeId鏋氫妇
         * @param type_str 绫诲瀷瀛楃涓诧紙濡? "INT", "STRING", "BOOLEAN"锛?
         * @return 瀵瑰簲鐨凾ypeId鏋氫妇鍊?
         * @throw std::invalid_argument 濡傛灉绫诲瀷瀛楃涓叉棤鏁?
         */
        TypeId convert_ast_type_to_typeid(const std::string& type_str) const;

        /**
         * @brief 璁＄畻VARCHAR绫诲瀷鐨勫悎閫傞暱搴?
         * @param type_str 绫诲瀷瀛楃涓诧紙濡? "STRING" 鎴? "VARCHAR(255)"锛?
         * @return 璁＄畻寰楀埌鐨勯暱搴?
         */
        uint32_t calculate_varchar_length(const std::string& type_str) const;
    };

} // namespace minidb