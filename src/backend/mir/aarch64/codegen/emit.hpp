#pragma once

#include "../module/module.hpp"

namespace c4c::backend::aarch64::codegen {

// Emit-owned prepared-module build coordinator. This is the thin top-level
// handoff from accepted AArch64 prepared facts into target MIR/module records;
// ABI policy, traversal, dispatch, and printing stay in their narrower owners.
[[nodiscard]] module::BuildResult build_module(
    const c4c::backend::prepare::PreparedBirModule& prepared);

}  // namespace c4c::backend::aarch64::codegen
