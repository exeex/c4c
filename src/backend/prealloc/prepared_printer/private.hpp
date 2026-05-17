#pragma once

#include "../module.hpp"

#include <iosfwd>

namespace c4c::backend::prepare {

void append_function_summaries(std::ostringstream& out, const PreparedBirModule& module);
void append_stack_layout(std::ostringstream& out, const PreparedBirModule& module);
void append_frame_plan(std::ostringstream& out, const PreparedBirModule& module);
void append_dynamic_stack_plan(std::ostringstream& out, const PreparedBirModule& module);

}  // namespace c4c::backend::prepare
