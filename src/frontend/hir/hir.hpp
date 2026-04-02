#pragma once

// Public HIR facade.
//
// Architecture:
// - parser handles syntax plus the minimum early disambiguation needed to parse C++
// - sema handles the first semantic pass: traditional type/canonicalization checks
// - HIR handles the second semantic pass for template/dependent C++ constructs
//
// In practice, HIR is where higher-level C++ semantics are lowered toward a
// stable, more C-like semantic model that later lowering/codegen stages can
// consume. Deferred compile-time work such as template-driven retries,
// deferred consteval reduction, and deferred NTTP-related work is coordinated
// by compile_time_engine after initial HIR construction.

#include <string>

#include "ast.hpp"
#include "canonical_symbol.hpp"
#include "hir_printer.hpp"
#include "hir_ir.hpp"

namespace c4c::hir {

Module build_hir(const Node* program_root,
                 const sema::ResolvedTypeTable* resolved_types = nullptr,
                 SourceProfile source_profile = SourceProfile::C,
                 const std::string& target_triple = "");
std::string format_summary(const Module& module);

}  // namespace c4c::hir
