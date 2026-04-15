#pragma once

#include "prepare.hpp"

namespace c4c::backend::prepare {

void run_liveness(PreparedBirModule& module, const PrepareOptions& options);

}  // namespace c4c::backend::prepare
