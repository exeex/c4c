#pragma once

#include <optional>
#include <string>

#include "ir.hpp"
#include "validate.hpp"

namespace tinyc2ll::frontend_cxx::sema {

using HirModule = tinyc2ll::frontend_cxx::sema_ir::phase2::hir::Module;

struct AnalyzeResult {
  ValidateResult validation;
  std::optional<HirModule> hir_module;
};

AnalyzeResult analyze_program(const Node* root);
std::string format_hir(const HirModule& module);
std::string format_summary(const HirModule& module);

}  // namespace tinyc2ll::frontend_cxx::sema
