#pragma once

#include <string>

#include "ir.hpp"

namespace c4c::codegen::llvm_backend {

using Module = c4c::hir::Module;

std::string emit_module_native(const Module& mod);

}  // namespace tinyc2ll::codegen::llvm_backend
