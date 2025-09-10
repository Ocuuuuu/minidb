#include <iostream>
#include "../include/compiler/Lexer.h"

#include "./common/Value.h"
#include "./common/Tuple.h"

int main() {
    // ≤‚ ‘Value¿‡
    minidb::Value int_val(42);
    minidb::Value str_val("Hello");
    minidb::Value bool_val(true);

    std::cout << "Integer: " << int_val << std::endl;
    std::cout << "String: " << str_val << std::endl;
    std::cout << "Boolean: " << bool_val << std::endl;

    // ≤‚ ‘Tuple¿‡
    std::vector<minidb::Value> values = {int_val, str_val, bool_val};
    minidb::Tuple tuple(values);

    std::cout << "Tuple: " << tuple << std::endl;
    std::cout << "First value: " << tuple.getValue(0) << std::endl;

    return 0;
}