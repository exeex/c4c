#pragma once

#include "../prealloc.hpp"
#include "../regalloc.hpp"

namespace c4c::backend::prepare {

namespace regalloc_detail {

void append_phi_move_resolution(const PreparedNameTables& names,
                                const bir::Function& function,
                                const PreparedControlFlowFunction& function_cf,
                                PreparedRegallocFunction& regalloc_function);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
