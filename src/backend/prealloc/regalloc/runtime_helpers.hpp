#pragma once

#include "../regalloc.hpp"
#include "../runtime_helpers.hpp"

namespace c4c::backend::prepare {

namespace regalloc_detail {

void append_i128_runtime_helper_mappings(const PreparedNameTables& names,
                                         const bir::Function& function,
                                         const PreparedRegallocFunction& regalloc_function,
                                         PreparedI128RuntimeHelpers& helper_mappings);

void append_f128_runtime_helper_mappings(const PreparedNameTables& names,
                                         const bir::Function& function,
                                         const PreparedRegallocFunction& regalloc_function,
                                         PreparedF128RuntimeHelpers& helper_mappings);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
