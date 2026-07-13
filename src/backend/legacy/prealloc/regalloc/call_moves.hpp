#pragma once

#include "../regalloc.hpp"

namespace c4c::backend::prepare {

namespace regalloc_detail {

void append_call_arg_move_resolution(const PreparedNameTables& names,
                                     const c4c::TargetProfile& target_profile,
                                     const bir::Function& function,
                                     PreparedRegallocFunction& regalloc_function);

void append_call_result_move_resolution(const PreparedNameTables& names,
                                        const c4c::TargetProfile& target_profile,
                                        const bir::Function& function,
                                        PreparedRegallocFunction& regalloc_function);

void append_return_move_resolution(const PreparedNameTables& names,
                                   const c4c::TargetProfile& target_profile,
                                   const bir::Function& function,
                                   PreparedRegallocFunction& regalloc_function);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
