#pragma once

#include "prealloc.hpp"

namespace c4c::backend::prepare {

void run_legalize(PreparedBirModule& module, const PrepareOptions& options);

}  // namespace c4c::backend::prepare
