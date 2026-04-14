#pragma once

#include "prepare.hpp"

namespace c4c::backend::prepare {

void run_legalize(PreparedLirModule& module, const PrepareOptions& options);
void run_legalize(PreparedBirModule& module, const PrepareOptions& options);

}  // namespace c4c::backend::prepare
