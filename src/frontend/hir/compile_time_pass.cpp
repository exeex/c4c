#include "compile_time_pass.hpp"

#include <sstream>
#include <string>
#include <unordered_set>
#include <variant>

namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir {

namespace {

/// Attempt one round of template instantiation resolution.
///
/// Walks all CallExpr nodes with template_info and checks whether the
/// instantiated target function exists in the module.  Returns the number
/// of resolved and pending template applications.
///
/// Currently, AST-to-HIR lowering produces all instantiations, so every
/// template call should already have a corresponding function.  This step
/// provides the verification hook and will later drive deferred instantiation.
struct TemplateInstantiationStep {
  const Module& module;
  size_t resolved = 0;
  size_t pending = 0;

  void run() {
    // Build set of known instantiated function names for fast lookup.
    std::unordered_set<std::string> instantiated_fns;
    for (const auto& fn : module.functions) {
      instantiated_fns.insert(fn.name);
    }

    // Also build a set of all known template instance mangled names from
    // template_defs metadata, so we can cross-reference.
    std::unordered_set<std::string> template_instance_names;
    for (const auto& [name, tdef] : module.template_defs) {
      for (const auto& inst : tdef.instances) {
        template_instance_names.insert(inst.mangled_name);
      }
    }

    // Walk all expressions looking for template-originated calls.
    for (const auto& expr : module.expr_pool) {
      const auto* call = std::get_if<CallExpr>(&expr.payload);
      if (!call || !call->template_info.has_value()) continue;

      const auto& tci = *call->template_info;

      // Find the callee name from the DeclRef.
      const auto* callee_expr = module.find_expr(call->callee);
      if (!callee_expr) {
        ++pending;
        continue;
      }
      const auto* decl_ref = std::get_if<DeclRef>(&callee_expr->payload);
      if (!decl_ref) {
        ++pending;
        continue;
      }

      // The callee DeclRef name is the mangled instantiation name (e.g., "add_i").
      const std::string& target_name = decl_ref->name;

      // Check if the target function exists in the module.
      if (instantiated_fns.count(target_name)) {
        ++resolved;
      } else {
        ++pending;
      }
    }
  }
};

}  // namespace

CompileTimePassStats run_compile_time_reduction(Module& module) {
  CompileTimePassStats stats{};

  static constexpr int kMaxIterations = 8;

  for (int iter = 0; iter < kMaxIterations; ++iter) {
    ++stats.iterations;

    // Step 1: template instantiation resolution.
    TemplateInstantiationStep tpl_step{module};
    tpl_step.run();

    stats.templates_instantiated = tpl_step.resolved;
    stats.templates_pending = tpl_step.pending;

    // Step 2: consteval reduction count (verification).
    // Currently all consteval calls are reduced during lowering.
    stats.consteval_reduced = module.consteval_calls.size();

    // Convergence: no pending work remains.
    if (tpl_step.pending == 0) {
      stats.converged = true;
      break;
    }

    // If there are pending items but no progress was made, we've hit an
    // irreducible set.  Future slices will add actual instantiation here;
    // for now, we report non-convergence and stop.
    stats.converged = false;
    break;
  }

  return stats;
}

std::string format_compile_time_stats(const CompileTimePassStats& stats) {
  std::ostringstream out;
  out << "compile-time reduction: "
      << stats.iterations << " iteration"
      << (stats.iterations != 1 ? "s" : "")
      << ", " << stats.templates_instantiated << " template call"
      << (stats.templates_instantiated != 1 ? "s" : "")
      << " resolved"
      << ", " << stats.consteval_reduced << " consteval reduction"
      << (stats.consteval_reduced != 1 ? "s" : "");
  if (stats.templates_pending > 0) {
    out << ", " << stats.templates_pending << " pending";
  }
  if (stats.converged) {
    out << " (converged)";
  } else {
    out << " (NOT converged)";
  }
  return out.str();
}

}  // namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir
