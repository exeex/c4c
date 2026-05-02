#include "templates.hpp"

namespace c4c::hir {
namespace {

bool eval_template_arg_expr_with_nttp_bindings(const Node* expr,
                                               const NttpBindings* enclosing_nttp,
                                               const NttpTextBindings* enclosing_nttp_by_text,
                                               long long* out) {
  if (!expr || !out) return false;
  ConstEvalEnv env{};
  env.nttp_bindings = enclosing_nttp;
  env.nttp_bindings_by_text = enclosing_nttp_by_text;
  auto value = evaluate_constant_expr(expr, env);
  if (!value.ok()) return false;
  *out = value.as_int();
  return true;
}

bool typespec_has_complete_text_identity_for_deduction(const TypeSpec& ts) {
  if (ts.namespace_context_id < 0 || ts.tag_text_id == kInvalidText) {
    return false;
  }
  if (ts.n_qualifier_segments < 0) return false;
  if (ts.n_qualifier_segments > 0 && !ts.qualifier_text_ids) return false;
  for (int i = 0; i < ts.n_qualifier_segments; ++i) {
    if (ts.qualifier_text_ids[i] == kInvalidText) return false;
  }
  return true;
}

bool typespec_has_structured_nominal_identity_for_deduction(const TypeSpec& ts) {
  return (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) ||
         typespec_has_complete_text_identity_for_deduction(ts);
}

bool typespec_base_requires_nominal_identity_for_deduction(TypeBase base) {
  return base == TB_STRUCT || base == TB_UNION || base == TB_ENUM ||
         base == TB_TYPEDEF;
}

std::optional<bool> structured_typespec_nominal_match_for_deduction(
    const TypeSpec& existing_ts,
    const TypeSpec& deduced_ts) {
  const bool existing_has_record =
      existing_ts.record_def && existing_ts.record_def->kind == NK_STRUCT_DEF;
  const bool deduced_has_record =
      deduced_ts.record_def && deduced_ts.record_def->kind == NK_STRUCT_DEF;
  if (existing_has_record && deduced_has_record) {
    return existing_ts.record_def == deduced_ts.record_def;
  }

  const bool existing_has_text =
      typespec_has_complete_text_identity_for_deduction(existing_ts);
  const bool deduced_has_text =
      typespec_has_complete_text_identity_for_deduction(deduced_ts);
  if (existing_has_text && deduced_has_text) {
    if (existing_ts.namespace_context_id != deduced_ts.namespace_context_id ||
        existing_ts.tag_text_id != deduced_ts.tag_text_id ||
        existing_ts.is_global_qualified != deduced_ts.is_global_qualified ||
        existing_ts.n_qualifier_segments != deduced_ts.n_qualifier_segments) {
      return false;
    }
    for (int i = 0; i < existing_ts.n_qualifier_segments; ++i) {
      if (existing_ts.qualifier_text_ids[i] != deduced_ts.qualifier_text_ids[i]) {
        return false;
      }
    }
    return true;
  }

  if (typespec_has_structured_nominal_identity_for_deduction(existing_ts) ||
      typespec_has_structured_nominal_identity_for_deduction(deduced_ts)) {
    return false;
  }
  return std::nullopt;
}

bool typespec_nominal_match_for_deduction(const TypeSpec& existing_ts,
                                          const TypeSpec& deduced_ts) {
  if (std::optional<bool> structured_match =
          structured_typespec_nominal_match_for_deduction(existing_ts,
                                                          deduced_ts)) {
    return *structured_match;
  }
  return !typespec_base_requires_nominal_identity_for_deduction(existing_ts.base) &&
         !typespec_base_requires_nominal_identity_for_deduction(deduced_ts.base);
}

TextId template_param_carrier_text_id_for_deduction(const TypeSpec& ts) {
  if (ts.template_param_text_id != kInvalidText) return ts.template_param_text_id;
  return ts.tag_text_id;
}

std::optional<std::string> template_type_param_binding_name_for_deduction(
    const TypeSpec& ts,
    const Node* fn_def) {
  if (ts.base != TB_TYPEDEF || !fn_def || fn_def->n_template_params <= 0 ||
      !fn_def->template_param_names) {
    return std::nullopt;
  }
  if (ts.template_param_index >= 0 &&
      ts.template_param_index < fn_def->n_template_params &&
      fn_def->template_param_names[ts.template_param_index] &&
      !(fn_def->template_param_is_nttp &&
        fn_def->template_param_is_nttp[ts.template_param_index])) {
    const bool owner_matches =
        ts.template_param_owner_text_id != kInvalidText &&
        fn_def->unqualified_text_id != kInvalidText &&
        ts.template_param_owner_text_id == fn_def->unqualified_text_id &&
        (ts.template_param_owner_namespace_context_id < 0 ||
         ts.template_param_owner_namespace_context_id == fn_def->namespace_context_id);
    if (owner_matches) {
      return std::string(fn_def->template_param_names[ts.template_param_index]);
    }
  }

  const TextId carrier_text_id = template_param_carrier_text_id_for_deduction(ts);
  if (carrier_text_id == kInvalidText || !fn_def->template_param_name_text_ids) {
    return std::nullopt;
  }
  for (int i = 0; i < fn_def->n_template_params; ++i) {
    if (!fn_def->template_param_names[i]) continue;
    if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) {
      continue;
    }
    if (fn_def->template_param_name_text_ids[i] == carrier_text_id) {
      return std::string(fn_def->template_param_names[i]);
    }
  }
  return std::nullopt;
}

const TypeSpec* find_enclosing_type_binding_for_deduction(
    const TypeSpec& ts,
    const TypeBindings* enclosing_bindings,
    const TextTable* texts) {
  if (ts.base != TB_TYPEDEF || !enclosing_bindings || enclosing_bindings->empty()) {
    return nullptr;
  }
  const TextId carrier_text_id = template_param_carrier_text_id_for_deduction(ts);
  if (carrier_text_id != kInvalidText && texts) {
    const std::string key(texts->lookup(carrier_text_id));
    if (!key.empty()) {
      auto resolved = enclosing_bindings->find(key);
      if (resolved != enclosing_bindings->end()) return &resolved->second;
    }
  }
  if (carrier_text_id == kInvalidText && ts.template_param_index < 0) return nullptr;
  const TypeSpec* unique = nullptr;
  for (const auto& [key, value] : *enclosing_bindings) {
    if (key.find('#') != std::string::npos) continue;
    if (unique) return nullptr;
    unique = &value;
  }
  return unique;
}

}  // namespace

