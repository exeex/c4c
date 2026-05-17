#pragma once

#include "../module.hpp"

#include <iosfwd>

namespace c4c::backend::prepare {

void append_function_summaries(std::ostringstream& out, const PreparedBirModule& module);

}  // namespace c4c::backend::prepare
