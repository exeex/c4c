#pragma once

#include <string>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::string apply_deferred_peephole_boundary(std::string assembly);

}  // namespace c4c::backend::aarch64::codegen
