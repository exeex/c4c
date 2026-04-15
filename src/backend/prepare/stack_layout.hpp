#pragma once

#include "prepare.hpp"

namespace c4c::backend::prepare {

void run_stack_layout(PreparedLirModule& module, const PrepareOptions& options);
void run_stack_layout(PreparedBirModule& module, const PrepareOptions& options);

}  // namespace c4c::backend::prepare
