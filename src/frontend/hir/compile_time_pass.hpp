#pragma once

#include "ir.hpp"

#include <string>
#include <vector>

namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir {

/// A diagnostic for a single irreducible compile-time node.
struct CompileTimeDiagnostic {
  enum Kind { UnresolvedTemplate, UnreducedConsteval };
  Kind kind;
  std::string description;  // human-readable description of the irreducible node
};

/// Result of a single compile-time reduction iteration.
struct CompileTimePassStats {
  size_t templates_instantiated = 0;  // template calls with resolved target functions
  size_t templates_pending = 0;       // template calls whose target is missing
  size_t consteval_reduced = 0;       // consteval calls successfully reduced to constants
  size_t consteval_pending = 0;       // consteval calls whose result is missing or invalid
  size_t iterations = 0;              // total fixpoint iterations performed
  bool converged = false;             // true if no new work was found
  std::vector<CompileTimeDiagnostic> diagnostics;  // details on irreducible nodes
};

/// Run the HIR compile-time reduction loop.
///
/// This pass iterates:
///   1. template instantiation for newly-ready applications
///   2. consteval reduction for newly-ready immediate calls
/// until convergence or the iteration limit is reached.
///
/// Currently, all template instantiation and consteval reduction is performed
/// during AST-to-HIR lowering.  This pass validates that all template
/// applications have been resolved and reports statistics.  Future iterations
/// will move reduction work here for cases where lowering cannot resolve all
/// compile-time dependencies in a single pass.
CompileTimePassStats run_compile_time_reduction(Module& module);

/// Format pass statistics for debug output (used by --dump-hir).
std::string format_compile_time_stats(const CompileTimePassStats& stats);

}  // namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir
