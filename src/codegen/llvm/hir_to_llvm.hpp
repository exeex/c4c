#pragma once

#include <string>

#include "ir.hpp"

// HIR -> LLVM backend module.

namespace tinyc2ll::codegen::llvm_backend {

using Module = tinyc2ll::frontend_cxx::sema_ir::phase2::hir::Module;

// Emits LLVM IR directly from HIR.
std::string emit_module_native(const Module& mod);

}  // namespace tinyc2ll::codegen::llvm_backend
