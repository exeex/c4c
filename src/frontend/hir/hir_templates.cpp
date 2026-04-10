#include "hir_lowerer_internal.hpp"
#include "consteval.hpp"

#include <climits>
#include <cstring>
#include <functional>

namespace c4c::hir {

std::string encode_template_type_arg_ref_hir(const TypeSpec& ts);

namespace {

std::string encode_template_arg_ref_hir(const TemplateArgRef& arg) {
  if (arg.debug_text && arg.debug_text[0]) return arg.debug_text;
  if (arg.kind == TemplateArgKind::Value) return std::to_string(arg.value);
  if (arg.kind == TemplateArgKind::Type &&
      (has_concrete_type(arg.type) || arg.type.tpl_struct_origin)) {
    return encode_template_type_arg_ref_hir(arg.type);
  }
  return {};
}

std::vector<std::string> encode_template_arg_ref_list_hir(const TypeSpec& ts) {
  std::vector<std::string> refs;
  if (!ts.tpl_struct_args.data || ts.tpl_struct_args.size <= 0) return refs;
  refs.reserve(ts.tpl_struct_args.size);
  for (int i = 0; i < ts.tpl_struct_args.size; ++i) {
    refs.push_back(encode_template_arg_ref_hir(ts.tpl_struct_args.data[i]));
  }
  return refs;
}

std::string encode_template_arg_debug_compat_hir(const TypeSpec& ts) {
  std::string out;
  if (!ts.tpl_struct_args.data || ts.tpl_struct_args.size <= 0) return out;
  for (int i = 0; i < ts.tpl_struct_args.size; ++i) {
    if (i > 0) out += ",";
    out += encode_template_arg_ref_hir(ts.tpl_struct_args.data[i]);
  }
  return out;
}

bool matches_trait_family(const std::string& name, const char* suffix);

std::optional<TypeSpec> try_resolve_unary_type_transform_trait(
    const std::string& tpl_name,
    const std::string& member,
    const std::vector<HirTemplateArg>& actual_args) {
  if (member != "type" || actual_args.size() != 1 || actual_args[0].is_value) {
    return std::nullopt;
  }

  TypeSpec resolved = actual_args[0].type;
  if (matches_trait_family(tpl_name, "remove_const")) {
    resolved.is_const = false;
    return resolved;
  }
  if (matches_trait_family(tpl_name, "remove_volatile")) {
    resolved.is_volatile = false;
    return resolved;
  }
  if (matches_trait_family(tpl_name, "remove_cv")) {
    resolved.is_const = false;
    resolved.is_volatile = false;
    return resolved;
  }
  if (matches_trait_family(tpl_name, "remove_reference")) {
    resolved.is_lvalue_ref = false;
    resolved.is_rvalue_ref = false;
    return resolved;
  }
  return std::nullopt;
}

bool matches_trait_family(const std::string& name, const char* suffix) {
  if (name == suffix) return true;
  if (name.size() <= std::strlen(suffix) + 2) return false;
  return name.compare(name.size() - std::strlen(suffix), std::strlen(suffix), suffix) == 0 &&
         name.compare(name.size() - std::strlen(suffix) - 2, 2, "::") == 0;
}

void assign_template_arg_refs_from_hir_args(
    TypeSpec* ts,
    const std::vector<HirTemplateArg>& args) {
  if (!ts) return;
  ts->tpl_struct_args.data = nullptr;
  ts->tpl_struct_args.size = 0;
  if (args.empty()) return;

  ts->tpl_struct_args.data = new TemplateArgRef[args.size()]();
  ts->tpl_struct_args.size = static_cast<int>(args.size());
  for (size_t i = 0; i < args.size(); ++i) {
    const HirTemplateArg& arg = args[i];
    TemplateArgRef& out = ts->tpl_struct_args.data[i];
    out.kind = arg.is_value ? TemplateArgKind::Value
                            : TemplateArgKind::Type;
    out.type = arg.is_value ? TypeSpec{} : arg.type;
    out.value = arg.is_value ? arg.value : 0;
    const std::string debug_text =
        arg.is_value ? std::to_string(arg.value)
                     : encode_template_type_arg_ref_hir(arg.type);
    out.debug_text =
        debug_text.empty() ? nullptr : ::strdup(debug_text.c_str());
  }
}

}  // namespace

std::string encode_template_type_arg_ref_hir(const TypeSpec& ts) {
  if (ts.tpl_struct_origin && ts.tpl_struct_origin[0]) {
    std::string ref = "@";
    ref += ts.tpl_struct_origin;
    ref += ":";
    ref += encode_template_arg_debug_compat_hir(ts);
    if (ts.deferred_member_type_name && ts.deferred_member_type_name[0]) {
      ref += "$";
      ref += ts.deferred_member_type_name;
    }
    return ref;
  }

  const bool plain_tag_ref =
      ts.tag && ts.tag[0] && !ts.is_const && !ts.is_volatile &&
      ts.ptr_level == 0 && !ts.is_lvalue_ref && !ts.is_rvalue_ref;
  if (plain_tag_ref) return ts.tag;

  std::string ref;
  if (ts.is_const) ref += "const_";
  if (ts.is_volatile) ref += "volatile_";
  switch (ts.base) {
    case TB_INT: ref += "int"; break;
    case TB_UINT: ref += "uint"; break;
    case TB_CHAR: ref += "char"; break;
    case TB_SCHAR: ref += "schar"; break;
    case TB_UCHAR: ref += "uchar"; break;
    case TB_SHORT: ref += "short"; break;
    case TB_USHORT: ref += "ushort"; break;
    case TB_LONG: ref += "long"; break;
    case TB_ULONG: ref += "ulong"; break;
    case TB_LONGLONG: ref += "llong"; break;
    case TB_ULONGLONG: ref += "ullong"; break;
    case TB_FLOAT: ref += "float"; break;
    case TB_DOUBLE: ref += "double"; break;
    case TB_LONGDOUBLE: ref += "ldouble"; break;
    case TB_VOID: ref += "void"; break;
    case TB_BOOL: ref += "bool"; break;
    case TB_INT128: ref += "i128"; break;
    case TB_UINT128: ref += "u128"; break;
    case TB_STRUCT: ref += "struct_"; ref += ts.tag ? ts.tag : "anon"; break;
    case TB_UNION: ref += "union_"; ref += ts.tag ? ts.tag : "anon"; break;
    case TB_ENUM: ref += "enum_"; ref += ts.tag ? ts.tag : "anon"; break;
    case TB_TYPEDEF: ref += ts.tag ? ts.tag : "typedef"; break;
    default: return "?";
  }
  for (int p = 0; p < ts.ptr_level; ++p) ref += "_ptr";
  if (ts.is_lvalue_ref) ref += "_ref";
  if (ts.is_rvalue_ref) ref += "_rref";
  return ref;
}

bool eval_struct_static_member_value_hir(
    const Node* sdef,
    const std::unordered_map<std::string, const Node*>& struct_defs,
    const std::string& member_name,
    const NttpBindings* nttp_bindings,
    long long* out) {
  if (!sdef) return false;
  static const std::unordered_map<std::string, Node*> kEmptyStructs;
  static const std::unordered_map<std::string, long long> kEmptyConsts;

  auto search_decl_array = [&](Node* const* decls, int count) -> bool {
    if (!decls) return false;
    for (int fi = 0; fi < count; ++fi) {
      const Node* f = decls[fi];
      if (!f || f->kind != NK_DECL || !f->is_static || !f->name) continue;
      if (member_name != f->name) continue;
      long long val = 0;
      if (f->init && nttp_bindings &&
          eval_const_int(const_cast<Node*>(f->init), &val, &kEmptyStructs,
                         nttp_bindings)) {
        *out = val;
        return true;
      }
      if (f->init && eval_const_int(f->init, &val, &kEmptyStructs, &kEmptyConsts)) {
        *out = val;
        return true;
      }
      if (f->init && f->init->kind == NK_INT_LIT) {
        *out = f->init->ival;
        return true;
      }
    }
    return false;
  };

  if (search_decl_array(sdef->fields, sdef->n_fields)) return true;
  if (search_decl_array(sdef->children, sdef->n_children)) return true;
  for (int bi = 0; bi < sdef->n_bases; ++bi) {
    const TypeSpec& base_ts = sdef->base_types[bi];
    if (!base_ts.tag || !base_ts.tag[0]) continue;
    auto it = struct_defs.find(base_ts.tag);
    if (it == struct_defs.end()) continue;
    if (eval_struct_static_member_value_hir(it->second, struct_defs, member_name,
                                            nttp_bindings, out)) {
      return true;
    }
  }
  if (sdef->template_origin_name && sdef->template_origin_name[0]) {
    auto it = struct_defs.find(sdef->template_origin_name);
    if (it != struct_defs.end() &&
        eval_struct_static_member_value_hir(it->second, struct_defs, member_name,
                                            nttp_bindings, out)) {
      return true;
    }
  }
  return false;
}

bool Lowerer::is_referenced_without_template_args(
    const char* fn_name, const std::vector<const Node*>& items) {
  if (!fn_name) return false;
  for (const Node* item : items) {
    if (item->kind != NK_FUNCTION || !item->body) continue;
    if (item->n_template_params > 0) continue;
    if (has_plain_call(item->body, fn_name)) return true;
  }
  return false;
}

bool Lowerer::has_plain_call(const Node* n, const char* fn_name) {
  if (!n) return false;
  if (n->kind == NK_CALL && n->left && n->left->kind == NK_VAR &&
      n->left->name && std::strcmp(n->left->name, fn_name) == 0 &&
      n->left->n_template_args == 0) {
    return true;
  }
  if (has_plain_call(n->left, fn_name)) return true;
  if (has_plain_call(n->right, fn_name)) return true;
  if (has_plain_call(n->cond, fn_name)) return true;
  if (has_plain_call(n->then_, fn_name)) return true;
  if (has_plain_call(n->else_, fn_name)) return true;
  if (has_plain_call(n->body, fn_name)) return true;
  if (has_plain_call(n->init, fn_name)) return true;
  if (has_plain_call(n->update, fn_name)) return true;
  for (int i = 0; i < n->n_children; ++i)
    if (has_plain_call(n->children[i], fn_name)) return true;
  return false;
}

bool Lowerer::template_struct_has_pack_params(const Node* primary_tpl) {
  if (!primary_tpl || !primary_tpl->template_param_is_pack) return false;
  for (int pi = 0; pi < primary_tpl->n_template_params; ++pi) {
    if (primary_tpl->template_param_is_pack[pi]) return true;
  }
  return false;
}

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
        if (arg_ts.base == TB_TYPEDEF && arg_ts.tag && enclosing_bindings) {
          auto resolved = enclosing_bindings->find(arg_ts.tag);
          if (resolved != enclosing_bindings->end()) arg_ts = resolved->second;
        }
        bindings[pack_binding_name(fn_def->template_param_names[i], pack_index)] = arg_ts;
      }
      continue;
    }
    if (arg_index >= total_args) continue;
    TypeSpec arg_ts = call_var->template_arg_types[arg_index++];
    if (arg_ts.base == TB_TYPEDEF && arg_ts.tag && enclosing_bindings) {
      auto resolved = enclosing_bindings->find(arg_ts.tag);
      if (resolved != enclosing_bindings->end()) arg_ts = resolved->second;
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
                                               const NttpBindings* enclosing_nttp) {
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
        if (call_var->template_arg_nttp_names && call_var->template_arg_nttp_names[arg_index]) {
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
      if (call_var->template_arg_nttp_names && call_var->template_arg_nttp_names[arg_index]) {
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

    const bool is_forwarding_reference_param =
        param_ts.base == TB_TYPEDEF && param_ts.tag &&
        type_param_names.count(param_ts.tag) &&
        param_ts.ptr_level == 0 && param_ts.array_rank == 0 &&
        param_ts.is_rvalue_ref &&
        !param_ts.is_const && !param_ts.is_volatile;

    const bool is_dependent_rvalue_ref_param =
        param_ts.base == TB_TYPEDEF && param_ts.tag &&
        type_param_names.count(param_ts.tag) &&
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
      auto existing = deduced.find(param_ts.tag);
      if (existing != deduced.end()) {
        if (existing->second.base != deduced_ts.base ||
            existing->second.ptr_level != deduced_ts.ptr_level ||
            existing->second.tag != deduced_ts.tag ||
            existing->second.is_lvalue_ref != deduced_ts.is_lvalue_ref ||
            existing->second.is_rvalue_ref != deduced_ts.is_rvalue_ref) {
          return {};
        }
      } else {
        deduced[param_ts.tag] = deduced_ts;
      }
    } else if (is_dependent_rvalue_ref_param && is_ast_lvalue(arg)) {
      rejected_template_calls_.insert(call_node);
      return {};
    } else if (param_ts.base == TB_TYPEDEF && param_ts.tag &&
               type_param_names.count(param_ts.tag) &&
               param_ts.ptr_level == 0 && param_ts.array_rank == 0) {
      auto arg_type = try_infer_arg_type_for_deduction(arg, enclosing_fn);
      if (!arg_type) continue;
      TypeSpec deduced_ts = *arg_type;
      auto existing = deduced.find(param_ts.tag);
      if (existing != deduced.end()) {
        if (existing->second.base != deduced_ts.base ||
            existing->second.ptr_level != deduced_ts.ptr_level ||
            existing->second.tag != deduced_ts.tag) {
          return {};
        }
      } else {
        deduced[param_ts.tag] = deduced_ts;
      }
    } else if (param_ts.base == TB_TYPEDEF && param_ts.tag &&
               type_param_names.count(param_ts.tag) &&
               param_ts.ptr_level > 0 && param_ts.array_rank == 0) {
      auto arg_type = try_infer_arg_type_for_deduction(arg, enclosing_fn);
      if (!arg_type || arg_type->ptr_level < param_ts.ptr_level) continue;
      TypeSpec deduced_ts = *arg_type;
      deduced_ts.ptr_level -= param_ts.ptr_level;
      auto existing = deduced.find(param_ts.tag);
      if (existing != deduced.end()) {
        if (existing->second.base != deduced_ts.base ||
            existing->second.ptr_level != deduced_ts.ptr_level) {
          return {};
        }
      } else {
        deduced[param_ts.tag] = deduced_ts;
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

namespace {

bool parse_pack_binding_name(const std::string& key,
                             const std::string& base,
                             int* out_index = nullptr) {
  if (key.size() <= base.size() + 1) return false;
  if (key.compare(0, base.size(), base) != 0) return false;
  if (key[base.size()] != '#') return false;
  int index = 0;
  try {
    index = std::stoi(key.substr(base.size() + 1));
  } catch (...) {
    return false;
  }
  if (index < 0) return false;
  if (out_index) *out_index = index;
  return true;
}

long long count_pack_bindings_for_name(
    const std::unordered_map<std::string, TypeSpec>& bindings,
    const std::unordered_map<std::string, long long>& nttp_bindings,
    const std::string& base) {
  int max_index = -1;
  for (const auto& [key, _] : bindings) {
    int pack_index = 0;
    if (parse_pack_binding_name(key, base, &pack_index))
      max_index = std::max(max_index, pack_index);
  }
  for (const auto& [key, _] : nttp_bindings) {
    int pack_index = 0;
    if (parse_pack_binding_name(key, base, &pack_index))
      max_index = std::max(max_index, pack_index);
  }
  return max_index + 1;
}

struct DeferredNttpExprCursor {
  const std::string& input;
  size_t pos = 0;

  void skip_ws() {
    while (pos < input.size() &&
           std::isspace(static_cast<unsigned char>(input[pos]))) {
      ++pos;
    }
  }

  bool consume(const char* text) {
    skip_ws();
    const size_t len = std::strlen(text);
    if (input.compare(pos, len, text) == 0) {
      pos += len;
      return true;
    }
    return false;
  }

  std::string parse_identifier() {
    skip_ws();
    const size_t start = pos;
    if (pos >= input.size() ||
        !(std::isalpha(static_cast<unsigned char>(input[pos])) ||
          input[pos] == '_')) {
      return {};
    }
    ++pos;
    while (pos < input.size() &&
           (std::isalnum(static_cast<unsigned char>(input[pos])) ||
            input[pos] == '_')) {
      ++pos;
    }
    return input.substr(start, pos - start);
  }

  bool parse_number(long long* out_val) {
    skip_ws();
    const size_t start = pos;
    if (pos < input.size() &&
        std::isdigit(static_cast<unsigned char>(input[pos]))) {
      ++pos;
      while (pos < input.size() &&
             std::isdigit(static_cast<unsigned char>(input[pos]))) {
        ++pos;
      }
      *out_val =
          std::strtoll(input.substr(start, pos - start).c_str(), nullptr, 10);
      return true;
    }
    return false;
  }

  std::string parse_arg_text() {
    skip_ws();
    const size_t start = pos;
    int angle_depth = 0;
    int paren_depth = 0;
    while (pos < input.size()) {
      const char ch = input[pos];
      if (ch == '<') ++angle_depth;
      else if (ch == '>') {
        if (angle_depth == 0) break;
        --angle_depth;
      } else if (ch == '(') {
        ++paren_depth;
      } else if (ch == ')') {
        if (paren_depth > 0) --paren_depth;
      } else if (ch == ',' && angle_depth == 0 && paren_depth == 0) {
        break;
      }
      ++pos;
    }
    return trim_copy(input.substr(start, pos - start));
  }

  bool at_end() {
    skip_ws();
    return pos == input.size();
  }
};

struct DeferredNttpExprEnv {
  std::unordered_map<std::string, TypeSpec> type_lookup;
  std::unordered_map<std::string, long long> nttp_lookup;

  static DeferredNttpExprEnv from_bindings(
      const std::vector<std::pair<std::string, TypeSpec>>& type_bindings_vec,
      const std::vector<std::pair<std::string, long long>>& nttp_bindings_vec) {
    DeferredNttpExprEnv env;
    for (const auto& [name, ts_val] : type_bindings_vec) {
      env.type_lookup[name] = ts_val;
    }
    for (const auto& [name, val] : nttp_bindings_vec) {
      env.nttp_lookup[name] = val;
    }
    return env;
  }

  bool resolve_arg_text(const std::string& text, HirTemplateArg* out_arg) const {
    if (!out_arg || text.empty()) return false;
    if (text.size() > 1 && text[0] == '@') {
      const size_t colon = text.find(':', 1);
      if (colon == std::string::npos) return false;
      const std::string inner_origin = text.substr(1, colon - 1);
      const size_t member_sep = text.find('$', colon + 1);
      const std::string inner_args =
          member_sep == std::string::npos
              ? text.substr(colon + 1)
              : text.substr(colon + 1, member_sep - (colon + 1));
      std::function<bool(const std::string&, HirTemplateArg*)> parse_nested_arg;
      parse_nested_arg = [&](const std::string& ref,
                             HirTemplateArg* nested_out) -> bool {
        if (!nested_out || ref.empty()) return false;
        auto tit_inner = type_lookup.find(ref);
        if (tit_inner != type_lookup.end()) {
          nested_out->is_value = false;
          nested_out->type = tit_inner->second;
          return true;
        }
        auto nit_inner = nttp_lookup.find(ref);
        if (nit_inner != nttp_lookup.end()) {
          nested_out->is_value = true;
          nested_out->value = nit_inner->second;
          return true;
        }
        if (ref == "true" || ref == "false") {
          nested_out->is_value = true;
          nested_out->value = (ref == "true") ? 1 : 0;
          return true;
        }
        char* nested_end = nullptr;
        const long long parsed_val = std::strtoll(ref.c_str(), &nested_end, 10);
        if (nested_end && *nested_end == '\0') {
          nested_out->is_value = true;
          nested_out->value = parsed_val;
          return true;
        }
        if (ref.size() > 1 && ref[0] == '@') {
          return resolve_arg_text(ref, nested_out);
        }
        TypeSpec nested_builtin{};
        if (parse_builtin_typespec_text(ref, &nested_builtin)) {
          nested_out->is_value = false;
          nested_out->type = nested_builtin;
          return true;
        }
        return false;
      };

      TypeSpec nested_ts{};
      nested_ts.base = TB_STRUCT;
      nested_ts.array_size = -1;
      nested_ts.inner_rank = -1;
      nested_ts.tpl_struct_origin = ::strdup(inner_origin.c_str());
      if (!inner_args.empty()) {
        std::vector<HirTemplateArg> nested_args;
        const auto parts = split_deferred_template_arg_refs(inner_args);
        nested_args.reserve(parts.size());
        for (const auto& part : parts) {
          HirTemplateArg nested_arg{};
          if (!parse_nested_arg(part, &nested_arg)) return false;
          nested_args.push_back(nested_arg);
        }
        assign_template_arg_refs_from_hir_args(&nested_ts, nested_args);
      }
      if (member_sep != std::string::npos && member_sep + 1 < text.size()) {
        nested_ts.deferred_member_type_name =
            ::strdup(text.substr(member_sep + 1).c_str());
      }
      out_arg->is_value = false;
      out_arg->type = nested_ts;
      return true;
    }
    auto tit = type_lookup.find(text);
    if (tit != type_lookup.end()) {
      out_arg->is_value = false;
      out_arg->type = tit->second;
      return true;
    }
    auto nit = nttp_lookup.find(text);
    if (nit != nttp_lookup.end()) {
      out_arg->is_value = true;
      out_arg->value = nit->second;
      return true;
    }
    if (text == "true" || text == "false") {
      out_arg->is_value = true;
      out_arg->value = (text == "true") ? 1 : 0;
      return true;
    }
    char* end = nullptr;
    long long parsed = std::strtoll(text.c_str(), &end, 10);
    if (end && *end == '\0') {
      out_arg->is_value = true;
      out_arg->value = parsed;
      return true;
    }
    TypeSpec builtin{};
    if (parse_builtin_typespec_text(text, &builtin)) {
      out_arg->is_value = false;
      out_arg->type = builtin;
      return true;
    }
    return false;
  }

  bool lookup_bound_value(const std::string& ident, long long* out_val) const {
    auto nit = nttp_lookup.find(ident);
    if (nit != nttp_lookup.end()) {
      *out_val = nit->second;
      return true;
    }
    if (ident == "true") {
      *out_val = 1;
      return true;
    }
    if (ident == "false") {
      *out_val = 0;
      return true;
    }
    return false;
  }

  bool lookup_cast_type(const std::string& ident, TypeSpec* out_ts) const {
    auto tit = type_lookup.find(ident);
    if (tit != type_lookup.end()) {
      *out_ts = tit->second;
      return true;
    }
    return parse_builtin_typespec_text(ident, out_ts);
  }

  long long count_pack_bindings(const std::string& pack_name) const {
    return count_pack_bindings_for_name(type_lookup, nttp_lookup, pack_name);
  }
};

struct DeferredNttpTemplateLookup {
  const std::unordered_map<std::string, const Node*>& template_defs;
  const std::unordered_map<std::string, std::vector<const Node*>>&
      specializations;
  const std::unordered_map<std::string, const Node*>& struct_defs;

  bool eval_template_static_member_lookup(
      const std::string& tpl_name,
      const std::vector<HirTemplateArg>& actual_args,
      const std::string& member_name,
      long long* out_val) const {
    auto tpl_it = template_defs.find(tpl_name);
    if (tpl_it == template_defs.end()) return false;
    const Node* ref_primary = tpl_it->second;

    TemplateStructEnv ref_env;
    ref_env.primary_def = ref_primary;
    if (ref_primary && ref_primary->name) {
      auto it = specializations.find(ref_primary->name);
      if (it != specializations.end()) ref_env.specialization_patterns = &it->second;
    }

    SelectedTemplateStructPattern ref_selection =
        select_template_struct_pattern_hir(actual_args, ref_env);
    if (!ref_selection.selected_pattern) return false;

    std::string ref_mangled;
    if (ref_selection.selected_pattern != ref_primary &&
        ref_selection.selected_pattern->n_template_params == 0 &&
        ref_selection.selected_pattern->name &&
        ref_selection.selected_pattern->name[0]) {
      ref_mangled = ref_selection.selected_pattern->name;
    } else {
      const std::string ref_family =
          (ref_selection.selected_pattern &&
           ref_selection.selected_pattern->template_origin_name &&
           ref_selection.selected_pattern->template_origin_name[0])
              ? ref_selection.selected_pattern->template_origin_name
              : tpl_name;
      ref_mangled = ref_family;
      for (int pi = 0; pi < ref_primary->n_template_params; ++pi) {
        ref_mangled += "_";
        ref_mangled += ref_primary->template_param_names[pi];
        ref_mangled += "_";
        if (pi < static_cast<int>(actual_args.size()) && actual_args[pi].is_value) {
          ref_mangled += std::to_string(actual_args[pi].value);
        } else if (pi < static_cast<int>(actual_args.size()) &&
                   !actual_args[pi].is_value) {
          ref_mangled += type_suffix_for_mangling(actual_args[pi].type);
        }
      }
    }

    auto concrete_it = struct_defs.find(ref_mangled);
    if (concrete_it != struct_defs.end()) {
      if (eval_struct_static_member_value_hir(
              concrete_it->second, struct_defs, member_name, nullptr, out_val)) {
        return true;
      }
    }

    return eval_struct_static_member_value_hir(
        ref_selection.selected_pattern, struct_defs, member_name,
        &ref_selection.nttp_bindings, out_val);
  }
};

struct DeferredNttpExprParser {
  using ParseExprFn = bool (DeferredNttpExprParser::*)(long long*);
  using ApplyBinaryFn = bool (*)(long long*, long long);

  struct BinaryOpSpec {
    const char* token;
    ApplyBinaryFn apply;
  };

  DeferredNttpExprCursor cursor;
  const DeferredNttpTemplateLookup& template_lookup;
  const DeferredNttpExprEnv& env;

  static long long apply_integral_cast(TypeSpec ts, long long v) {
    if (ts.ptr_level != 0) return v;
    int bits = 0;
    switch (ts.base) {
      case TB_BOOL: bits = 1; break;
      case TB_CHAR:
      case TB_UCHAR:
      case TB_SCHAR: bits = 8; break;
      case TB_SHORT:
      case TB_USHORT: bits = 16; break;
      case TB_INT:
      case TB_UINT:
      case TB_ENUM: bits = 32; break;
      default: break;
    }
    if (bits <= 0 || bits >= 64) return v;
    long long mask = (1LL << bits) - 1;
    v &= mask;
    if (!is_unsigned_base(ts.base) && ts.base != TB_BOOL && (v >> (bits - 1)))
      v |= ~mask;
    return v;
  }

  static bool apply_mul_op(long long* lhs, long long rhs) {
    *lhs *= rhs;
    return true;
  }

  static bool apply_div_op(long long* lhs, long long rhs) {
    if (rhs == 0) return false;
    *lhs /= rhs;
    return true;
  }

  static bool apply_add_op(long long* lhs, long long rhs) {
    *lhs += rhs;
    return true;
  }

  static bool apply_sub_op(long long* lhs, long long rhs) {
    *lhs -= rhs;
    return true;
  }

  static bool apply_le_op(long long* lhs, long long rhs) {
    *lhs = (*lhs <= rhs) ? 1 : 0;
    return true;
  }

  static bool apply_ge_op(long long* lhs, long long rhs) {
    *lhs = (*lhs >= rhs) ? 1 : 0;
    return true;
  }

  static bool apply_lt_op(long long* lhs, long long rhs) {
    *lhs = (*lhs < rhs) ? 1 : 0;
    return true;
  }

  static bool apply_gt_op(long long* lhs, long long rhs) {
    *lhs = (*lhs > rhs) ? 1 : 0;
    return true;
  }

  static bool apply_eq_op(long long* lhs, long long rhs) {
    *lhs = (*lhs == rhs) ? 1 : 0;
    return true;
  }

  static bool apply_ne_op(long long* lhs, long long rhs) {
    *lhs = (*lhs != rhs) ? 1 : 0;
    return true;
  }

  static bool apply_and_op(long long* lhs, long long rhs) {
    *lhs = (*lhs && rhs) ? 1 : 0;
    return true;
  }

  static bool apply_or_op(long long* lhs, long long rhs) {
    *lhs = (*lhs || rhs) ? 1 : 0;
    return true;
  }

  bool parse_left_associative(
      long long* out_val, ParseExprFn operand_parser,
      std::initializer_list<BinaryOpSpec> ops) {
    if (!(this->*operand_parser)(out_val)) return false;
    while (true) {
      cursor.skip_ws();
      const BinaryOpSpec* matched = nullptr;
      for (const BinaryOpSpec& spec : ops) {
        if (cursor.consume(spec.token)) {
          matched = &spec;
          break;
        }
      }
      if (!matched) break;
      long long rhs = 0;
      if (!(this->*operand_parser)(&rhs)) return false;
      if (!matched->apply(out_val, rhs)) return false;
    }
    return true;
  }

  bool resolve_arg(const std::string& text, HirTemplateArg* out_arg) {
    return env.resolve_arg_text(text, out_arg);
  }

  bool eval_member_lookup(long long* out_val) {
    const std::string tpl_name = cursor.parse_identifier();
    if (tpl_name.empty() || !cursor.consume("<")) return false;

    std::vector<HirTemplateArg> actual_args;
    if (!parse_template_arg_list(&actual_args)) return false;

    std::string member_name;
    if (!parse_template_member_name(&member_name)) return false;
    return template_lookup.eval_template_static_member_lookup(
        tpl_name, actual_args, member_name, out_val);
  }

  bool parse_template_arg_list(std::vector<HirTemplateArg>* out_args) {
    if (!out_args) return false;
    cursor.skip_ws();
    if (!cursor.consume(">")) {
      while (true) {
        HirTemplateArg arg;
        if (!resolve_arg(cursor.parse_arg_text(), &arg)) return false;
        out_args->push_back(arg);
        cursor.skip_ws();
        if (cursor.consume(">")) break;
        if (!cursor.consume(",")) return false;
      }
    }
    return true;
  }

  bool parse_template_member_name(std::string* out_member_name) {
    if (!out_member_name) return false;
    if (!cursor.consume("::")) return false;
    *out_member_name = cursor.parse_identifier();
    return !out_member_name->empty();
  }

  bool parse_pack_sizeof(long long* out_val) {
    if (!cursor.consume("sizeof")) return false;
    cursor.skip_ws();
    if (!cursor.consume("...")) return false;
    if (!cursor.consume("(")) return false;
    const std::string pack_name = cursor.parse_identifier();
    if (pack_name.empty()) return false;
    if (!cursor.consume(")")) return false;
    *out_val = env.count_pack_bindings(pack_name);
    return true;
  }

  bool parse_parenthesized_expr(long long* out_val) {
    if (!cursor.consume("(")) return false;
    if (!parse_or(out_val)) return false;
    return cursor.consume(")");
  }

  bool parse_cast_expr(long long* out_val) {
    size_t saved = cursor.pos;
    std::string ident = cursor.parse_identifier();
    if (ident.empty()) return false;
    cursor.skip_ws();
    if (!cursor.consume("(")) {
      cursor.pos = saved;
      return false;
    }

    TypeSpec cast_ts{};
    cast_ts.array_size = -1;
    cast_ts.inner_rank = -1;
    if (!env.lookup_cast_type(ident, &cast_ts)) {
      cursor.pos = saved;
      return false;
    }

    long long inner = 0;
    if (!parse_or(&inner)) return false;
    if (!cursor.consume(")")) return false;
    *out_val = apply_integral_cast(cast_ts, inner);
    return true;
  }

  bool parse_parenthesized_or_cast(long long* out_val) {
    if (parse_parenthesized_expr(out_val)) return true;
    return parse_cast_expr(out_val);
  }

  bool parse_bound_identifier(long long* out_val) {
    size_t saved = cursor.pos;
    std::string ident = cursor.parse_identifier();
    if (ident.empty()) return false;
    if (env.lookup_bound_value(ident, out_val)) return true;
    cursor.pos = saved;
    return false;
  }

  bool parse_primary(long long* out_val) {
    cursor.skip_ws();
    if (parse_pack_sizeof(out_val)) return true;
    if (parse_parenthesized_or_cast(out_val)) return true;
    if (parse_numeric_literal(out_val)) return true;
    if (parse_bound_identifier(out_val)) return true;
    return eval_member_lookup(out_val);
  }

  bool parse_numeric_literal(long long* out_val) {
    return cursor.parse_number(out_val);
  }

  bool parse_unary(long long* out_val) {
    cursor.skip_ws();
    if (cursor.consume("!")) {
      long long inner = 0;
      if (!parse_unary(&inner)) return false;
      *out_val = inner ? 0 : 1;
      return true;
    }
    if (cursor.consume("-")) {
      long long inner = 0;
      if (!parse_unary(&inner)) return false;
      *out_val = -inner;
      return true;
    }
    if (cursor.consume("+")) return parse_unary(out_val);
    return parse_primary(out_val);
  }

  bool parse_mul(long long* out_val) {
    return parse_left_associative(out_val, &DeferredNttpExprParser::parse_unary,
                                  {{"*", &DeferredNttpExprParser::apply_mul_op},
                                   {"/", &DeferredNttpExprParser::apply_div_op}});
  }

  bool parse_add(long long* out_val) {
    return parse_left_associative(out_val, &DeferredNttpExprParser::parse_mul,
                                  {{"+", &DeferredNttpExprParser::apply_add_op},
                                   {"-", &DeferredNttpExprParser::apply_sub_op}});
  }

  bool parse_rel(long long* out_val) {
    return parse_left_associative(out_val, &DeferredNttpExprParser::parse_add,
                                  {{"<=", &DeferredNttpExprParser::apply_le_op},
                                   {">=", &DeferredNttpExprParser::apply_ge_op},
                                   {"<", &DeferredNttpExprParser::apply_lt_op},
                                   {">", &DeferredNttpExprParser::apply_gt_op}});
  }

  bool parse_eq(long long* out_val) {
    return parse_left_associative(out_val, &DeferredNttpExprParser::parse_rel,
                                  {{"==", &DeferredNttpExprParser::apply_eq_op},
                                   {"!=", &DeferredNttpExprParser::apply_ne_op}});
  }

  bool parse_and(long long* out_val) {
    return parse_left_associative(out_val, &DeferredNttpExprParser::parse_eq,
                                  {{"&&", &DeferredNttpExprParser::apply_and_op}});
  }

  bool parse_or(long long* out_val) {
    return parse_left_associative(out_val, &DeferredNttpExprParser::parse_and,
                                  {{"||", &DeferredNttpExprParser::apply_or_op}});
  }
};

bool hir_is_type_template_param(const Node* tpl_def, const char* name) {
  if (!tpl_def || !name) return false;
  for (int i = 0; i < tpl_def->n_template_params; ++i) {
    if (!tpl_def->template_param_is_nttp[i] &&
        tpl_def->template_param_names[i] &&
        std::strcmp(tpl_def->template_param_names[i], name) == 0)
      return true;
  }
  return false;
}

TypeSpec hir_strip_pattern_qualifiers(TypeSpec ts, const TypeSpec& pattern) {
  if (pattern.is_const) ts.is_const = false;
  if (pattern.is_volatile) ts.is_volatile = false;
  ts.ptr_level -= pattern.ptr_level;
  if (ts.ptr_level < 0) ts.ptr_level = 0;
  if (pattern.is_lvalue_ref) ts.is_lvalue_ref = false;
  if (pattern.is_rvalue_ref) ts.is_rvalue_ref = false;
  if (pattern.array_rank > 0) {
    ts.array_rank -= pattern.array_rank;
    if (ts.array_rank < 0) ts.array_rank = 0;
    ts.array_size = ts.array_rank > 0 ? ts.array_dims[0] : -1;
  }
  return ts;
}

bool hir_match_type_pattern(
    const TypeSpec& pattern_raw, const TypeSpec& actual_raw,
    const Node* tpl_def,
    std::unordered_map<std::string, TypeSpec>* type_bindings) {
  static const std::unordered_map<std::string, TypeSpec> kEmptyTypedefs;
  TypeSpec pattern = resolve_typedef_chain(pattern_raw, kEmptyTypedefs);
  TypeSpec actual = resolve_typedef_chain(actual_raw, kEmptyTypedefs);
  if (pattern.is_const && !actual.is_const) return false;
  if (pattern.is_volatile && !actual.is_volatile) return false;
  if (pattern.ptr_level > actual.ptr_level) return false;
  if (pattern.is_lvalue_ref && !actual.is_lvalue_ref) return false;
  if (pattern.is_rvalue_ref && !actual.is_rvalue_ref) return false;
  if (pattern.array_rank > actual.array_rank) return false;

  if (pattern.base == TB_TYPEDEF && pattern.tag &&
      hir_is_type_template_param(tpl_def, pattern.tag)) {
    TypeSpec bound = hir_strip_pattern_qualifiers(actual, pattern);
    auto it = type_bindings->find(pattern.tag);
    if (it == type_bindings->end()) {
      (*type_bindings)[pattern.tag] = bound;
      return true;
    }
    return types_compatible_p(it->second, bound, kEmptyTypedefs);
  }

  return types_compatible_p(pattern, actual, kEmptyTypedefs);
}

int hir_specialization_match_score(const Node* spec) {
  if (!spec) return -1;
  if (spec->n_template_params == 0) return 100000;
  int score = 0;
  for (int i = 0; i < spec->n_template_args; ++i) {
    if (spec->template_arg_is_value && spec->template_arg_is_value[i]) {
      if (!(spec->template_arg_nttp_names && spec->template_arg_nttp_names[i])) score += 8;
      continue;
    }
    const TypeSpec& ts = spec->template_arg_types[i];
    if (ts.base != TB_TYPEDEF || !spec->template_param_names) score += 8;
    else if (!hir_is_type_template_param(spec, ts.tag)) score += 4;
    if (ts.is_const || ts.is_volatile || ts.ptr_level > 0 ||
        ts.is_lvalue_ref || ts.is_rvalue_ref || ts.array_rank > 0)
      score += 4;
  }
  return score;
}

}  // namespace

bool Lowerer::eval_deferred_nttp_expr_hir(
    const Node* owner_tpl, int param_idx,
    const std::vector<std::pair<std::string, TypeSpec>>& type_bindings_vec,
    const std::vector<std::pair<std::string, long long>>& nttp_bindings_vec,
    const std::string* expr_override,
    long long* out) {
  if (!owner_tpl || param_idx < 0 || param_idx >= owner_tpl->n_template_params) {
    return false;
  }

  std::string expr;
  if (expr_override) {
    expr = *expr_override;
  } else {
    if (!owner_tpl->template_param_default_exprs ||
        !owner_tpl->template_param_default_exprs[param_idx]) {
      return false;
    }
    expr = owner_tpl->template_param_default_exprs[param_idx];
  }
  if (expr.empty()) return false;

  DeferredNttpExprEnv env =
      DeferredNttpExprEnv::from_bindings(type_bindings_vec, nttp_bindings_vec);
  DeferredNttpTemplateLookup template_lookup{template_struct_defs_,
                                             template_struct_specializations_,
                                             struct_def_nodes_};
  DeferredNttpExprParser parser{{expr, 0}, template_lookup, env};
  long long value = 0;
  if (!parser.parse_or(&value)) return false;
  if (!parser.cursor.at_end()) return false;
  *out = value;
  return true;
}

SelectedTemplateStructPattern select_template_struct_pattern_hir(
    const std::vector<HirTemplateArg>& actual_args,
    const TemplateStructEnv& env) {
  SelectedTemplateStructPattern selected;
  selected.primary_def = env.primary_def;
  selected.selected_pattern = env.primary_def;
  int best_score = -1;

  auto try_candidate = [&](const Node* cand) {
    if (!cand) return;
    if (cand->n_template_args != static_cast<int>(actual_args.size())) return;
    std::unordered_map<std::string, TypeSpec> type_bindings_map;
    std::unordered_map<std::string, long long> value_bindings_map;
    for (int i = 0; i < cand->n_template_args; ++i) {
      const HirTemplateArg& actual = actual_args[i];
      const bool pattern_is_value =
          cand->template_arg_is_value && cand->template_arg_is_value[i];
      if (pattern_is_value != actual.is_value) return;
      if (pattern_is_value) {
        const char* pname = cand->template_arg_nttp_names ?
            cand->template_arg_nttp_names[i] : nullptr;
        if (pname && cand->template_param_names) {
          auto it = value_bindings_map.find(pname);
          if (it == value_bindings_map.end()) value_bindings_map[pname] = actual.value;
          else if (it->second != actual.value) return;
        } else {
          if (cand->template_arg_values[i] != actual.value) return;
        }
      } else {
        if (!hir_match_type_pattern(cand->template_arg_types[i], actual.type, cand,
                                    &type_bindings_map))
          return;
      }
    }

    for (int i = 0; i < cand->n_template_params; ++i) {
      const char* pname = cand->template_param_names[i];
      if (!pname) continue;
      const bool has_default =
          cand->template_param_has_default &&
          cand->template_param_has_default[i];
      if (cand->template_param_is_nttp[i]) {
        if (!has_default && value_bindings_map.count(pname) == 0) return;
      } else {
        if (!has_default && type_bindings_map.count(pname) == 0) return;
      }
    }

    int score = hir_specialization_match_score(cand);
    if (score <= best_score) return;
    selected.selected_pattern = cand;
    best_score = score;
    selected.type_bindings.clear();
    selected.nttp_bindings.clear();
    for (int i = 0; i < cand->n_template_params; ++i) {
      const char* pname = cand->template_param_names[i];
      if (!pname) continue;
      if (cand->template_param_is_nttp[i]) {
        auto it = value_bindings_map.find(pname);
        if (it != value_bindings_map.end())
          selected.nttp_bindings[pname] = it->second;
      } else {
        auto it = type_bindings_map.find(pname);
        if (it != type_bindings_map.end())
          selected.type_bindings[pname] = it->second;
      }
    }
  };

  if (env.specialization_patterns) {
    for (const Node* cand : *env.specialization_patterns) try_candidate(cand);
  }

  if (selected.selected_pattern != env.primary_def && selected.selected_pattern)
    return selected;

  selected.type_bindings.clear();
  selected.nttp_bindings.clear();
  if (!env.primary_def) return selected;
  int arg_idx = 0;
  for (; arg_idx < static_cast<int>(actual_args.size()) &&
         arg_idx < env.primary_def->n_template_params; ++arg_idx) {
    const char* param_name = env.primary_def->template_param_names[arg_idx];
    if (env.primary_def->template_param_is_nttp[arg_idx]) {
      if (!actual_args[arg_idx].is_value) {
        selected.selected_pattern = nullptr;
        return selected;
      }
      selected.nttp_bindings[param_name] = actual_args[arg_idx].value;
    } else {
      if (actual_args[arg_idx].is_value) {
        selected.selected_pattern = nullptr;
        return selected;
      }
      selected.type_bindings[param_name] = actual_args[arg_idx].type;
    }
  }
  return selected;
}

const Node* Lowerer::find_template_struct_primary(const std::string& name) const {
  auto it = template_struct_defs_.find(name);
  return it != template_struct_defs_.end() ? it->second : nullptr;
}

const std::vector<const Node*>* Lowerer::find_template_struct_specializations(
    const Node* primary_tpl) const {
  if (!primary_tpl || !primary_tpl->name) return nullptr;
  auto it = template_struct_specializations_.find(primary_tpl->name);
  return it != template_struct_specializations_.end() ? &it->second : nullptr;
}

TemplateStructEnv Lowerer::build_template_struct_env(const Node* primary_tpl) const {
  TemplateStructEnv env;
  env.primary_def = primary_tpl;
  env.specialization_patterns = find_template_struct_specializations(primary_tpl);
  return env;
}

void Lowerer::register_template_struct_primary(
    const std::string& name,
    const Node* node) {
  if (!is_primary_template_struct_def(node)) return;
  template_struct_defs_[name] = node;
  ct_state_->register_template_struct_def(name, node);
}

void Lowerer::register_template_struct_specialization(
    const Node* primary_tpl,
    const Node* node) {
  if (!primary_tpl || !primary_tpl->name || !node) return;
  template_struct_specializations_[primary_tpl->name].push_back(node);
  ct_state_->register_template_struct_specialization(primary_tpl, node);
}

void Lowerer::seed_pending_template_type(const TypeSpec& ts,
                                         const TypeBindings& tpl_bindings,
                                         const NttpBindings& nttp_bindings,
                                         const Node* span_node,
                                         PendingTemplateTypeKind kind,
                                         const std::string& context_name) {
  if (!ts.tpl_struct_origin && !ts.deferred_member_type_name) return;
  const Node* owner_primary_def =
      ts.tpl_struct_origin ? find_template_struct_primary(ts.tpl_struct_origin) : nullptr;
  TypeSpec canonical_ts = ts;
  if (owner_primary_def && owner_primary_def->name && canonical_ts.tpl_struct_origin) {
    canonical_ts.tpl_struct_origin = owner_primary_def->name;
  }
  ct_state_->record_pending_template_type(
      kind, canonical_ts, owner_primary_def, tpl_bindings, nttp_bindings,
      make_span(span_node), context_name);
}

const Node* Lowerer::canonical_template_struct_primary(
    const TypeSpec& ts,
    const Node* primary_tpl) const {
  if (primary_tpl) return primary_tpl;
  if (!ts.tpl_struct_origin) return nullptr;
  return find_template_struct_primary(ts.tpl_struct_origin);
}

void Lowerer::realize_template_struct_if_needed(
    TypeSpec& ts,
    const TypeBindings& tpl_bindings,
    const NttpBindings& nttp_bindings,
    const Node* primary_tpl) {
  if (!ts.tpl_struct_origin) return;
  realize_template_struct(
      ts, canonical_template_struct_primary(ts, primary_tpl),
      tpl_bindings, nttp_bindings);
}

std::string Lowerer::format_deferred_template_type_diagnostic(
    const PendingTemplateTypeWorkItem& work_item,
    const char* prefix,
    const char* detail) const {
  std::string message = prefix;
  if (!work_item.context_name.empty()) {
    message += ": ";
    message += work_item.context_name;
    if (detail && detail[0]) {
      message += " (";
      message += detail;
      message += ")";
    }
    return message;
  }
  if (detail && detail[0]) {
    message += ": ";
    message += detail;
  }
  return message;
}

DeferredTemplateTypeResult Lowerer::blocked_deferred_template_type(
    const PendingTemplateTypeWorkItem& work_item,
    const char* detail,
    bool spawned_new_work) const {
  return DeferredTemplateTypeResult::blocked(
      spawned_new_work,
      format_deferred_template_type_diagnostic(
          work_item, "blocked template type", detail));
}

DeferredTemplateTypeResult Lowerer::terminal_deferred_template_type(
    const PendingTemplateTypeWorkItem& work_item,
    const char* detail) const {
  return DeferredTemplateTypeResult::terminal(
      format_deferred_template_type_diagnostic(
          work_item, "unresolved template type", detail));
}

void Lowerer::realize_template_struct(
    TypeSpec& ts,
    const Node* primary_tpl,
    const TypeBindings& tpl_bindings,
    const NttpBindings& nttp_bindings) {
  if (!primary_tpl && !ts.tpl_struct_origin) return;
  const char* origin = ts.tpl_struct_origin;
  if (!primary_tpl && origin) primary_tpl = find_template_struct_primary(origin);
  if (!primary_tpl) return;
  if (!origin) origin = primary_tpl->name;
  if (primary_tpl->name) ts.tpl_struct_origin = primary_tpl->name;

  ResolvedTemplateArgs resolved =
      materialize_template_args(primary_tpl, ts, tpl_bindings, nttp_bindings);

  for (const auto& arg : resolved.concrete_args) {
    if (!arg.is_value && arg.type.tpl_struct_origin) return;
  }

  TemplateStructEnv tpl_env = build_template_struct_env(primary_tpl);
  SelectedTemplateStructPattern selected_pattern;
  if (template_struct_has_pack_params(primary_tpl)) {
    selected_pattern.primary_def = primary_tpl;
    selected_pattern.selected_pattern = primary_tpl;
    for (const auto& [name, type] : resolved.type_bindings)
      selected_pattern.type_bindings[name] = type;
    for (const auto& [name, value] : resolved.nttp_bindings)
      selected_pattern.nttp_bindings[name] = value;
  } else {
    selected_pattern = select_template_struct_pattern_hir(
        resolved.concrete_args, tpl_env);
  }
  const Node* tpl_def = selected_pattern.selected_pattern;
  if (!tpl_def) tpl_def = primary_tpl;

  PreparedTemplateStructInstance prepared_instance =
      prepare_template_struct_instance(primary_tpl, origin, resolved);

  std::string mangled = build_template_mangled_name(
      primary_tpl, tpl_def, origin, resolved);

  instantiate_template_struct_body(
      mangled, primary_tpl, tpl_def, selected_pattern,
      resolved.concrete_args, prepared_instance.instance_key);

  ts.tag = module_->struct_defs.count(mangled) ?
      module_->struct_defs.at(mangled).tag.c_str() : nullptr;
  while (resolve_struct_member_typedef_if_ready(&ts)) {
  }
  if (!ts.deferred_member_type_name) {
    ts.tpl_struct_origin = nullptr;
    ts.tpl_struct_args.data = nullptr;
    ts.tpl_struct_args.size = 0;
  }
}

}  // namespace c4c::hir
