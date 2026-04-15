#pragma once

#include "prepare.hpp"

namespace c4c::backend::prepare {

void run_regalloc(PreparedBirModule& module, const PrepareOptions& options);

}  // namespace c4c::backend::prepare
