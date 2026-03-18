#include "sema.hpp"

#include "hir.hpp"
#include "hir_printer.hpp"

namespace c4c::sema {

// Base C analysis: validate + lower to HIR.
static AnalyzeResult analyze_program_base(const Node* root, SourceProfile source_profile) {
  AnalyzeResult result{};
  result.validation = validate_program(root);
  if (!result.validation.ok) return result;
  result.canonical = build_canonical_symbols(root, source_profile);
  result.hir_module = hir::build_hir(root, &result.canonical.resolved_types);
  return result;
}

// C++ subset extension layer (no-op for now).
static void apply_cpp_subset_extensions(AnalyzeResult& /*result*/) {
  // Future: cpp-subset semantic checks go here.
}

// c4 extension layer (no-op for now).
static void apply_c4_extensions(AnalyzeResult& /*result*/) {
  // Future: c4-specific semantic checks go here.
}

AnalyzeResult analyze_program(const Node* root, SemaProfile profile) {
  SourceProfile source_profile = SourceProfile::C;
  switch (profile) {
    case SemaProfile::C:
      source_profile = SourceProfile::C;
      break;
    case SemaProfile::CppSubset:
      source_profile = SourceProfile::CppSubset;
      break;
    case SemaProfile::C4:
      source_profile = SourceProfile::C4;
      break;
  }

  AnalyzeResult result = analyze_program_base(root, source_profile);
  if (!result.validation.ok) return result;

  switch (profile) {
    case SemaProfile::C:
      break;
    case SemaProfile::CppSubset:
      apply_cpp_subset_extensions(result);
      break;
    case SemaProfile::C4:
      apply_cpp_subset_extensions(result);
      apply_c4_extensions(result);
      break;
  }

  return result;
}


}  // namespace c4c::sema
