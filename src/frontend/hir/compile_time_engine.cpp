#include "compile_time_engine.hpp"
#include "consteval.hpp"

#include <sstream>
#include <string>
#include <unordered_set>
#include <variant>

namespace c4c::hir {

std::optional<long long> CompileTimeState::evaluate_consteval(
    const std::string& fn_name,
    const std::vector<long long>& const_args,
    const TypeBindings& bindings,
    const NttpBindings& nttp_bindings) const {
  auto ce_it = consteval_fn_defs_.find(fn_name);
  if (ce_it == consteval_fn_defs_.end()) return std::nullopt;
  std::vector<ConstValue> args;
  args.reserve(const_args.size());
  for (long long v : const_args) args.push_back(ConstValue::make_int(v));
  ConstEvalEnv env{&enum_consts_, &const_int_bindings_, nullptr};
  TypeBindings tpl_bindings = bindings;
  env.type_bindings = &tpl_bindings;
  NttpBindings nttp_copy = nttp_bindings;
  if (!nttp_copy.empty())
    env.nttp_bindings = &nttp_copy;
  auto result = evaluate_consteval_call(
      ce_it->second, args, env, consteval_fn_defs_);
  if (result.ok()) return result.as_int();
  return std::nullopt;
}

namespace {

/// Attempt one round of template instantiation resolution.
///
/// Walks all CallExpr nodes with template_info and checks whether the
/// instantiated target function exists in the module.  When a deferred
/// instantiation callback is provided, unresolved calls trigger on-demand
/// lowering of the missing template function.
struct TemplateInstantiationStep {
  Module& module;
  DeferredInstantiateFn instantiate_fn;
  CompileTimeState* ct_state = nullptr;  // engine-owned state for registry queries
  size_t resolved = 0;
  size_t pending = 0;
  size_t newly_instantiated = 0;
  size_t consteval_unlocked = 0;  // consteval reductions produced by deferred instantiation
  std::vector<CompileTimeDiagnostic> pending_diags;

  void run() {
    // Build set of known instantiated function names for fast lookup.
    std::unordered_set<std::string> instantiated_fns;
    for (const auto& fn : module.functions) {
      instantiated_fns.insert(fn.name);
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
      } else if (instantiate_fn) {
        // Pre-check: does the compile-time state know about this template?
        // This gives the engine direct visibility into available definitions
        // rather than blindly probing the callback.
        if (ct_state && !ct_state->has_template_def(tci.source_template)) {
          ++pending;
          pending_diags.push_back({CompileTimeDiagnostic::UnresolvedTemplate,
              "unresolved template call: " + tci.source_template +
              " (no definition registered in compile-time state)"});
          continue;
        }

        // Pre-check: consteval templates are handled by the consteval
        // evaluation path, not by deferred instantiation.
        if (ct_state && ct_state->is_consteval_template(tci.source_template)) {
          ++pending;
          continue;
        }

        // Attempt deferred instantiation: reconstruct TypeBindings from
        // TemplateCallInfo + HirTemplateDef parameter names.
        auto tdef_it = module.template_defs.find(tci.source_template);
        if (tdef_it != module.template_defs.end()) {
          const auto& tdef = tdef_it->second;
          TypeBindings bindings;
          size_t count = std::min(tdef.template_params.size(), tci.template_args.size());
          for (size_t i = 0; i < count; ++i) {
            // Skip NTTP params in type bindings (they use dummy TypeSpec).
            if (i < tdef.param_is_nttp.size() && tdef.param_is_nttp[i]) continue;
            bindings[tdef.template_params[i]] = tci.template_args[i];
          }
          NttpBindings nttp_bindings = tci.nttp_args;
          size_t fn_count_before = module.functions.size();
          size_t ce_before = module.consteval_calls.size();
          if (instantiate_fn(tci.source_template, bindings, nttp_bindings, target_name)) {
            instantiated_fns.insert(target_name);
            ++resolved;
            ++newly_instantiated;
            // Track consteval reductions unlocked by this deferred instantiation.
            consteval_unlocked += module.consteval_calls.size() - ce_before;
            // Set template metadata on the newly-lowered function.
            // The engine now owns this post-instantiation bookkeeping
            // rather than relying on the callback to set it.
            if (module.functions.size() > fn_count_before) {
              auto& new_fn = module.functions.back();
              new_fn.template_origin = tci.source_template;
              new_fn.spec_key = nttp_bindings.empty()
                  ? make_specialization_key(
                      tci.source_template, tdef.template_params, bindings)
                  : make_specialization_key(
                      tci.source_template, tdef.template_params, bindings,
                      nttp_bindings);
            }
            // Record in engine-owned state and update module metadata.
            if (ct_state) {
              auto hi = ct_state->record_deferred_instance(
                  tci.source_template, bindings, nttp_bindings,
                  target_name, tdef.template_params);
              tdef_it->second.instances.push_back(std::move(hi));
            } else {
              // Fallback when no engine state: build metadata directly.
              HirTemplateInstantiation hi;
              hi.mangled_name = target_name;
              hi.bindings = bindings;
              hi.nttp_bindings = nttp_bindings;
              hi.spec_key = nttp_bindings.empty()
                  ? make_specialization_key(
                      tci.source_template, tdef.template_params, bindings)
                  : make_specialization_key(
                      tci.source_template, tdef.template_params, bindings,
                      nttp_bindings);
              tdef_it->second.instances.push_back(std::move(hi));
            }
          } else {
            ++pending;
            pending_diags.push_back({CompileTimeDiagnostic::UnresolvedTemplate,
                "unresolved template call: " + tci.source_template +
                " (deferred instantiation of '" + target_name + "' failed)"});
          }
        } else {
          ++pending;
          pending_diags.push_back({CompileTimeDiagnostic::UnresolvedTemplate,
              "unresolved template call: " + tci.source_template +
              " (no template definition found)"});
        }
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

/// Attempt one round of pending consteval evaluation.
///
/// Walks the module's expr_pool looking for PendingConstevalExpr nodes.
/// For each one, invokes the evaluation callback and replaces the node
/// with an IntLiteral on success, recording a ConstevalCallInfo.
struct PendingConstevalEvalStep {
  Module& module;
  DeferredConstevalEvalFn eval_fn;
  CompileTimeState* ct_state = nullptr;
  size_t evaluated = 0;
  size_t pending = 0;
  std::vector<CompileTimeDiagnostic> pending_diags;

  void run() {
    if (!eval_fn && !ct_state) return;

    for (auto& expr : module.expr_pool) {
      auto* pce = std::get_if<PendingConstevalExpr>(&expr.payload);
      if (!pce) continue;

      // Pre-check: does the compile-time state know about this consteval fn?
      if (ct_state && !ct_state->has_consteval_def(pce->fn_name)) {
        ++pending;
        pending_diags.push_back({CompileTimeDiagnostic::UnreducedConsteval,
            "pending consteval: " + pce->fn_name +
            " (no definition registered in compile-time state)"});
        continue;
      }

      // Prefer engine-owned evaluation when ct_state is available;
      // fall back to the callback for compatibility.
      std::optional<long long> result;
      if (ct_state) {
        result = ct_state->evaluate_consteval(
            pce->fn_name, pce->const_args, pce->tpl_bindings, pce->nttp_bindings);
      } else if (eval_fn) {
        result = eval_fn(pce->fn_name, pce->const_args, pce->tpl_bindings, pce->nttp_bindings);
      }
      if (result.has_value()) {
        long long rv = *result;
        // Record consteval call metadata.
        ConstevalCallInfo info;
        info.fn_name = pce->fn_name;
        info.const_args = pce->const_args;
        info.template_bindings = pce->tpl_bindings;
        info.result_value = rv;
        info.result_expr = expr.id;
        info.span = pce->call_span;
        module.consteval_calls.push_back(std::move(info));
        // Replace the PendingConstevalExpr with an IntLiteral in-place.
        expr.payload = IntLiteral{rv, false};
        ++evaluated;
      } else {
        ++pending;
        pending_diags.push_back({CompileTimeDiagnostic::UnreducedConsteval,
            "pending consteval: " + pce->fn_name + " (evaluation failed)"});
      }
    }
  }
};

}  // namespace

CompileTimeEngineStats run_compile_time_engine(
    Module& module, std::shared_ptr<CompileTimeState> ct_state,
    DeferredInstantiateFn instantiate_fn,
    DeferredConstevalEvalFn consteval_fn) {
  CompileTimeEngineStats stats{};

  static constexpr int kMaxIterations = 8;

  size_t prev_templates_pending = SIZE_MAX;
  size_t prev_consteval_pending = SIZE_MAX;

  for (int iter = 0; iter < kMaxIterations; ++iter) {
    ++stats.iterations;

    // Step 1: template instantiation resolution (with optional deferred lowering).
    TemplateInstantiationStep tpl_step{module, instantiate_fn,
                                       ct_state ? ct_state.get() : nullptr};
    tpl_step.run();

    stats.templates_instantiated = tpl_step.resolved;
    stats.templates_pending = tpl_step.pending;
    stats.templates_deferred += tpl_step.newly_instantiated;
    stats.consteval_deferred += tpl_step.consteval_unlocked;

    // Step 2: evaluate pending consteval expressions created during deferred
    // template instantiation.  These are PendingConstevalExpr nodes that
    // were deferred instead of being evaluated eagerly during lowering.
    PendingConstevalEvalStep pce_step{module, consteval_fn,
                                     ct_state ? ct_state.get() : nullptr};
    pce_step.run();

    stats.consteval_deferred += pce_step.evaluated;

    // Step 3: consteval reduction verification.
    ConstevalReductionStep ce_step{module};
    ce_step.run();

    stats.consteval_reduced = ce_step.reduced;
    stats.consteval_pending = ce_step.pending + pce_step.pending;

    // Convergence: no pending work remains in any step.
    size_t total_pending = tpl_step.pending + pce_step.pending + ce_step.pending;
    if (total_pending == 0) {
      stats.converged = true;
      break;
    }

    // Check whether this iteration made progress compared to the previous one.
    // Progress means at least one pending count decreased.
    size_t cur_consteval_pending = ce_step.pending + pce_step.pending;
    bool made_progress =
        tpl_step.pending < prev_templates_pending ||
        cur_consteval_pending < prev_consteval_pending;

    prev_templates_pending = tpl_step.pending;
    prev_consteval_pending = cur_consteval_pending;

    if (!made_progress) {
      // No progress — we've hit an irreducible set.  Collect diagnostics
      // for all pending items so the caller can report them.
      stats.converged = false;
      stats.diagnostics.insert(stats.diagnostics.end(),
          tpl_step.pending_diags.begin(), tpl_step.pending_diags.end());
      stats.diagnostics.insert(stats.diagnostics.end(),
          pce_step.pending_diags.begin(), pce_step.pending_diags.end());
      stats.diagnostics.insert(stats.diagnostics.end(),
          ce_step.pending_diags.begin(), ce_step.pending_diags.end());
      break;
    }

    // Progress was made but pending items remain — continue iterating.
  }

  // Populate definition and registry parity stats from engine-owned state.
  if (ct_state) {
    stats.template_defs_known = ct_state->template_def_count();
    stats.consteval_defs_known = ct_state->consteval_def_count();
    stats.registry_seeds = ct_state->registry.total_seed_count();
    stats.registry_instances = ct_state->registry.total_instance_count();
    stats.registry_parity = ct_state->registry.verify_parity();
  }

  return stats;
}

std::string format_compile_time_stats(const CompileTimeEngineStats& stats) {
  std::ostringstream out;
  out << "compile-time reduction: "
      << stats.iterations << " iteration"
      << (stats.iterations != 1 ? "s" : "")
      << ", " << stats.templates_instantiated << " template call"
      << (stats.templates_instantiated != 1 ? "s" : "")
      << " resolved"
      << ", " << stats.consteval_reduced << " consteval reduction"
      << (stats.consteval_reduced != 1 ? "s" : "");
  if (stats.templates_deferred > 0 || stats.consteval_deferred > 0) {
    out << " (";
    bool need_comma = false;
    if (stats.templates_deferred > 0) {
      out << stats.templates_deferred << " deferred instantiation"
          << (stats.templates_deferred != 1 ? "s" : "");
      need_comma = true;
    }
    if (stats.consteval_deferred > 0) {
      if (need_comma) out << ", ";
      out << stats.consteval_deferred << " deferred consteval reduction"
          << (stats.consteval_deferred != 1 ? "s" : "");
    }
    out << ")";
  }
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
  // Append definition registry info when available.
  if (stats.template_defs_known > 0 || stats.consteval_defs_known > 0) {
    out << "\n  definitions: template_defs=" << stats.template_defs_known
        << " consteval_defs=" << stats.consteval_defs_known;
  }
  // Append registry parity info when available.
  if (stats.registry_seeds > 0 || stats.registry_instances > 0) {
    out << "\n  registry: seeds=" << stats.registry_seeds
        << " instances=" << stats.registry_instances
        << (stats.registry_parity ? " (parity OK)" : " (MISMATCH)");
  }
  // Append structured diagnostics for irreducible nodes.
  for (const auto& diag : stats.diagnostics) {
    out << "\n  - " << diag.description;
  }
  return out.str();
}

MaterializationStats materialize_ready_functions(Module& module) {
  MaterializationStats stats{};

  for (auto& fn : module.functions) {
    if (fn.consteval_only) {
      // Consteval-only functions are preserved in HIR for analysis but
      // are not emitted as LLVM code.
      fn.materialized = false;
      ++stats.non_materialized;
    } else {
      fn.materialized = true;
      ++stats.materialized;
    }
  }

  return stats;
}

std::string format_materialization_stats(const MaterializationStats& stats) {
  std::ostringstream out;
  out << "materialization: "
      << stats.materialized << " function"
      << (stats.materialized != 1 ? "s" : "")
      << " materialized";
  if (stats.non_materialized > 0) {
    out << ", " << stats.non_materialized << " compile-time only";
  }
  return out.str();
}

}  // namespace c4c::hir
