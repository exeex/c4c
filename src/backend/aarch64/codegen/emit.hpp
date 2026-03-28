#pragma once

#include "../../../codegen/lir/ir.hpp"

#include <string>

namespace c4c::backend::aarch64 {

std::string emit_module(const c4c::codegen::lir::LirModule& module);

}  // namespace c4c::backend::aarch64
