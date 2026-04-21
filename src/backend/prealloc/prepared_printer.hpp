#pragma once

#include "prealloc.hpp"

#include <string>

namespace c4c::backend::prepare {

// Return a human-readable dump of prepared backend state, including the
// prepared BIR plus key preparation metadata needed for route debugging.
std::string print(const PreparedBirModule& module);

}  // namespace c4c::backend::prepare
