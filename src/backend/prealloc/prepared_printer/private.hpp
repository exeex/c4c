#pragma once

#include "../module.hpp"

#include <iosfwd>

namespace c4c::backend::prepare {

void append_function_summaries(std::ostringstream& out, const PreparedBirModule& module);
void append_stack_layout(std::ostringstream& out, const PreparedBirModule& module);
void append_frame_plan(std::ostringstream& out, const PreparedBirModule& module);
void append_dynamic_stack_plan(std::ostringstream& out, const PreparedBirModule& module);
void append_call_plans(std::ostringstream& out, const PreparedBirModule& module);
void append_variadic_entry_plans(std::ostringstream& out, const PreparedBirModule& module);
void append_storage_plans(std::ostringstream& out, const PreparedBirModule& module);
void append_i128_carriers(std::ostringstream& out, const PreparedBirModule& module);
void append_f128_carriers(std::ostringstream& out, const PreparedBirModule& module);
void append_atomic_operations(std::ostringstream& out, const PreparedBirModule& module);
void append_intrinsic_carriers(std::ostringstream& out, const PreparedBirModule& module);

}  // namespace c4c::backend::prepare
