#include "./common/Tuple.h"
#include <sstream>

namespace minidb {

    Tuple::Tuple() : values_(), rid_(RID::invalid()) {}

    Tuple::Tuple(std::vector<Value> values) : values_(std::move(values)), rid_(RID::invalid()) {}

    const Value& Tuple::getValue(size_t index) const {
        if (index >= values_.size()) {
            throw OutOfRangeException("Tuple index " + std::to_string(index) +
                                    " out of range, size is " + std::to_string(values_.size()));
        }
        return values_[index];
    }

    Value& Tuple::getValue(size_t index) {
        if (index >= values_.size()) {
            throw OutOfRangeException("Tuple index " + std::to_string(index) +
                                    " out of range, size is " + std::to_string(values_.size()));
        }
        return values_[index];
    }

    size_t Tuple::getEstimatedSize() const {
        size_t total_size = 0;
        for (const auto& value : values_) {
            total_size += value.getSize();
        }
        return total_size;
    }

    std::string Tuple::toString() const {
        std::stringstream ss;
        ss << "(";
        for (size_t i = 0; i < values_.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << values_[i].toString();
        }
        ss << ")";
        if (hasRid()) {
            ss << " @ " << rid_.toString();
        }
        return ss.str();
    }

    std::ostream& operator<<(std::ostream& os, const Tuple& tuple) {
        os << tuple.toString();
        return os;
    }

    Tuple Tuple::createEmpty() {
        return Tuple();
    }

    Tuple Tuple::createFromValues(std::vector<Value> values) {
        return Tuple(std::move(values));
    }

} // namespace minidb