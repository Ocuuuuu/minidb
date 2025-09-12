// ¹ÜÀíËùÓĞ±íµÄÔªĞÅÏ¢

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include "table_info.h"
#include "compiler/AST.h"

namespace minidb {

    /**
     * @brief Ä¿Â¼¹ÜÀíÆ÷Àà
     *
     * Êı¾İ¿âÏµÍ³µÄÔªÊı¾İ¹ÜÀíÖĞĞÄ£¬¸ºÔğ¹ÜÀíËùÓĞ±íµÄ´´½¨¡¢É¾³ıºÍ²éÑ¯¡£
     * ÆäËûÄ£¿éÍ¨¹ı´ËÀà»ñÈ¡±íµÄÔªĞÅÏ¢À´ÕıÈ·²Ù×÷Êı¾İ¡£
     * ²ÉÓÃµ¥ÀıÄ£Ê½Éè¼Æ£¬È·±£ÔªÊı¾İµÄÒ»ÖÂĞÔ¡£
     */
    class CatalogManager {
    public:
        CatalogManager() = default;  ///< Ä¬ÈÏ¹¹Ôìº¯Êı
        ~CatalogManager() = default; ///< Ä¬ÈÏÎö¹¹º¯Êı

        // ±í¹ÜÀí½Ó¿Ú
        /**
         * @brief ´´½¨ĞÂ±í
         * @param table_name ±íÃû£¬±ØĞëÎ¨Ò»
         * @param schema ±íµÄÄ£Ê½¶¨Òå£¨ÁĞĞÅÏ¢¡¢Ô¼ÊøµÈ£©
         * @return true-´´½¨³É¹¦, false-±íÒÑ´æÔÚ»ò´´½¨Ê§°Ü
         */
        bool create_table(const std::string& table_name, const Schema& schema);

        /**
         * @brief É¾³ı±í
         * @param table_name ÒªÉ¾³ıµÄ±íÃû
         * @return true-É¾³ı³É¹¦, false-±í²»´æÔÚ
         */
        bool drop_table(const std::string& table_name);

        /**
         * @brief ¼ì²é±íÊÇ·ñ´æÔÚ
         * @param table_name Òª¼ì²éµÄ±íÃû
         * @return true-±í´æÔÚ, false-±í²»´æÔÚ
         */
        bool table_exists(const std::string& table_name) const;

        // ±í²éÑ¯½Ó¿Ú£¨ºËĞÄ½Ó¿Ú£©
        /**
         * @brief »ñÈ¡±íĞÅÏ¢£¨·Ç³£Á¿°æ±¾£©
         * @param table_name ±íÃû
         * @return TableInfoÖ¸Õë£¬Èç¹û±í²»´æÔÚÔò·µ»Ønullptr
         * @note ÔÊĞíÍ¨¹ı·µ»ØµÄÖ¸ÕëĞŞ¸Ä±íĞÅÏ¢£¬Ê¹ÓÃÊ±Ğè½÷É÷
         */
        TableInfo* get_table(const std::string& table_name);

        /**
         * @brief »ñÈ¡±íĞÅÏ¢£¨³£Á¿°æ±¾£©
         * @param table_name ±íÃû
         * @return TableInfo³£Á¿Ö¸Õë£¬Èç¹û±í²»´æÔÚÔò·µ»Ønullptr
         * @note ±£Ö¤·µ»ØµÄ±íĞÅÏ¢²»»á±»ĞŞ¸Ä£¬ÓÃÓÚÖ»¶Á²Ù×÷
         */
        const TableInfo* get_table(const std::string& table_name) const;

        // ¸¨Öú½Ó¿Ú
        /**
         * @brief »ñÈ¡ËùÓĞ±íÃûµÄÁĞ±í
         * @return °´×ÖÄ¸Ë³ĞòÅÅĞòµÄ±íÃûÏòÁ¿
         * @note Ö÷ÒªÓÃÓÚÏÔÊ¾Êı¾İ¿âÖĞµÄËùÓĞ±í
         */
        std::vector<std::string> get_table_names() const;

