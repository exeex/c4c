#pragma once

#include <optional>
#include <string>

#include "../../target_profile.hpp"
#include "canonical_symbol.hpp"
#include "hir_ir.hpp"
#include "source_profile.hpp"
#include "validate.hpp"

namespace c4c::sema {

using HirModule = c4c::hir::Module;

struct AnalyzeResult {
  ValidateResult validation;
  SemaCanonicalResult canonical;
  std::optional<HirModule> hir_module;
};

AnalyzeResult analyze_program(const Node* root,
                              SemaProfile profile = SemaProfile::C,
                              const c4c::TargetProfile& target_profile = {});


}  // namespace c4c::sema
