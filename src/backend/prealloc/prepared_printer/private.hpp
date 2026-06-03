#pragma once

#include "../module.hpp"

#include <iosfwd>

namespace c4c::backend::prepare {

void append_function_summaries(std::ostringstream& out, const PreparedBirModule& module);
void append_prepared_control_flow(std::ostringstream& out, const PreparedBirModule& module);
void append_value_locations(std::ostringstream& out, const PreparedBirModule& module);
void append_stack_layout(std::ostringstream& out, const PreparedBirModule& module);
void append_frame_plan(std::ostringstream& out, const PreparedBirModule& module);
void append_dynamic_stack_plan(std::ostringstream& out, const PreparedBirModule& module);
void append_call_plans(std::ostringstream& out, const PreparedBirModule& module);
void append_select_chain_materializations(std::ostringstream& out,
                                          const PreparedBirModule& module);
void append_variadic_entry_plans(std::ostringstream& out, const PreparedBirModule& module);
void append_regalloc(std::ostringstream& out, const PreparedBirModule& module);
void append_storage_plans(std::ostringstream& out, const PreparedBirModule& module);
void append_addressing(std::ostringstream& out, const PreparedBirModule& module);
void append_i128_carriers(std::ostringstream& out, const PreparedBirModule& module);
void append_f128_carriers(std::ostringstream& out, const PreparedBirModule& module);
void append_atomic_operations(std::ostringstream& out, const PreparedBirModule& module);
void append_intrinsic_carriers(std::ostringstream& out, const PreparedBirModule& module);
void append_inline_asm_carriers(std::ostringstream& out, const PreparedBirModule& module);
void append_f128_runtime_helpers(std::ostringstream& out, const PreparedBirModule& module);
void append_i128_runtime_helpers(std::ostringstream& out, const PreparedBirModule& module);

}  // namespace c4c::backend::prepare
