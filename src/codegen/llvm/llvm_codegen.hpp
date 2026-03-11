#pragma once

#include <string>

#include "ir.hpp"

namespace tinyc2ll::codegen::llvm_backend {

using Module = tinyc2ll::frontend_cxx::sema_ir::phase2::hir::Module;

std::string emit_module_native(const Module& mod);

}  // namespace tinyc2ll::codegen::llvm_backend
