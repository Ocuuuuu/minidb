#pragma once

#include "Value.h"
#include "Types.h"
#include <vector>
#include <memory>

namespace minidb {

    class Tuple {
    public:
        // 构造函数
        Tuple();
        explicit Tuple(std::vector<Value> values);

        // 禁止拷贝和赋值
        Tuple(const Tuple&) = default;
        Tuple& operator=(const Tuple&) = default;

        // 数据访问
        const Value& getValue(size_t index) const;
        Value& getValue(size_t index);
        const std::vector<Value>& getValues() const { return values_; }

        // 元数据信息
        size_t getColumnCount() const { return values_.size(); }
        size_t getEstimatedSize() const;

        // RID管理
        RID getRid() const { return rid_; }
        void setRid(RID rid) { rid_ = rid; }
        bool hasRid() const { return rid_.isValid(); }

        // 调试和输出
        std::string toString() const;
        friend std::ostream& operator<<(std::ostream& os, const Tuple& tuple);

        // 静态方法：创建空元组
        static Tuple createEmpty();

        // 静态方法：从值列表创建（工厂方法）
        static Tuple createFromValues(std::vector<Value> values);

    private:
        std::vector<Value> values_;
        RID rid_; // 该元组在磁盘上的位置
    };

} // namespace minidb