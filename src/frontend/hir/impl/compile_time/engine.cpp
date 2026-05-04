#include "compile_time.hpp"
#include "consteval.hpp"

#include <sstream>
#include <string>
#include <unordered_set>
#include <variant>

namespace c4c::hir {

namespace {

CompileTimeDiagnostic make_diag(CompileTimeDiagnostic::Kind kind,
                                std::string description,
                                const Node* node = nullptr) {
  CompileTimeDiagnostic diag;
  diag.kind = kind;
  if (node) {
    diag.file = node->file;
    diag.line = node->line;
    diag.column = node->column;
  }
  diag.description = std::move(description);
  return diag;
}

void sync_global_const_bindings(Module& module, CompileTimeState* ct_state) {
  if (!ct_state) return;
  for (const auto& global : module.globals) {
    if (!global.is_const) continue;
    const auto* scalar = std::get_if<InitScalar>(&global.init);
    if (!scalar) continue;
    const Expr* expr = module.find_expr(scalar->expr);
    if (!expr) continue;
    if (const auto* lit = std::get_if<IntLiteral>(&expr->payload)) {
      ct_state->register_const_int_binding(global.name, lit->value);
    } else if (const auto* ch = std::get_if<CharLiteral>(&expr->payload)) {
      ct_state->register_const_int_binding(global.name, ch->value);
    }
  }
}

struct EngineConstEvalStructuredMaps {
  ConstStructuredMap enum_consts_by_key;
  ConstStructuredMap named_consts_by_key;
};

std::optional<ConstEvalStructuredNameKey> to_consteval_name_key(
    const CompileTimeValueBindingKey& key) {
  if (!key.valid()) return std::nullopt;
  ConstEvalStructuredNameKey out;
  out.namespace_context_id = key.namespace_context_id;
  out.is_global_qualified = key.is_global_qualified;
  out.qualifier_text_ids = key.qualifier_segment_text_ids;
  out.base_text_id = key.unqualified_text_id;
  return out;
}

template <typename SourceMap>
void copy_consteval_structured_bindings(const SourceMap& source,
                                        ConstStructuredMap& out) {
  out.clear();
  out.reserve(source.size());
  for (const auto& [key, value] : source) {
    auto consteval_key = to_consteval_name_key(key);
    if (!consteval_key.has_value() || !consteval_key->valid()) continue;
    out[*consteval_key] = value;
  }
}

ConstEvalEnv make_engine_consteval_env(
    const CompileTimeState& ct_state,
    EngineConstEvalStructuredMaps& structured_maps) {
  copy_consteval_structured_bindings(ct_state.enum_consts_by_key(),
                                     structured_maps.enum_consts_by_key);
  copy_consteval_structured_bindings(ct_state.const_int_bindings_by_key(),
                                     structured_maps.named_consts_by_key);

  ConstEvalEnv env{&ct_state.enum_consts(), &ct_state.const_int_bindings(),
                   nullptr};
  env.enum_consts_by_key = &structured_maps.enum_consts_by_key;
  env.named_consts_by_key = &structured_maps.named_consts_by_key;
  return env;
}

struct PendingTemplateTypeStep {
  CompileTimeState* ct_state = nullptr;
  DeferredInstantiateTypeFn instantiate_type_fn;
  size_t resolved = 0;
  size_t terminal = 0;
  size_t pending = 0;
  size_t blocked = 0;
  size_t spawned_new_work = 0;
  std::vector<CompileTimeDiagnostic> pending_diags;

  // Returns resolved + terminal (items no longer pending).
  size_t completed() const { return resolved + terminal; }

  void run() {
    if (!ct_state) return;
    // Use index-based iteration: the callback may append new work items to the
    // pending list via record_pending_template_type(), which would invalidate
    // range-for iterators if the vector reallocates.  Newly appended items are
    // intentionally skipped this iteration — they will be processed in the
    // next engine pass.
    const size_t n = ct_state->pending_template_types().size();
    for (size_t i = 0; i < n; ++i) {
      PendingTemplateTypeWorkItem work_item =
          ct_state->pending_template_types()[i];
      if (ct_state->is_pending_template_type_resolved(work_item.dedup_key))
        continue;
      DeferredTemplateTypeResult result =
          instantiate_type_fn
              ? instantiate_type_fn(work_item)
              : DeferredTemplateTypeResult::blocked();
      if (result.kind == DeferredTemplateTypeResultKind::Resolved) {
        ct_state->mark_pending_template_type_resolved(work_item.dedup_key);
        ++resolved;
      } else if (result.kind == DeferredTemplateTypeResultKind::Terminal) {
        // Terminal items are done — mark resolved so they won't be retried,
        // but record the diagnostic.
        ct_state->mark_pending_template_type_resolved(work_item.dedup_key);
        ++terminal;
        std::string label = result.diagnostic;
        if (label.empty()) {
          label = work_item.context_name.empty()
              ? pending_template_type_kind_name(work_item.kind)
              : work_item.context_name;
          label = "unresolved template type: " + label;
        }
        pending_diags.push_back(
            make_diag(CompileTimeDiagnostic::UnresolvedTemplate, std::move(label)));
      } else {
        ++pending;
        ++blocked;
        if (result.spawned_new_work) ++spawned_new_work;
        if (!result.diagnostic.empty()) {
          pending_diags.push_back(
              make_diag(CompileTimeDiagnostic::UnresolvedTemplate, result.diagnostic));
        }
      }
    }
  }
};

ConstEvalResult evaluate_pending_consteval(
    const Module& module,
    const CompileTimeState& ct_state,
    const PendingConstevalExpr& pce) {
  const Node* ce_fn_def = ct_state.find_consteval_def(pce.fn_name);
  if (!ce_fn_def) {
    return ConstEvalResult::failure(
        "no consteval definition registered for '" + pce.fn_name + "'");
  }
  std::vector<ConstValue> args;
  args.reserve(pce.const_args.size());
  for (long long v : pce.const_args) args.push_back(ConstValue::make_int(v));
  EngineConstEvalStructuredMaps structured_maps;
  ConstEvalEnv env = make_engine_consteval_env(ct_state, structured_maps);
  TypeBindings tpl_bindings = pce.tpl_bindings;
  env.type_bindings = &tpl_bindings;
  NttpBindings nttp_copy = pce.nttp_bindings;
  if (!nttp_copy.empty()) env.nttp_bindings = &nttp_copy;
  NttpTextBindings nttp_text_copy = pce.nttp_bindings_by_text;
  if (!nttp_text_copy.empty()) env.nttp_bindings_by_text = &nttp_text_copy;
  env.struct_defs = &module.struct_defs;
  env.struct_def_owner_index = &module.struct_def_owner_index;
  env.link_name_texts = module.link_name_texts.get();
  return evaluate_consteval_call(
      ce_fn_def, args, env, ct_state.consteval_fn_defs());
}

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
        pending_diags.push_back(make_diag(CompileTimeDiagnostic::UnresolvedTemplate,
            "unresolved template call: " + tci.source_template + " (callee expr missing)"));
        continue;
      }
      const auto* decl_ref = std::get_if<DeclRef>(&callee_expr->payload);
      if (!decl_ref) {
        ++pending;
        pending_diags.push_back(make_diag(CompileTimeDiagnostic::UnresolvedTemplate,
            "unresolved template call: " + tci.source_template + " (callee is not a DeclRef)"));
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
          pending_diags.push_back(make_diag(CompileTimeDiagnostic::UnresolvedTemplate,
              "unresolved template call: " + tci.source_template +
              " (no definition registered in compile-time state)"));
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
          NttpTextBindings nttp_bindings_by_text = tci.nttp_args_by_text;
          size_t fn_count_before = module.functions.size();
          size_t ce_before = module.consteval_calls.size();
          if (instantiate_fn(tci.source_template, bindings, nttp_bindings,
                             nttp_bindings_by_text, target_name)) {
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
                  nttp_bindings_by_text,
                  target_name, tdef.template_params);
              tdef_it->second.instances.push_back(std::move(hi));
            }
          } else {
            ++pending;
            pending_diags.push_back(make_diag(CompileTimeDiagnostic::UnresolvedTemplate,
                "unresolved template call: " + tci.source_template +
                " (deferred instantiation of '" + target_name + "' failed)"));
          }
        } else {
          ++pending;
          pending_diags.push_back(make_diag(CompileTimeDiagnostic::UnresolvedTemplate,
              "unresolved template call: " + tci.source_template +
              " (no template definition found)"));
        }
      } else {
        ++pending;
        pending_diags.push_back(make_diag(CompileTimeDiagnostic::UnresolvedTemplate,
            "unresolved template call: " + tci.source_template +
            " (instantiation '" + target_name + "' not found)"));
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
        pending_diags.push_back(make_diag(CompileTimeDiagnostic::UnreducedConsteval,
            "unreduced consteval: " + cci.fn_name + " (result expr missing)"));
        continue;
      }
      const auto* lit = std::get_if<IntLiteral>(&expr->payload);
      if (!lit) {
        ++pending;
        pending_diags.push_back(make_diag(CompileTimeDiagnostic::UnreducedConsteval,
            "unreduced consteval: " + cci.fn_name + " (result is not an IntLiteral)"));
        continue;
      }
      if (lit->value != cci.result_value) {
        ++pending;
        pending_diags.push_back(make_diag(CompileTimeDiagnostic::UnreducedConsteval,
            "unreduced consteval: " + cci.fn_name + " (result value mismatch: expected " +
            std::to_string(cci.result_value) + ", got " + std::to_string(lit->value) + ")"));
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
  CompileTimeState* ct_state = nullptr;
  size_t evaluated = 0;
  size_t deferred_evaluated = 0;
  size_t pending = 0;
  std::vector<CompileTimeDiagnostic> pending_diags;

  void run() {
    if (!ct_state) return;

    for (auto& expr : module.expr_pool) {
      auto* pce = std::get_if<PendingConstevalExpr>(&expr.payload);
      if (!pce) continue;

      // Pre-check: does the compile-time state know about this consteval fn?
      if (!ct_state->has_consteval_def(pce->fn_name)) {
        ++pending;
        pending_diags.push_back(make_diag(CompileTimeDiagnostic::UnreducedConsteval,
            "pending consteval: " + pce->fn_name +
            " (no definition registered in compile-time state)"));
        continue;
      }

      auto result = evaluate_pending_consteval(module, *ct_state, *pce);
      if (result.ok()) {
        long long rv = result.as_int();
        // Record consteval call metadata.
        ConstevalCallInfo info;
        info.fn_name = pce->fn_name;
        info.fn_name_text_id =
            module.link_name_texts ? module.link_name_texts->intern(info.fn_name) : kInvalidText;
        info.const_args = pce->const_args;
        info.template_bindings = pce->tpl_bindings;
        info.result_value = rv;
        info.result_expr = expr.id;
        info.span = pce->call_span;
        module.consteval_calls.push_back(std::move(info));
        // Replace the PendingConstevalExpr with an IntLiteral in-place.
        if (pce->unlocked_by_deferred_instantiation) ++deferred_evaluated;
        expr.payload = IntLiteral{rv, false};
        ++evaluated;
      } else {
        ++pending;
        pending_diags.push_back(make_diag(CompileTimeDiagnostic::UnreducedConsteval,
            "pending consteval: " + pce->fn_name +
            (result.error.empty()
                ? " (evaluation failed)"
                : " (" + result.error + ")")));
      }
    }
  }
};

bool try_evaluate_consteval_call_expr(
    const Node* expr,
    const Module& module,
    const CompileTimeState& ct_state,
    long long* out_value) {
  if (!expr || expr->kind != NK_CALL || !expr->left ||
      expr->left->kind != NK_VAR || !expr->left->name) {
    return false;
  }

  const Node* ce_fn_def = ct_state.find_consteval_def(expr->left->name);
  if (!ce_fn_def) return false;

  EngineConstEvalStructuredMaps structured_maps;
  ConstEvalEnv env = make_engine_consteval_env(ct_state, structured_maps);
  env.struct_defs = &module.struct_defs;
  env.struct_def_owner_index = &module.struct_def_owner_index;
  env.link_name_texts = module.link_name_texts.get();
  TypeBindings tpl_bindings;
  NttpBindings nttp_bindings;
  env = bind_consteval_call_env(expr->left, ce_fn_def, env, &tpl_bindings, &nttp_bindings);

  std::vector<ConstValue> args;
  args.reserve(expr->n_children);
  for (int i = 0; i < expr->n_children; ++i) {
    auto arg = evaluate_constant_expr(expr->children[i], env);
    if (!arg.ok()) return false;
    args.push_back(*arg.value);
  }

  auto result = evaluate_consteval_call(ce_fn_def, args, env, ct_state.consteval_fn_defs());
  if (!result.ok()) return false;
  if (out_value) *out_value = result.as_int();
  return true;
}

