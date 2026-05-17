#pragma once

#include "codegen.hpp"

namespace c4c::backend::aarch64::codegen {

// Prepared-module compile coordinator. This is the thin top-level handoff from
// accepted AArch64 prepared facts into target MIR/module records; ABI policy,
// traversal, dispatch, and printing stay in their narrower owners.
[[nodiscard]] CompileResult build_module(
    const c4c::backend::prepare::PreparedBirModule& prepared);

}  // namespace c4c::backend::aarch64::codegen