        /**
         * @brief »ñÈ¡±íÊıÁ¿
         * @return µ±Ç°¹ÜÀíµÄ±í×ÜÊı
         */
        uint32_t get_table_count() const { return tables_.size(); }

        // ==================== æ–°å¢æ¥å£ï¼šASTé›†æˆ ====================
        /**
         * @brief ä»ASTåˆ›å»ºæ–°è¡¨
         * @param create_ast CREATE TABLEè¯­å¥çš„ASTèŠ‚ç‚¹
         * @return true-åˆ›å»ºæˆåŠŸ, false-è¡¨å·²å­˜åœ¨æˆ–åˆ›å»ºå¤±è´¥
         */
        bool create_table_from_ast(const CreateTableAST& create_ast);

        /**
         * @brief éªŒè¯INSERTè¯­å¥çš„ASTæ˜¯å¦æœ‰æ•ˆ
         * @param insert_ast INSERTè¯­å¥çš„ASTèŠ‚ç‚¹
         * @return true-è¯­å¥æœ‰æ•ˆ, false-è¡¨ä¸å­˜åœ¨æˆ–åˆ—ä¸åŒ¹é…
         */
        bool validate_insert_ast(const InsertAST& insert_ast) const;

        /**
         * @brief éªŒè¯SELECTè¯­å¥çš„ASTæ˜¯å¦æœ‰æ•ˆ
         * @param select_ast SELECTè¯­å¥çš„ASTèŠ‚ç‚¹
         * @return true-è¯­å¥æœ‰æ•ˆ, false-è¡¨ä¸å­˜åœ¨æˆ–åˆ—ä¸å­˜åœ¨
         */
        bool validate_select_ast(const SelectAST& select_ast) const;

        /**
         * @brief è·å–è¡¨çš„Schemaä¿¡æ¯ï¼ˆç”¨äºASTæ‰§è¡Œï¼‰
         * @param table_name è¡¨å
         * @return Schemaçš„å…±äº«æŒ‡é’ˆï¼Œå¦‚æœè¡¨ä¸å­˜åœ¨åˆ™è¿”å›nullptr
         */
        std::shared_ptr<Schema> get_table_schema(const std::string& table_name) const;

    private:
        /// ±íĞÅÏ¢´æ´¢ÈİÆ÷£º±íÃûµ½TableInfoµÄÓ³Éä
        /// Ê¹ÓÃunique_ptrÖÇÄÜÖ¸Õë×Ô¶¯¹ÜÀíÄÚ´æ£¬±ÜÃâÄÚ´æĞ¹Â©
        /// unordered_mapÌá¹©O(1)Ê±¼ä¸´ÔÓ¶ÈµÄ²éÕÒ²Ù×÷
        std::unordered_map<std::string, std::unique_ptr<TableInfo>> tables_;

        // ==================== æ–°å¢ç§æœ‰æ–¹æ³• ====================
        /**
         * @brief å°†ASTä¸­çš„å­—ç¬¦ä¸²ç±»å‹è½¬æ¢ä¸ºTypeIdæšä¸¾
         * @param type_str ç±»å‹å­—ç¬¦ä¸²ï¼ˆå¦‚ "INT", "STRING", "BOOLEAN"ï¼‰
         * @return å¯¹åº”çš„TypeIdæšä¸¾å€¼
         * @throw std::invalid_argument å¦‚æœç±»å‹å­—ç¬¦ä¸²æ— æ•ˆ
         */
        TypeId convert_ast_type_to_typeid(const std::string& type_str) const;

        /**
         * @brief è®¡ç®—VARCHARç±»å‹çš„åˆé€‚é•¿åº¦
         * @param type_str ç±»å‹å­—ç¬¦ä¸²ï¼ˆå¦‚ "STRING" æˆ– "VARCHAR(255)"ï¼‰
         * @return è®¡ç®—å¾—åˆ°çš„é•¿åº¦
         */
        uint32_t calculate_varchar_length(const std::string& type_str) const;
    };

} // namespace minidb