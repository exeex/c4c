#pragma once

#include "codegen.hpp"

#include <string>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::string print_prepared_machine_nodes(
    const c4c::backend::prepare::PreparedBirModule& prepared);

}  // namespace c4c::backend::aarch64::codegen