bool try_evaluate_static_assert_expr(
    const Node* expr,
    const Module& module,
    const CompileTimeState& ct_state,
    long long* out_value) {
  if (!expr) return false;
  EngineConstEvalStructuredMaps structured_maps;
  ConstEvalEnv env = make_engine_consteval_env(ct_state, structured_maps);
  env.struct_defs = &module.struct_defs;
  env.struct_def_owner_index = &module.struct_def_owner_index;
  env.link_name_texts = module.link_name_texts.get();

  if (auto r = evaluate_constant_expr(expr, env); r.ok()) {
    if (out_value) *out_value = r.as_int();
    return true;
  }

  long long call_value = 0;
  if (try_evaluate_consteval_call_expr(expr, module, ct_state, &call_value)) {
    if (out_value) *out_value = call_value;
    return true;
  }

  auto eval_child = [&](const Node* child, long long* value) {
    return try_evaluate_static_assert_expr(child, module, ct_state, value);
  };

  switch (expr->kind) {
    case NK_UNARY: {
      long long value = 0;
      if (!expr->op || !eval_child(expr->left, &value)) return false;
      if (std::strcmp(expr->op, "+") == 0) {
        if (out_value) *out_value = value;
        return true;
      }
      if (std::strcmp(expr->op, "-") == 0) {
        if (out_value) *out_value = -value;
        return true;
      }
      if (std::strcmp(expr->op, "~") == 0) {
        if (out_value) *out_value = ~value;
        return true;
      }
      if (std::strcmp(expr->op, "!") == 0) {
        if (out_value) *out_value = !value;
        return true;
      }
      return false;
    }
    case NK_TERNARY: {
      long long cond = 0;
      if (!eval_child(expr->cond ? expr->cond : expr->left, &cond)) return false;
      return eval_child(cond ? expr->then_ : expr->else_, out_value);
    }
    case NK_BINOP: {
      if (!expr->op) return false;
      long long lhs = 0;
      long long rhs = 0;
      if (std::strcmp(expr->op, "&&") == 0) {
        if (!eval_child(expr->left, &lhs)) return false;
        if (!lhs) {
          if (out_value) *out_value = 0;
          return true;
        }
        if (!eval_child(expr->right, &rhs)) return false;
        if (out_value) *out_value = !!rhs;
        return true;
      }
      if (std::strcmp(expr->op, "||") == 0) {
        if (!eval_child(expr->left, &lhs)) return false;
        if (lhs) {
          if (out_value) *out_value = 1;
          return true;
        }
        if (!eval_child(expr->right, &rhs)) return false;
        if (out_value) *out_value = !!rhs;
        return true;
      }
      if (!eval_child(expr->left, &lhs) || !eval_child(expr->right, &rhs)) return false;
      long long result = 0;
      if (std::strcmp(expr->op, "+") == 0) result = lhs + rhs;
      else if (std::strcmp(expr->op, "-") == 0) result = lhs - rhs;
      else if (std::strcmp(expr->op, "*") == 0) result = lhs * rhs;
      else if (std::strcmp(expr->op, "/") == 0) {
        if (rhs == 0) return false;
        result = lhs / rhs;
      } else if (std::strcmp(expr->op, "%") == 0) {
        if (rhs == 0) return false;
        result = lhs % rhs;
      } else if (std::strcmp(expr->op, "<<") == 0) result = lhs << rhs;
      else if (std::strcmp(expr->op, ">>") == 0) result = lhs >> rhs;
      else if (std::strcmp(expr->op, "&") == 0) result = lhs & rhs;
      else if (std::strcmp(expr->op, "|") == 0) result = lhs | rhs;
      else if (std::strcmp(expr->op, "^") == 0) result = lhs ^ rhs;
      else if (std::strcmp(expr->op, "<") == 0) result = lhs < rhs;
      else if (std::strcmp(expr->op, "<=") == 0) result = lhs <= rhs;
      else if (std::strcmp(expr->op, ">") == 0) result = lhs > rhs;
      else if (std::strcmp(expr->op, ">=") == 0) result = lhs >= rhs;
      else if (std::strcmp(expr->op, "==") == 0) result = lhs == rhs;
      else if (std::strcmp(expr->op, "!=") == 0) result = lhs != rhs;
      else return false;
      if (out_value) *out_value = result;
      return true;
    }
    default:
      return false;
  }
}

struct StaticAssertStep {
  const Module& module;
  CompileTimeState* ct_state = nullptr;
  size_t resolved = 0;
  size_t pending = 0;
  std::vector<CompileTimeDiagnostic> pending_diags;

  void run() {
    if (!ct_state) return;
    for (const Node* sa : ct_state->static_assert_nodes()) {
      if (!sa || sa->kind != NK_STATIC_ASSERT || !sa->left) continue;
      long long value = 0;
      if (try_evaluate_static_assert_expr(sa->left, module, *ct_state, &value)) {
        ++resolved;
        if (value == 0) {
          pending_diags.push_back(
              make_diag(CompileTimeDiagnostic::StaticAssert,
                        "_Static_assert condition is false", sa));
        }
      } else {
        ++pending;
        pending_diags.push_back(
            make_diag(CompileTimeDiagnostic::StaticAssert,
                      "_Static_assert requires an integer constant expression", sa));
      }
    }
  }
};

}  // namespace

