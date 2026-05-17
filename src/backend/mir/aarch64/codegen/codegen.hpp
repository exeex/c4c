#pragma once

#include "../module/module.hpp"

namespace c4c::backend::aarch64::codegen {

using CompiledModule = module::Module;
using CompileResult = module::BuildResult;

[[nodiscard]] CompileResult compile_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& prepared);

}  // namespace c4c::backend::aarch64::codegen
