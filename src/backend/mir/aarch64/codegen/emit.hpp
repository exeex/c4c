#pragma once

#include "../module/module.hpp"

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] module::BuildResult build_module(
    const c4c::backend::prepare::PreparedBirModule& prepared);

}  // namespace c4c::backend::aarch64::codegen