CompileTimeEngineStats run_compile_time_engine(
    Module& module, std::shared_ptr<CompileTimeState> ct_state,
    DeferredInstantiateFn instantiate_fn,
    DeferredInstantiateTypeFn instantiate_type_fn) {
  CompileTimeEngineStats stats{};

  static constexpr int kMaxIterations = 8;

  size_t prev_templates_pending = SIZE_MAX;
  size_t prev_consteval_pending = SIZE_MAX;

  for (int iter = 0; iter < kMaxIterations; ++iter) {
    ++stats.iterations;

    // Step 0: observe pending type-driven template work discovered during lowering.
    PendingTemplateTypeStep ptt_step{ct_state ? ct_state.get() : nullptr,
                                     instantiate_type_fn};
    ptt_step.run();
    stats.template_types_resolved += ptt_step.resolved;
    stats.template_types_terminal += ptt_step.terminal;
    stats.template_types_pending = ptt_step.pending;

    // Step 1: template instantiation resolution (with optional deferred lowering).
    TemplateInstantiationStep tpl_step{module, instantiate_fn,
                                       ct_state ? ct_state.get() : nullptr};
    tpl_step.run();

    stats.templates_instantiated = tpl_step.resolved;
    stats.templates_pending = tpl_step.pending;
    stats.templates_deferred += tpl_step.newly_instantiated;
    stats.consteval_deferred += tpl_step.consteval_unlocked;

    // Step 2: evaluate PendingConstevalExpr nodes using engine-owned state.
    // Some come from the initial HIR build; others are unlocked later by
    // deferred template instantiation.
    PendingConstevalEvalStep pce_step{module,
                                     ct_state ? ct_state.get() : nullptr};
    pce_step.run();

    stats.consteval_deferred += pce_step.deferred_evaluated;
    sync_global_const_bindings(module, ct_state ? ct_state.get() : nullptr);

    // Step 3: consteval reduction verification.
    ConstevalReductionStep ce_step{module};
    ce_step.run();

    // Step 4: static_assert verification after compile-time convergence work.
    StaticAssertStep sa_step{module, ct_state ? ct_state.get() : nullptr};
    sa_step.run();

    stats.consteval_reduced = ce_step.reduced;
    stats.consteval_pending = ce_step.pending + pce_step.pending;

    // Convergence: no pending work remains in any step.
    size_t total_pending =
        ptt_step.pending + tpl_step.pending + pce_step.pending + ce_step.pending +
        sa_step.pending;
    if (total_pending == 0) {
      if (!sa_step.pending_diags.empty()) {
        stats.diagnostics.insert(stats.diagnostics.end(),
            sa_step.pending_diags.begin(), sa_step.pending_diags.end());
        stats.converged = false;
        break;
      }
      stats.converged = true;
      break;
    }

    // Check whether this iteration made progress compared to the previous one.
    // Progress means at least one pending count decreased.
    size_t cur_template_pending = ptt_step.pending + tpl_step.pending;
    size_t cur_consteval_pending = ce_step.pending + pce_step.pending;
    bool made_progress =
        ptt_step.completed() > 0 ||
         ptt_step.spawned_new_work > 0 ||
         tpl_step.newly_instantiated > 0 ||
         pce_step.evaluated > 0 ||
         sa_step.resolved > 0 ||
         cur_template_pending < prev_templates_pending ||
         cur_consteval_pending < prev_consteval_pending;

    prev_templates_pending = cur_template_pending;
    prev_consteval_pending = cur_consteval_pending;

    if (!made_progress) {
      // No progress — we've hit an irreducible set.  Collect diagnostics
      // for all pending items so the caller can report them.
      stats.converged = false;
      stats.diagnostics.insert(stats.diagnostics.end(),
          ptt_step.pending_diags.begin(), ptt_step.pending_diags.end());
      stats.diagnostics.insert(stats.diagnostics.end(),
          tpl_step.pending_diags.begin(), tpl_step.pending_diags.end());
      stats.diagnostics.insert(stats.diagnostics.end(),
          pce_step.pending_diags.begin(), pce_step.pending_diags.end());
      stats.diagnostics.insert(stats.diagnostics.end(),
          ce_step.pending_diags.begin(), ce_step.pending_diags.end());
      stats.diagnostics.insert(stats.diagnostics.end(),
          sa_step.pending_diags.begin(), sa_step.pending_diags.end());
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
      << ", " << stats.template_types_resolved << " template type"
      << (stats.template_types_resolved != 1 ? "s" : "")
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
  if (stats.template_types_pending > 0) {
    out << "\n  pending template types=" << stats.template_types_pending;
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
