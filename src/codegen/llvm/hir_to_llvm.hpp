#pragma once

#include <string>

#include "ast.hpp"
#include "ir.hpp"

// Phase 3 of docs/sema_ir_split_plan.md:
// HIR -> LLVM backend module.
//
// emit_module() is the main entry point for the new pipeline:
//   AST -> HIR (ast_to_hir) -> LLVM IR (this module)
//
// During Phase 3 bootstrap, ast_root is forwarded to the legacy IRBuilder.
// It will be phased out as HIR->LLVM translation matures.

namespace tinyc2ll::codegen::llvm_backend {

using tinyc2ll::frontend_cxx::Node;
using Module = tinyc2ll::frontend_cxx::sema_ir::phase2::hir::Module;

std::string emit_module(const Module& mod, const Node* ast_root);

}  // namespace tinyc2ll::codegen::llvm_backend
