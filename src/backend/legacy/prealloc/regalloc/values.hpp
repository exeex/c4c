#pragma once

#include "../regalloc.hpp"

#include <vector>

namespace c4c::backend::prepare {

namespace regalloc_detail {

[[nodiscard]] bool is_f128_immediate_constant(const bir::Value& value);

void append_f128_constant_values_for_function(std::vector<PreparedRegallocValue>& values,
                                              PreparedNameTables& names,
                                              const bir::Function* function,
                                              FunctionNameId function_name,
                                              PreparedValueId& next_value_id);

[[nodiscard]] const PreparedRegallocValue* find_f128_constant_regalloc_value(
    const PreparedRegallocFunction& function,
    const bir::Value& value);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
