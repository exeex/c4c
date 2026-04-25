#include "sema.hpp"

#include "hir.hpp"

#include <cstdlib>

namespace c4c::sema {

// Base C analysis: validate + lower to HIR.
static AnalyzeResult analyze_program_base(const Node* root,
                                          SourceProfile source_profile,
                                          const c4c::TargetProfile& target_profile) {
  AnalyzeResult result{};
  result.validation = validate_program(root);
  if (!result.validation.ok) return result;
  result.canonical = build_canonical_symbols(root, source_profile);
  result.hir_module = hir::build_hir(
      root, &result.canonical.resolved_types, source_profile, target_profile);
  if (result.hir_module && !result.hir_module->ct_info.diagnostics.empty()) {
    result.validation.ok = false;
    result.validation.diagnostics.clear();
    for (const auto& diag : result.hir_module->ct_info.diagnostics) {
      Diagnostic parsed{nullptr, 0, 1, diag};
      const size_t err_sep = diag.find(": error: ");
      if (err_sep != std::string::npos) {
        parsed.message = diag.substr(err_sep + 9);
        const std::string prefix = diag.substr(0, err_sep);
        const size_t last_colon = prefix.rfind(':');
        const size_t second_last_colon =
            last_colon == std::string::npos ? std::string::npos : prefix.rfind(':', last_colon - 1);
        if (last_colon != std::string::npos && second_last_colon != std::string::npos) {
          parsed.file = nullptr;
          parsed.line = std::atoi(prefix.substr(second_last_colon + 1, last_colon - second_last_colon - 1).c_str());
          parsed.column = std::atoi(prefix.substr(last_colon + 1).c_str());
          parsed.message = parsed.message;
        }
      }
      result.validation.diagnostics.push_back(std::move(parsed));
    }
  }
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

AnalyzeResult analyze_program(const Node* root, SemaProfile profile,
                              const c4c::TargetProfile& target_profile) {
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

  AnalyzeResult result = analyze_program_base(root, source_profile, target_profile);
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
