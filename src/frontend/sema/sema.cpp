#include "sema.hpp"

#include "ast_to_hir.hpp"
#include "hir_printer.hpp"

namespace tinyc2ll::frontend_cxx::sema {

AnalyzeResult analyze_program(const Node* root) {
  AnalyzeResult result{};
  result.validation = validate_program(root);
  if (!result.validation.ok) return result;
  result.hir_module = sema_ir::phase2::hir::lower_ast_to_hir(root);
  return result;
}

std::string format_hir(const HirModule& module) {
  return sema_ir::phase2::hir::format_hir(module);
}

std::string format_summary(const HirModule& module) {
  return sema_ir::phase2::hir::format_summary(module);
}

}  // namespace tinyc2ll::frontend_cxx::sema