TypeBindings Lowerer::build_call_bindings(const Node* call_var, const Node* fn_def,
                                          const TypeBindings* enclosing_bindings) {
  TypeBindings bindings;
  if (!call_var || !fn_def || fn_def->n_template_params <= 0) return bindings;
  const int total_args = call_var->n_template_args > 0 ? call_var->n_template_args : 0;
  int arg_index = 0;
  for (int i = 0; i < fn_def->n_template_params; ++i) {
    if (!fn_def->template_param_names[i]) continue;
    const bool is_nttp =
        fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i];
    const bool is_pack =
        fn_def->template_param_is_pack && fn_def->template_param_is_pack[i];
    if (is_nttp) {
      if (is_pack) arg_index = total_args;
      else if (arg_index < total_args) ++arg_index;
      continue;
    }
    if (is_pack) {
      for (int pack_index = 0; arg_index < total_args; ++arg_index, ++pack_index) {
        TypeSpec arg_ts = call_var->template_arg_types[arg_index];
        if (const TypeSpec* resolved = find_enclosing_type_binding_for_deduction(
                arg_ts, enclosing_bindings,
                module_ ? module_->link_name_texts.get() : nullptr)) {
          arg_ts = *resolved;
        }
        bindings[pack_binding_name(fn_def->template_param_names[i], pack_index)] = arg_ts;
      }
      continue;
    }
    if (arg_index >= total_args) continue;
    TypeSpec arg_ts = call_var->template_arg_types[arg_index++];
    if (const TypeSpec* resolved = find_enclosing_type_binding_for_deduction(
            arg_ts, enclosing_bindings,
            module_ ? module_->link_name_texts.get() : nullptr)) {
      arg_ts = *resolved;
    }
    bindings[fn_def->template_param_names[i]] = arg_ts;
  }
  if (fn_def->template_param_has_default) {
    for (int i = 0; i < fn_def->n_template_params; ++i) {
      if (!fn_def->template_param_names[i]) continue;
      if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) continue;
      if (fn_def->template_param_is_pack && fn_def->template_param_is_pack[i]) continue;
      if (bindings.count(fn_def->template_param_names[i])) continue;
      if (fn_def->template_param_has_default[i]) {
        bindings[fn_def->template_param_names[i]] = fn_def->template_param_default_types[i];
      }
    }
  }
  return bindings;
}

NttpBindings Lowerer::build_call_nttp_bindings(const Node* call_var, const Node* fn_def,
                                               const NttpBindings* enclosing_nttp,
                                               const NttpTextBindings* enclosing_nttp_by_text) {
  NttpBindings bindings;
  if (!call_var || !fn_def || fn_def->n_template_params <= 0) return bindings;
  const int total_args = call_var->n_template_args > 0 ? call_var->n_template_args : 0;
  int arg_index = 0;
  for (int i = 0; i < fn_def->n_template_params; ++i) {
    if (!fn_def->template_param_names[i]) continue;
    const bool is_nttp =
        fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i];
    const bool is_pack =
        fn_def->template_param_is_pack && fn_def->template_param_is_pack[i];
    if (!is_nttp) {
      if (is_pack) arg_index = total_args;
      else if (arg_index < total_args) ++arg_index;
      continue;
    }
    if (is_pack) {
      for (int pack_index = 0; arg_index < total_args; ++arg_index, ++pack_index) {
        if (!(call_var->template_arg_is_value && call_var->template_arg_is_value[arg_index])) {
          continue;
        }
        const std::string key = pack_binding_name(fn_def->template_param_names[i], pack_index);
        long long expr_value = 0;
        if (call_var->template_arg_exprs && call_var->template_arg_exprs[arg_index]) {
          if (eval_template_arg_expr_with_nttp_bindings(
                  call_var->template_arg_exprs[arg_index], enclosing_nttp,
                  enclosing_nttp_by_text,
                  &expr_value)) {
            bindings[key] = expr_value;
            continue;
          }
        }
        if (call_var->template_arg_nttp_names && call_var->template_arg_nttp_names[arg_index]) {
          const TextId forwarded_text_id =
              call_var->template_arg_nttp_text_ids
                  ? call_var->template_arg_nttp_text_ids[arg_index]
                  : kInvalidText;
          if (forwarded_text_id != kInvalidText && enclosing_nttp_by_text) {
            auto text_it = enclosing_nttp_by_text->find(forwarded_text_id);
            if (text_it != enclosing_nttp_by_text->end()) {
              bindings[key] = text_it->second;
              continue;
            }
          }
          if (enclosing_nttp) {
            auto it = enclosing_nttp->find(call_var->template_arg_nttp_names[arg_index]);
            if (it != enclosing_nttp->end()) bindings[key] = it->second;
          }
          continue;
        }
        bindings[key] = call_var->template_arg_values[arg_index];
      }
      continue;
    }
    if (arg_index >= total_args) continue;
    if (call_var->template_arg_is_value && call_var->template_arg_is_value[arg_index]) {
      long long expr_value = 0;
      if (call_var->template_arg_exprs && call_var->template_arg_exprs[arg_index]) {
        if (eval_template_arg_expr_with_nttp_bindings(
                call_var->template_arg_exprs[arg_index], enclosing_nttp,
                enclosing_nttp_by_text,
                &expr_value)) {
          bindings[fn_def->template_param_names[i]] = expr_value;
          ++arg_index;
          continue;
        }
      }
      if (call_var->template_arg_nttp_names && call_var->template_arg_nttp_names[arg_index]) {
        const TextId forwarded_text_id =
            call_var->template_arg_nttp_text_ids
                ? call_var->template_arg_nttp_text_ids[arg_index]
                : kInvalidText;
        if (forwarded_text_id != kInvalidText && enclosing_nttp_by_text) {
          auto text_it = enclosing_nttp_by_text->find(forwarded_text_id);
          if (text_it != enclosing_nttp_by_text->end()) {
            bindings[fn_def->template_param_names[i]] = text_it->second;
            ++arg_index;
            continue;
          }
        }
        if (enclosing_nttp) {
          auto it = enclosing_nttp->find(call_var->template_arg_nttp_names[arg_index]);
          if (it != enclosing_nttp->end()) {
            bindings[fn_def->template_param_names[i]] = it->second;
            ++arg_index;
            continue;
          }
        }
        ++arg_index;
        continue;
      }
      bindings[fn_def->template_param_names[i]] = call_var->template_arg_values[arg_index];
    }
    ++arg_index;
  }
  if (fn_def->template_param_has_default) {
    for (int i = 0; i < fn_def->n_template_params; ++i) {
      if (!fn_def->template_param_names[i]) continue;
      if (!fn_def->template_param_is_nttp || !fn_def->template_param_is_nttp[i]) continue;
      if (fn_def->template_param_is_pack && fn_def->template_param_is_pack[i]) continue;
      if (bindings.count(fn_def->template_param_names[i])) continue;
      if (fn_def->template_param_has_default[i]) {
        bindings[fn_def->template_param_names[i]] = fn_def->template_param_default_values[i];
      }
    }
  }
  return bindings;
}

NttpTextBindings Lowerer::build_call_nttp_text_bindings(
    const Node* call_var, const Node* fn_def, const NttpBindings& nttp_bindings) {
  NttpTextBindings bindings_by_text;
  (void)call_var;
  if (!fn_def || !fn_def->template_param_name_text_ids ||
      fn_def->n_template_params <= 0) {
    return bindings_by_text;
  }
  for (int i = 0; i < fn_def->n_template_params; ++i) {
    if (!fn_def->template_param_names[i]) continue;
    const bool is_nttp =
        fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i];
    const bool is_pack =
        fn_def->template_param_is_pack && fn_def->template_param_is_pack[i];
    if (!is_nttp || is_pack) continue;
    const TextId text_id = fn_def->template_param_name_text_ids[i];
    if (text_id == kInvalidText) continue;
    auto it = nttp_bindings.find(fn_def->template_param_names[i]);
    if (it != nttp_bindings.end()) bindings_by_text[text_id] = it->second;
  }
  return bindings_by_text;
}

bool Lowerer::has_forwarded_nttp(const Node* call_var) {
  if (!call_var || !call_var->template_arg_nttp_names) return false;
  for (int i = 0; i < call_var->n_template_args; ++i) {
    if (call_var->template_arg_nttp_names[i]) return true;
  }
  return false;
}

std::optional<TypeSpec> Lowerer::try_infer_arg_type_for_deduction(
    const Node* expr, const Node* enclosing_fn) {
  if (!expr) return std::nullopt;

  if (resolved_types_) {
    if (auto ct = resolved_types_->lookup(expr)) {
      TypeSpec ts = sema::typespec_from_canonical(*ct);
      if (has_concrete_type(ts)) return ts;
    }
  }

  if (has_concrete_type(expr->type)) return expr->type;

  switch (expr->kind) {
    case NK_INT_LIT:
      return infer_int_literal_type(expr);
    case NK_FLOAT_LIT: {
      TypeSpec ts = expr->type;
      if (!has_concrete_type(ts))
        ts = classify_float_literal_type(const_cast<Node*>(expr));
      return ts;
    }
    case NK_CHAR_LIT: {
      TypeSpec ts = expr->type;
      if (!has_concrete_type(ts)) ts.base = TB_INT;
      return ts;
    }
    case NK_STR_LIT: {
      TypeSpec ts{};
      ts.base = TB_CHAR;
      ts.ptr_level = 1;
      return ts;
    }
    case NK_VAR: {
      if (!expr->name || !enclosing_fn) return std::nullopt;
      for (int i = 0; i < enclosing_fn->n_params; ++i) {
        const Node* p = enclosing_fn->params[i];
        if (p && p->name && std::strcmp(p->name, expr->name) == 0 &&
            has_concrete_type(p->type)) {
          return p->type;
        }
      }
      if (enclosing_fn->body) {
        const Node* body = enclosing_fn->body;
        for (int i = 0; i < body->n_children; ++i) {
          const Node* stmt = body->children[i];
          if (stmt && stmt->kind == NK_DECL && stmt->name &&
              std::strcmp(stmt->name, expr->name) == 0 &&
              has_concrete_type(stmt->type)) {
            return stmt->type;
          }
        }
      }
      return std::nullopt;
    }
    case NK_CAST:
      if (has_concrete_type(expr->type)) return expr->type;
      return std::nullopt;
    case NK_CALL:
    case NK_BUILTIN_CALL:
      if (resolved_types_ && expr->left) {
        if (auto callee_ct = resolved_types_->lookup(expr->left);
            callee_ct && sema::is_callable_type(*callee_ct)) {
          if (const auto* sig = sema::get_function_sig(*callee_ct);
              sig && sig->return_type) {
            return sema::typespec_from_canonical(*sig->return_type);
          }
        }
      }
      if (expr->left && expr->left->kind == NK_VAR && expr->left->name) {
        auto fit = function_decl_nodes_.find(expr->left->name);
        if (fit != function_decl_nodes_.end() &&
            fit->second && has_concrete_type(fit->second->type)) {
          return fit->second->type;
        }
      }
      if (auto call_ts = infer_call_result_type(nullptr, expr)) {
        return reference_value_ts(*call_ts);
      }
      return std::nullopt;
    case NK_ADDR:
      if (expr->left) {
        auto inner = try_infer_arg_type_for_deduction(expr->left, enclosing_fn);
        if (inner) {
          inner->ptr_level++;
          return inner;
        }
      }
      return std::nullopt;
    case NK_DEREF:
      if (expr->left) {
        auto inner = try_infer_arg_type_for_deduction(expr->left, enclosing_fn);
        if (inner && inner->ptr_level > 0) {
          inner->ptr_level--;
          return inner;
        }
      }
      return std::nullopt;
    case NK_UNARY:
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

TypeBindings Lowerer::try_deduce_template_type_args(
    const Node* call_node, const Node* fn_def, const Node* enclosing_fn) {
  TypeBindings deduced;
  if (!call_node || !fn_def || fn_def->n_template_params <= 0) return deduced;

  std::unordered_set<std::string> type_param_names;
  for (int i = 0; i < fn_def->n_template_params; ++i) {
    if (fn_def->template_param_names[i] &&
        !(fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i])) {
      type_param_names.insert(fn_def->template_param_names[i]);
    }
  }
  if (type_param_names.empty()) return deduced;

  const int n_args = call_node->n_children;
  const int n_params = fn_def->n_params;
  const int match_count = std::min(n_args, n_params);

  for (int i = 0; i < match_count; ++i) {
    const Node* param = fn_def->params[i];
    const Node* arg = call_node->children[i];
    if (!param || !arg) continue;

    const TypeSpec& param_ts = param->type;
    const std::optional<std::string> param_binding_name =
        template_type_param_binding_name_for_deduction(param_ts, fn_def);

    const bool is_forwarding_reference_param =
        param_binding_name &&
        param_ts.ptr_level == 0 && param_ts.array_rank == 0 &&
        param_ts.is_rvalue_ref &&
        !param_ts.is_const && !param_ts.is_volatile;

    const bool is_dependent_rvalue_ref_param =
        param_binding_name &&
        param_ts.ptr_level == 0 && param_ts.array_rank == 0 &&
        param_ts.is_rvalue_ref;

    if (is_forwarding_reference_param) {
      auto arg_type = try_infer_arg_type_for_deduction(arg, enclosing_fn);
      if (!arg_type) continue;
      TypeSpec deduced_ts = *arg_type;
      if (is_ast_lvalue(arg)) {
        deduced_ts.is_lvalue_ref = true;
        deduced_ts.is_rvalue_ref = false;
      }
      auto existing = deduced.find(*param_binding_name);
      if (existing != deduced.end()) {
        if (existing->second.base != deduced_ts.base ||
            existing->second.ptr_level != deduced_ts.ptr_level ||
            !typespec_nominal_match_for_deduction(existing->second, deduced_ts) ||
            existing->second.is_lvalue_ref != deduced_ts.is_lvalue_ref ||
            existing->second.is_rvalue_ref != deduced_ts.is_rvalue_ref) {
          return {};
        }
      } else {
        deduced[*param_binding_name] = deduced_ts;
      }
    } else if (is_dependent_rvalue_ref_param && is_ast_lvalue(arg)) {
      rejected_template_calls_.insert(call_node);
      return {};
    } else if (param_binding_name &&
               param_ts.ptr_level == 0 && param_ts.array_rank == 0) {
      auto arg_type = try_infer_arg_type_for_deduction(arg, enclosing_fn);
      if (!arg_type) continue;
      TypeSpec deduced_ts = *arg_type;
      auto existing = deduced.find(*param_binding_name);
      if (existing != deduced.end()) {
        if (existing->second.base != deduced_ts.base ||
            existing->second.ptr_level != deduced_ts.ptr_level ||
            !typespec_nominal_match_for_deduction(existing->second, deduced_ts)) {
          return {};
        }
      } else {
        deduced[*param_binding_name] = deduced_ts;
      }
    } else if (param_binding_name &&
               param_ts.ptr_level > 0 && param_ts.array_rank == 0) {
      auto arg_type = try_infer_arg_type_for_deduction(arg, enclosing_fn);
      if (!arg_type || arg_type->ptr_level < param_ts.ptr_level) continue;
      TypeSpec deduced_ts = *arg_type;
      deduced_ts.ptr_level -= param_ts.ptr_level;
      auto existing = deduced.find(*param_binding_name);
      if (existing != deduced.end()) {
        if (existing->second.base != deduced_ts.base ||
            existing->second.ptr_level != deduced_ts.ptr_level) {
          return {};
        }
      } else {
        deduced[*param_binding_name] = deduced_ts;
      }
    }
  }

  return deduced;
}

bool Lowerer::deduction_covers_all_type_params(const TypeBindings& deduced,
                                               const Node* fn_def) {
  for (int i = 0; i < fn_def->n_template_params; ++i) {
    if (!fn_def->template_param_names[i]) continue;
    if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) continue;
    const std::string pname = fn_def->template_param_names[i];
    if (deduced.count(pname)) continue;
    if (fn_def->template_param_has_default && fn_def->template_param_has_default[i]) continue;
    return false;
  }
  return true;
}

void Lowerer::fill_deduced_defaults(TypeBindings& deduced, const Node* fn_def) {
  if (!fn_def->template_param_has_default) return;
  for (int i = 0; i < fn_def->n_template_params; ++i) {
    if (!fn_def->template_param_names[i]) continue;
    if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) continue;
    const std::string pname = fn_def->template_param_names[i];
    if (deduced.count(pname)) continue;
    if (fn_def->template_param_has_default[i]) {
      deduced[pname] = fn_def->template_param_default_types[i];
    }
  }
}

TypeBindings Lowerer::merge_explicit_and_deduced_type_bindings(
    const Node* call_node, const Node* call_var, const Node* fn_def,
    const TypeBindings* enclosing_bindings, const Node* enclosing_fn) {
  TypeBindings bindings = build_call_bindings(call_var, fn_def, enclosing_bindings);
  if (!call_node || !fn_def) return bindings;

  TypeBindings deduced = try_deduce_template_type_args(call_node, fn_def, enclosing_fn);
  for (const auto& [name, ts] : deduced) {
    bindings.emplace(name, ts);
  }
  fill_deduced_defaults(bindings, fn_def);
  return bindings;
}

}  // namespace c4c::hir
