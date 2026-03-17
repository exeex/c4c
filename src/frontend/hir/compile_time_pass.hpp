#pragma once

#include "ir.hpp"

#include <functional>
#include <string>
#include <vector>

namespace c4c::hir {

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
  size_t templates_deferred = 0;      // template instantiations created by this pass
  size_t consteval_reduced = 0;       // consteval calls successfully reduced to constants
  size_t consteval_pending = 0;       // consteval calls whose result is missing or invalid
  size_t iterations = 0;              // total fixpoint iterations performed
  bool converged = false;             // true if no new work was found
  std::vector<CompileTimeDiagnostic> diagnostics;  // details on irreducible nodes
};

/// Callback for deferred template instantiation.
///
/// Called by the compile-time reduction pass when it discovers a template
/// call whose target function has not been lowered yet.
///
/// Parameters:
///   template_name  - original template name (e.g. "add")
///   bindings       - resolved type bindings (e.g. {T: int})
///   mangled_name   - target mangled name (e.g. "add_i")
///
/// Returns true if the function was successfully lowered and added to the module.
using DeferredInstantiateFn = std::function<bool(
    const std::string& template_name,
    const TypeBindings& bindings,
    const std::string& mangled_name)>;

/// Run the HIR compile-time reduction loop.
///
/// This pass iterates:
///   1. template instantiation for newly-ready applications
///   2. consteval reduction for newly-ready immediate calls
/// until convergence or the iteration limit is reached.
///
/// When instantiate_fn is provided, the pass will invoke it to lower template
/// functions that were not instantiated during initial AST-to-HIR lowering
/// (e.g., nested template calls like add<T>() inside twice<T>()).
/// When instantiate_fn is null, the pass only verifies existing state.
CompileTimePassStats run_compile_time_reduction(
    Module& module,
    DeferredInstantiateFn instantiate_fn = nullptr);

/// Format pass statistics for debug output (used by --dump-hir).
std::string format_compile_time_stats(const CompileTimePassStats& stats);

/// Result of materialization.
struct MaterializationStats {
  size_t materialized = 0;      // functions marked for emission
  size_t non_materialized = 0;  // functions kept for compile-time only
};

/// Mark functions in the module for materialization.
///
/// Current policy: all concrete functions are materialized.  This pass
/// makes the materialization decision explicit and separable from codegen,
/// so future policies (lazy emission, JIT deferral) can override it.
MaterializationStats materialize_ready_functions(Module& module);

/// Format materialization stats for debug output.
std::string format_materialization_stats(const MaterializationStats& stats);

}  // namespace c4c::hir
