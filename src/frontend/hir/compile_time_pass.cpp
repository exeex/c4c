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
  std::vector<CompileTimeDiagnostic> pending_diags;

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
        pending_diags.push_back({CompileTimeDiagnostic::UnresolvedTemplate,
            "unresolved template call: " + tci.source_template + " (callee expr missing)"});
        continue;
      }
      const auto* decl_ref = std::get_if<DeclRef>(&callee_expr->payload);
      if (!decl_ref) {
        ++pending;
        pending_diags.push_back({CompileTimeDiagnostic::UnresolvedTemplate,
            "unresolved template call: " + tci.source_template + " (callee is not a DeclRef)"});
        continue;
      }

      // The callee DeclRef name is the mangled instantiation name (e.g., "add_i").
      const std::string& target_name = decl_ref->name;

      // Check if the target function exists in the module.
      if (instantiated_fns.count(target_name)) {
        ++resolved;
      } else {
        ++pending;
        pending_diags.push_back({CompileTimeDiagnostic::UnresolvedTemplate,
            "unresolved template call: " + tci.source_template +
            " (instantiation '" + target_name + "' not found)"});
      }
    }
  }
};

/// Attempt one round of consteval reduction verification.
///
/// Walks all ConstevalCallInfo records and checks whether the result_expr
/// points to a valid IntLiteral in the module's expr_pool with a matching
/// value.  Returns the number of verified (reduced) and pending records.
///
/// Currently, AST-to-HIR lowering reduces all consteval calls eagerly, so
/// every record should already have a valid result.  This step provides the
/// verification hook and will later drive deferred consteval reduction when
/// template instantiation produces new consteval call sites.
struct ConstevalReductionStep {
  const Module& module;
  size_t reduced = 0;
  size_t pending = 0;
  std::vector<CompileTimeDiagnostic> pending_diags;

  void run() {
    for (const auto& cci : module.consteval_calls) {
      // Check that result_expr points to a valid IntLiteral with the expected value.
      const auto* expr = module.find_expr(cci.result_expr);
      if (!expr) {
        ++pending;
        pending_diags.push_back({CompileTimeDiagnostic::UnreducedConsteval,
            "unreduced consteval: " + cci.fn_name + " (result expr missing)"});
        continue;
      }
      const auto* lit = std::get_if<IntLiteral>(&expr->payload);
      if (!lit) {
        ++pending;
        pending_diags.push_back({CompileTimeDiagnostic::UnreducedConsteval,
            "unreduced consteval: " + cci.fn_name + " (result is not an IntLiteral)"});
        continue;
      }
      if (lit->value != cci.result_value) {
        ++pending;
        pending_diags.push_back({CompileTimeDiagnostic::UnreducedConsteval,
            "unreduced consteval: " + cci.fn_name + " (result value mismatch: expected " +
            std::to_string(cci.result_value) + ", got " + std::to_string(lit->value) + ")"});
        continue;
      }
      ++reduced;
    }
  }
};

}  // namespace

CompileTimePassStats run_compile_time_reduction(Module& module) {
  CompileTimePassStats stats{};

  static constexpr int kMaxIterations = 8;

  size_t prev_templates_pending = SIZE_MAX;
  size_t prev_consteval_pending = SIZE_MAX;

  for (int iter = 0; iter < kMaxIterations; ++iter) {
    ++stats.iterations;

    // Step 1: template instantiation resolution.
    TemplateInstantiationStep tpl_step{module};
    tpl_step.run();

    stats.templates_instantiated = tpl_step.resolved;
    stats.templates_pending = tpl_step.pending;

    // Step 2: consteval reduction verification.
    ConstevalReductionStep ce_step{module};
    ce_step.run();

    stats.consteval_reduced = ce_step.reduced;
    stats.consteval_pending = ce_step.pending;

    // Convergence: no pending work remains in either step.
    if (tpl_step.pending == 0 && ce_step.pending == 0) {
      stats.converged = true;
      break;
    }

    // Check whether this iteration made progress compared to the previous one.
    // Progress means at least one pending count decreased.
    bool made_progress =
        tpl_step.pending < prev_templates_pending ||
        ce_step.pending < prev_consteval_pending;

    prev_templates_pending = tpl_step.pending;
    prev_consteval_pending = ce_step.pending;

    if (!made_progress) {
      // No progress — we've hit an irreducible set.  Collect diagnostics
      // for all pending items so the caller can report them.
      stats.converged = false;
      stats.diagnostics.insert(stats.diagnostics.end(),
          tpl_step.pending_diags.begin(), tpl_step.pending_diags.end());
      stats.diagnostics.insert(stats.diagnostics.end(),
          ce_step.pending_diags.begin(), ce_step.pending_diags.end());
      break;
    }

    // Progress was made but pending items remain — continue iterating.
    // Future work: trigger deferred template instantiation or consteval
    // reduction here to resolve newly-ready items.
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
  if (stats.templates_pending > 0 || stats.consteval_pending > 0) {
    out << " (";
    bool first = true;
    if (stats.templates_pending > 0) {
      out << stats.templates_pending << " template" << (stats.templates_pending != 1 ? "s" : "") << " pending";
      first = false;
    }
    if (stats.consteval_pending > 0) {
      if (!first) out << ", ";
      out << stats.consteval_pending << " consteval" << (stats.consteval_pending != 1 ? "s" : "") << " pending";
    }
    out << ")";
  }
  if (stats.converged) {
    out << " (converged)";
  } else {
    out << " (NOT converged)";
  }
  // Append structured diagnostics for irreducible nodes.
  for (const auto& diag : stats.diagnostics) {
    out << "\n  - " << diag.description;
  }
  return out.str();
}

}  // namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir
