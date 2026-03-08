#include "hir_to_llvm.hpp"

#include "ir_builder.hpp"

// Phase 3 bootstrap: delegate to legacy IRBuilder.
// The HIR module is passed for future incremental replacement of emission logic.
// Once all constructs are handled natively, ast_root will be removed.

namespace tinyc2ll::codegen::llvm_backend {

std::string emit_module(const Module& /*mod*/, const Node* ast_root) {
  tinyc2ll::frontend_cxx::IRBuilder builder;
  // const_cast: IRBuilder::emit_program predates const correctness; it does not
  // mutate the AST. This cast will be removed when the bridge is replaced.
  return builder.emit_program(const_cast<tinyc2ll::frontend_cxx::Node*>(ast_root));
}

}  // namespace tinyc2ll::codegen::llvm_backend
