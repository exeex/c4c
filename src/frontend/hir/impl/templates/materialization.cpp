#include "templates.hpp"

#include <climits>

namespace c4c::hir {

namespace {

std::string encode_template_arg_ref_hir(const TemplateArgRef& arg) {
  if (arg.kind == TemplateArgKind::Value && arg.value == 0 &&
      arg.debug_text && arg.debug_text[0]) {
    return arg.debug_text;
  }
  if (arg.kind == TemplateArgKind::Value) return std::to_string(arg.value);
  if (arg.kind == TemplateArgKind::Type &&
      (has_concrete_type(arg.type) || arg.type.tpl_struct_origin)) {
    return encode_template_type_arg_ref_hir(arg.type);
  }
  if (arg.debug_text && arg.debug_text[0]) return arg.debug_text;
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

struct HirTemplateArgMaterializer {
  const Node* primary_tpl;
  const TypeBindings& tpl_bindings;
  const NttpBindings& nttp_bindings;
  const std::unordered_map<std::string, const Node*>& struct_def_nodes;
  std::function<bool(const Node*, int,
                     const std::vector<std::pair<std::string, TypeSpec>>&,
                     const std::vector<std::pair<std::string, long long>>&,
                     const std::string*, long long*)> eval_deferred_nttp;
  std::function<void(TypeSpec&)> resolve_pending_type;
  std::function<std::string(const std::string&, int)> make_pack_name;
  ResolvedTemplateArgs result;

  std::vector<std::pair<std::string, TypeSpec>> merged_type_bindings() const;
  std::vector<std::pair<std::string, long long>> merged_nttp_bindings() const;
  bool has_type_binding(const char* param_name) const;
  bool has_nttp_binding(const char* param_name) const;
  TypeSpec substitute_bound_type(TypeSpec ts) const;
  bool decode_type_ref(const std::string& ref, TypeSpec* out_type) const;
  bool can_bind_value_param(const std::string& ref) const;
  bool can_bind_type_param(const std::string& ref) const;
  bool resolve_any_arg_ref(const std::string& ref, HirTemplateArg* out_arg);
  bool resolve_explicit_string_arg(int pi, const std::string& ref, HirTemplateArg* out_arg);
  bool resolve_explicit_typed_arg(int pi, const TemplateArgRef& ref, HirTemplateArg* out_arg);
  void append_default_arg(int pi, const char* param_name);
  ResolvedTemplateArgs materialize_from_strings(const std::vector<std::string>& arg_refs);
  ResolvedTemplateArgs materialize_from_typed(const TypeSpec& owner_ts);
};

std::vector<std::pair<std::string, TypeSpec>>
HirTemplateArgMaterializer::merged_type_bindings() const {
  std::vector<std::pair<std::string, TypeSpec>> merged;
  merged.reserve(tpl_bindings.size() + result.type_bindings.size());
  for (const auto& [name, ts] : tpl_bindings) merged.push_back({name, ts});
  for (const auto& [name, ts] : result.type_bindings) {
    bool replaced = false;
    for (auto& [merged_name, merged_ts] : merged) {
      if (merged_name == name) {
        merged_ts = ts;
        replaced = true;
        break;
      }
    }
    if (!replaced) merged.push_back({name, ts});
  }
  return merged;
}

std::vector<std::pair<std::string, long long>>
HirTemplateArgMaterializer::merged_nttp_bindings() const {
  std::vector<std::pair<std::string, long long>> merged;
  merged.reserve(nttp_bindings.size() + result.nttp_bindings.size());
  for (const auto& [name, val] : nttp_bindings) merged.push_back({name, val});
  for (const auto& [name, val] : result.nttp_bindings) {
    bool replaced = false;
    for (auto& [merged_name, merged_val] : merged) {
      if (merged_name == name) {
        merged_val = val;
        replaced = true;
        break;
      }
    }
    if (!replaced) merged.push_back({name, val});
  }
  return merged;
}

bool HirTemplateArgMaterializer::has_type_binding(const char* param_name) const {
  for (const auto& [name, _] : result.type_bindings) {
    if (name == param_name) return true;
  }
  return false;
}

bool HirTemplateArgMaterializer::has_nttp_binding(const char* param_name) const {
  for (const auto& [name, _] : result.nttp_bindings) {
    if (name == param_name) return true;
  }
  return false;
}

TypeSpec HirTemplateArgMaterializer::substitute_bound_type(TypeSpec ts) const {
  if (ts.base == TB_TYPEDEF && ts.tag) {
    const int outer_ptr_level = ts.ptr_level;
    const bool outer_lref = ts.is_lvalue_ref;
    const bool outer_rref = ts.is_rvalue_ref;
    const bool outer_const = ts.is_const;
    const bool outer_volatile = ts.is_volatile;
    auto it = tpl_bindings.find(ts.tag);
    if (it != tpl_bindings.end()) {
      ts = it->second;
      ts.ptr_level += outer_ptr_level;
      ts.is_lvalue_ref = ts.is_lvalue_ref || outer_lref;
      ts.is_rvalue_ref = !ts.is_lvalue_ref && (ts.is_rvalue_ref || outer_rref);
      ts.is_const = ts.is_const || outer_const;
      ts.is_volatile = ts.is_volatile || outer_volatile;
    }
  }
  return ts;
}

bool HirTemplateArgMaterializer::decode_type_ref(const std::string& ref,
                                                 TypeSpec* out_type) const {
  if (!out_type || ref.empty()) return false;
  auto tit = tpl_bindings.find(ref);
  if (tit != tpl_bindings.end()) {
    *out_type = tit->second;
    return true;
  }

  std::string base_ref = ref;
  TypeSpec suffix_ts{};
  suffix_ts.array_size = -1;
  suffix_ts.inner_rank = -1;
  while (base_ref.size() >= 4 &&
         base_ref.compare(base_ref.size() - 4, 4, "_ptr") == 0) {
    suffix_ts.ptr_level++;
    base_ref.resize(base_ref.size() - 4);
  }
  if (base_ref.size() >= 5 &&
      base_ref.compare(base_ref.size() - 5, 5, "_rref") == 0) {
    suffix_ts.is_rvalue_ref = true;
    base_ref.resize(base_ref.size() - 5);
  } else if (base_ref.size() >= 4 &&
             base_ref.compare(base_ref.size() - 4, 4, "_ref") == 0) {
    suffix_ts.is_lvalue_ref = true;
    base_ref.resize(base_ref.size() - 4);
  }
  if (base_ref.size() >= 6 && base_ref.compare(0, 6, "const_") == 0) {
    suffix_ts.is_const = true;
    base_ref.erase(0, 6);
  }
  if (base_ref.size() >= 9 && base_ref.compare(0, 9, "volatile_") == 0) {
    suffix_ts.is_volatile = true;
    base_ref.erase(0, 9);
  }

  TypeSpec resolved{};
  resolved.array_size = -1;
  resolved.inner_rank = -1;
  auto base_it = tpl_bindings.find(base_ref);
  if (base_it != tpl_bindings.end()) {
    resolved = base_it->second;
  } else if (parse_builtin_typespec_text(base_ref, &resolved)) {
  } else if (auto sit = struct_def_nodes.find(base_ref);
             sit != struct_def_nodes.end()) {
    resolved.base = TB_STRUCT;
    resolved.tag = sit->first.c_str();
  } else if (base_ref.size() > 7 &&
             base_ref.compare(0, 7, "struct_") == 0 &&
             struct_def_nodes.count(base_ref.substr(7)) != 0) {
    resolved.base = TB_STRUCT;
    resolved.tag = struct_def_nodes.find(base_ref.substr(7))->first.c_str();
  } else if (base_ref.size() > 6 &&
             base_ref.compare(0, 6, "union_") == 0 &&
             struct_def_nodes.count(base_ref.substr(6)) != 0) {
    resolved.base = TB_UNION;
    resolved.tag = struct_def_nodes.find(base_ref.substr(6))->first.c_str();
  } else if (base_ref.size() > 5 &&
             base_ref.compare(0, 5, "enum_") == 0) {
    resolved.base = TB_ENUM;
    resolved.tag = ::strdup(base_ref.substr(5).c_str());
  } else {
    return false;
  }

  resolved.ptr_level += suffix_ts.ptr_level;
  resolved.is_lvalue_ref = resolved.is_lvalue_ref || suffix_ts.is_lvalue_ref;
  resolved.is_rvalue_ref =
      !resolved.is_lvalue_ref &&
      (resolved.is_rvalue_ref || suffix_ts.is_rvalue_ref);
  resolved.is_const = resolved.is_const || suffix_ts.is_const;
  resolved.is_volatile = resolved.is_volatile || suffix_ts.is_volatile;
  *out_type = resolved;
  return true;
}

bool HirTemplateArgMaterializer::can_bind_value_param(const std::string& ref) const {
  if (is_deferred_nttp_expr_ref(ref)) return true;
  if (nttp_bindings.count(ref)) return true;
  if (ref == "true" || ref == "false") return true;
  try {
    (void)std::stoll(ref);
    return true;
  } catch (...) {
    return false;
  }
}

bool HirTemplateArgMaterializer::can_bind_type_param(const std::string& ref) const {
  if (ref.size() > 1 && ref[0] == '@') return true;
  TypeSpec decoded{};
  return decode_type_ref(ref, &decoded);
}

bool HirTemplateArgMaterializer::resolve_any_arg_ref(const std::string& ref,
                                                     HirTemplateArg* out_arg) {
  if (!out_arg) return false;
  auto nit = nttp_bindings.find(ref);
  if (nit != nttp_bindings.end()) {
    out_arg->is_value = true;
    out_arg->value = nit->second;
    return true;
  }
  if (ref == "true" || ref == "false") {
    out_arg->is_value = true;
    out_arg->value = (ref == "true") ? 1 : 0;
    return true;
  }
  try {
    out_arg->is_value = true;
    out_arg->value = std::stoll(ref);
    return true;
  } catch (...) {
  }

  if (ref.size() > 1 && ref[0] == '@') {
    const size_t colon = ref.find(':', 1);
    if (colon == std::string::npos) return false;
    const std::string inner_origin = ref.substr(1, colon - 1);
    const size_t member_sep = ref.find('$', colon + 1);
    const std::string inner_args =
        member_sep == std::string::npos
            ? ref.substr(colon + 1)
            : ref.substr(colon + 1, member_sep - (colon + 1));
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
        if (!resolve_any_arg_ref(part, &nested_arg)) return false;
        nested_args.push_back(nested_arg);
      }
      assign_template_arg_refs_from_hir_args(&nested_ts, nested_args);
    }
    if (member_sep != std::string::npos && member_sep + 1 < ref.size()) {
      nested_ts.deferred_member_type_name =
          ::strdup(ref.substr(member_sep + 1).c_str());
    }
    resolve_pending_type(nested_ts);
    out_arg->is_value = false;
    out_arg->type = nested_ts;
    return true;
  }

  auto tit = tpl_bindings.find(ref);
  if (tit != tpl_bindings.end()) {
    out_arg->is_value = false;
    out_arg->type = tit->second;
    return true;
  }
  TypeSpec decoded{};
  if (decode_type_ref(ref, &decoded)) {
    out_arg->is_value = false;
    out_arg->type = decoded;
    return true;
  }
  return false;
}

bool HirTemplateArgMaterializer::resolve_explicit_string_arg(
    int pi, const std::string& ref, HirTemplateArg* out_arg) {
  if (!out_arg) return false;
  out_arg->is_value =
      primary_tpl->template_param_is_nttp &&
      primary_tpl->template_param_is_nttp[pi];
  if (out_arg->is_value) {
    if (is_deferred_nttp_expr_ref(ref)) {
      long long eval_val = 0;
      std::string expr = deferred_nttp_expr_text(ref);
      const auto type_env = merged_type_bindings();
      const auto nttp_env = merged_nttp_bindings();
      if (!eval_deferred_nttp(
              primary_tpl, pi, type_env, nttp_env, &expr, &eval_val)) {
        return false;
      }
      out_arg->value = eval_val;
      return true;
    }
    auto nit = nttp_bindings.find(ref);
    if (nit != nttp_bindings.end()) {
      out_arg->value = nit->second;
      return true;
    }
    if (ref == "true" || ref == "false") {
      out_arg->value = (ref == "true") ? 1 : 0;
      return true;
    }
    try {
      out_arg->value = std::stoll(ref);
      return true;
    } catch (...) {
      return false;
    }
  }

  HirTemplateArg resolved{};
  if (!resolve_any_arg_ref(ref, &resolved) || resolved.is_value) return false;
  out_arg->type = resolved.type;
  return true;
}

bool HirTemplateArgMaterializer::resolve_explicit_typed_arg(
    int pi, const TemplateArgRef& ref, HirTemplateArg* out_arg) {
  if (!out_arg) return false;
  const bool is_nttp =
      primary_tpl->template_param_is_nttp &&
      primary_tpl->template_param_is_nttp[pi];
  out_arg->is_value = is_nttp;
  if (is_nttp) {
    if (ref.kind != TemplateArgKind::Value) return false;
    const std::string debug_text =
        ref.debug_text ? std::string(ref.debug_text) : std::string{};
    if (!debug_text.empty() && is_deferred_nttp_expr_ref(debug_text)) {
      long long eval_val = 0;
      std::string expr = deferred_nttp_expr_text(debug_text);
      const auto type_env = merged_type_bindings();
      const auto nttp_env = merged_nttp_bindings();
      if (!eval_deferred_nttp(
              primary_tpl, pi, type_env, nttp_env, &expr, &eval_val)) {
        return false;
      }
      out_arg->value = eval_val;
      return true;
    }
    if (ref.value != 0 || debug_text.empty()) {
      out_arg->value = ref.value;
      return true;
    }
    if (!debug_text.empty()) {
      auto nit = nttp_bindings.find(debug_text);
      if (nit != nttp_bindings.end()) {
        out_arg->value = nit->second;
        return true;
      }
      if (debug_text == "true" || debug_text == "false") {
        out_arg->value = (debug_text == "true") ? 1 : 0;
        return true;
      }
      try {
        out_arg->value = std::stoll(debug_text);
        return true;
      } catch (...) {
      }
    }
    out_arg->value = ref.value;
    return true;
  }

  if (ref.kind == TemplateArgKind::Type &&
      (has_concrete_type(ref.type) || ref.type.tpl_struct_origin)) {
    out_arg->type = substitute_bound_type(ref.type);
    return true;
  }
  return false;
}

void HirTemplateArgMaterializer::append_default_arg(int pi, const char* param_name) {
  const bool is_nttp =
      primary_tpl->template_param_is_nttp &&
      primary_tpl->template_param_is_nttp[pi];
  HirTemplateArg arg;
  arg.is_value = is_nttp;
  if (arg.is_value) {
    if (primary_tpl->template_param_default_values[pi] != LLONG_MIN) {
      arg.value = primary_tpl->template_param_default_values[pi];
    } else {
      long long eval_val = 0;
      const auto type_env = merged_type_bindings();
      const auto nttp_env = merged_nttp_bindings();
      if (!eval_deferred_nttp(
              primary_tpl, pi, type_env, nttp_env, nullptr, &eval_val)) {
        return;
      }
      arg.value = eval_val;
    }
    if (!has_nttp_binding(param_name))
      result.nttp_bindings.push_back({param_name, arg.value});
  } else {
    arg.type = primary_tpl->template_param_default_types[pi];
    if (!has_type_binding(param_name))
      result.type_bindings.push_back({param_name, arg.type});
  }
  result.concrete_args.push_back(arg);
}

ResolvedTemplateArgs HirTemplateArgMaterializer::materialize_from_strings(
    const std::vector<std::string>& arg_refs) {
  auto count_explicit_suffix_args = [&](int start_param_idx, int start_arg_idx) {
    int suffix_matches = 0;
    for (int pi = primary_tpl->n_template_params - 1;
         pi >= start_param_idx &&
         static_cast<int>(arg_refs.size()) - 1 - suffix_matches >= start_arg_idx; --pi) {
      if (primary_tpl->template_param_is_pack &&
          primary_tpl->template_param_is_pack[pi]) {
        break;
      }
      const std::string& ref = arg_refs[arg_refs.size() - 1 - suffix_matches];
      const bool wants_value =
          primary_tpl->template_param_is_nttp &&
          primary_tpl->template_param_is_nttp[pi];
      const bool matches_kind =
          wants_value ? can_bind_value_param(ref) : can_bind_type_param(ref);
      if (!matches_kind) {
        if (primary_tpl->template_param_has_default &&
            primary_tpl->template_param_has_default[pi]) {
          continue;
        }
        break;
      }
      ++suffix_matches;
    }
    return suffix_matches;
  };

  int ai = 0;
  for (int pi = 0; pi < primary_tpl->n_template_params; ++pi) {
    const char* param_name = primary_tpl->template_param_names[pi];
    if (!param_name) continue;
    const bool is_nttp =
        primary_tpl->template_param_is_nttp &&
        primary_tpl->template_param_is_nttp[pi];
    const bool is_pack =
        primary_tpl->template_param_is_pack &&
        primary_tpl->template_param_is_pack[pi];

    if (is_pack) {
      const int suffix_explicit_args = count_explicit_suffix_args(pi + 1, ai);
      const int pack_arg_end =
          std::max(ai, static_cast<int>(arg_refs.size()) - suffix_explicit_args);
      for (int pack_index = 0; ai < pack_arg_end; ++pack_index, ++ai) {
        HirTemplateArg arg;
        if (!resolve_explicit_string_arg(pi, arg_refs[ai], &arg)) break;
        result.concrete_args.push_back(arg);
        const std::string packed_name = make_pack_name(param_name, pack_index);
        if (arg.is_value) result.nttp_bindings.push_back({packed_name, arg.value});
        else result.type_bindings.push_back({packed_name, arg.type});
      }
      continue;
    }

    if (ai < static_cast<int>(arg_refs.size())) {
      const bool matches_kind =
          is_nttp ? can_bind_value_param(arg_refs[ai])
                  : can_bind_type_param(arg_refs[ai]);
      if (matches_kind) {
        HirTemplateArg arg;
        if (resolve_explicit_string_arg(pi, arg_refs[ai], &arg)) {
          ++ai;
          result.concrete_args.push_back(arg);
          if (arg.is_value) result.nttp_bindings.push_back({param_name, arg.value});
          else result.type_bindings.push_back({param_name, arg.type});
          continue;
        }
      }
    }

    if (!primary_tpl->template_param_has_default ||
        !primary_tpl->template_param_has_default[pi]) {
      continue;
    }
    append_default_arg(pi, param_name);
  }
  return result;
}

ResolvedTemplateArgs HirTemplateArgMaterializer::materialize_from_typed(
    const TypeSpec& owner_ts) {
  auto fallback_refs = [&]() { return encode_template_arg_ref_list_hir(owner_ts); };
  if (!owner_ts.tpl_struct_args.data || owner_ts.tpl_struct_args.size <= 0) {
    return materialize_from_strings({});
  }

  int ai = 0;
  for (int pi = 0; pi < primary_tpl->n_template_params; ++pi) {
    const char* param_name = primary_tpl->template_param_names[pi];
    if (!param_name) continue;
    const bool is_pack =
        primary_tpl->template_param_is_pack &&
        primary_tpl->template_param_is_pack[pi];

    if (is_pack) {
      int pack_index = 0;
      while (ai < owner_ts.tpl_struct_args.size) {
        HirTemplateArg arg{};
        if (!resolve_explicit_typed_arg(pi, owner_ts.tpl_struct_args.data[ai], &arg)) {
          break;
        }
        ++ai;
        result.concrete_args.push_back(arg);
        const std::string packed_name = make_pack_name(param_name, pack_index++);
        if (arg.is_value) result.nttp_bindings.push_back({packed_name, arg.value});
        else result.type_bindings.push_back({packed_name, arg.type});
      }
      continue;
    }

    if (ai < owner_ts.tpl_struct_args.size) {
      HirTemplateArg arg{};
      if (resolve_explicit_typed_arg(pi, owner_ts.tpl_struct_args.data[ai], &arg)) {
        ++ai;
        result.concrete_args.push_back(arg);
        if (arg.is_value) result.nttp_bindings.push_back({param_name, arg.value});
        else result.type_bindings.push_back({param_name, arg.type});
        continue;
      }
      return materialize_from_strings(fallback_refs());
    }

    if (!primary_tpl->template_param_has_default ||
        !primary_tpl->template_param_has_default[pi]) {
      continue;
    }
    append_default_arg(pi, param_name);
  }
  return result;
}

}  // namespace

ResolvedTemplateArgs Lowerer::materialize_template_args(
    const Node* primary_tpl,
    const TypeSpec& owner_ts,
    const TypeBindings& tpl_bindings,
    const NttpBindings& nttp_bindings) {
  HirTemplateArgMaterializer materializer{
      primary_tpl,
      tpl_bindings,
      nttp_bindings,
      struct_def_nodes_,
      [this](const Node* owner_tpl, int param_idx,
             const std::vector<std::pair<std::string, TypeSpec>>& type_env,
             const std::vector<std::pair<std::string, long long>>& nttp_env,
             const std::string* expr_override,
             long long* out) {
        return eval_deferred_nttp_expr_hir(
            owner_tpl, param_idx, type_env, nttp_env, expr_override, out);
      },
      [this, &tpl_bindings, &nttp_bindings](TypeSpec& ts) {
        seed_and_resolve_pending_template_type_if_needed(
            ts, tpl_bindings, nttp_bindings, nullptr,
            PendingTemplateTypeKind::OwnerStruct, "nested-tpl-arg");
      },
      [this](const std::string& base, int index) {
        return pack_binding_name(base, index);
      }};
  return materializer.materialize_from_typed(owner_ts);
}

PreparedTemplateStructInstance Lowerer::prepare_template_struct_instance(
    const Node* primary_tpl,
    const char* origin,
    const ResolvedTemplateArgs& resolved) {
  PreparedTemplateStructInstance prepared;
  std::vector<std::string> primary_param_order;
  primary_param_order.reserve(primary_tpl->n_template_params);
  for (int pi = 0; pi < primary_tpl->n_template_params; ++pi) {
    const char* param_name = primary_tpl->template_param_names[pi];
    if (!param_name) continue;
    const bool is_pack =
        primary_tpl->template_param_is_pack &&
        primary_tpl->template_param_is_pack[pi];
    const bool is_nttp =
        primary_tpl->template_param_is_nttp &&
        primary_tpl->template_param_is_nttp[pi];
    if (is_pack) {
      std::vector<std::pair<int, std::string>> pack_names;
      if (is_nttp) {
        for (const auto& [name, _] : resolved.nttp_bindings) {
          int pack_index = 0;
          if (parse_pack_binding_name(name, param_name, &pack_index))
            pack_names.push_back({pack_index, name});
        }
      } else {
        for (const auto& [name, _] : resolved.type_bindings) {
          int pack_index = 0;
          if (parse_pack_binding_name(name, param_name, &pack_index))
            pack_names.push_back({pack_index, name});
        }
      }
      std::sort(pack_names.begin(), pack_names.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
      for (const auto& [_, pack_name] : pack_names) {
        primary_param_order.push_back(pack_name);
        if (is_nttp) {
          for (const auto& [name, value] : resolved.nttp_bindings) {
            if (name == pack_name) prepared.nttp_bindings[name] = value;
          }
        } else {
          for (const auto& [name, type] : resolved.type_bindings) {
            if (name == pack_name) prepared.type_bindings[name] = type;
          }
        }
      }
      continue;
    }
    primary_param_order.push_back(param_name);
    if (is_nttp) {
      for (const auto& [name, value] : resolved.nttp_bindings) {
        if (name == param_name) prepared.nttp_bindings[name] = value;
      }
    } else {
      for (const auto& [name, type] : resolved.type_bindings) {
        if (name == param_name) prepared.type_bindings[name] = type;
      }
    }
  }

  const std::string primary_name =
      primary_tpl->name ? primary_tpl->name
                        : std::string(origin ? origin : "");
  SpecializationKey instance_spec_key = prepared.nttp_bindings.empty()
      ? make_specialization_key(primary_name, primary_param_order,
                                prepared.type_bindings)
      : make_specialization_key(primary_name, primary_param_order,
                                prepared.type_bindings,
                                prepared.nttp_bindings);
  prepared.instance_key = TemplateStructInstanceKey{primary_tpl, instance_spec_key};
  return prepared;
}

std::string Lowerer::build_template_mangled_name(
    const Node* primary_tpl,
    const Node* tpl_def,
    const char* origin,
    const ResolvedTemplateArgs& resolved) {
  const std::string family_name =
      tpl_def->template_origin_name ? tpl_def->template_origin_name : std::string(origin);
  std::string mangled(family_name);
  auto append_type_suffix = [&](const TypeSpec& pts) {
    if (pts.is_const) mangled += "const_";
    if (pts.is_volatile) mangled += "volatile_";
    switch (pts.base) {
      case TB_INT: mangled += "int"; break;
      case TB_UINT: mangled += "uint"; break;
      case TB_CHAR: mangled += "char"; break;
      case TB_SCHAR: mangled += "schar"; break;
      case TB_UCHAR: mangled += "uchar"; break;
      case TB_SHORT: mangled += "short"; break;
      case TB_USHORT: mangled += "ushort"; break;
      case TB_LONG: mangled += "long"; break;
      case TB_ULONG: mangled += "ulong"; break;
      case TB_LONGLONG: mangled += "llong"; break;
      case TB_ULONGLONG: mangled += "ullong"; break;
      case TB_FLOAT: mangled += "float"; break;
      case TB_DOUBLE: mangled += "double"; break;
      case TB_LONGDOUBLE: mangled += "ldouble"; break;
      case TB_VOID: mangled += "void"; break;
      case TB_BOOL: mangled += "bool"; break;
      case TB_INT128: mangled += "i128"; break;
      case TB_UINT128: mangled += "u128"; break;
      case TB_STRUCT:
        mangled += "struct_";
        mangled += pts.tag ? pts.tag : "anon";
        break;
      case TB_UNION:
        mangled += "union_";
        mangled += pts.tag ? pts.tag : "anon";
        break;
      case TB_ENUM:
        mangled += "enum_";
        mangled += pts.tag ? pts.tag : "anon";
        break;
      case TB_TYPEDEF:
        mangled += pts.tag ? pts.tag : "typedef";
        break;
      default: mangled += "T"; break;
    }
    for (int p = 0; p < pts.ptr_level; ++p) mangled += "_ptr";
    if (pts.is_lvalue_ref) mangled += "_ref";
    if (pts.is_rvalue_ref) mangled += "_rref";
  };
  auto append_pack_entries = [&](const char* param_name, bool is_value_pack) {
    bool wrote_any = false;
    if (is_value_pack) {
      std::vector<std::pair<int, long long>> values;
      for (const auto& [name, value] : resolved.nttp_bindings) {
        int pack_index = 0;
        if (parse_pack_binding_name(name, param_name, &pack_index))
          values.push_back({pack_index, value});
      }
      std::sort(values.begin(), values.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
      for (const auto& [idx, value] : values) {
        if (idx > 0) mangled += "_";
        mangled += std::to_string(value);
        wrote_any = true;
      }
    } else {
      std::vector<std::pair<int, TypeSpec>> types;
      for (const auto& [name, type] : resolved.type_bindings) {
        int pack_index = 0;
        if (parse_pack_binding_name(name, param_name, &pack_index))
          types.push_back({pack_index, type});
      }
      std::sort(types.begin(), types.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
      for (const auto& [idx, type] : types) {
        if (idx > 0) mangled += "_";
        append_type_suffix(type);
        wrote_any = true;
      }
    }
    if (!wrote_any) mangled += is_value_pack ? "0" : "T";
  };
  for (int pi = 0; pi < primary_tpl->n_template_params; ++pi) {
    mangled += "_";
    mangled += primary_tpl->template_param_names[pi];
    mangled += "_";
    const bool is_pack =
        primary_tpl->template_param_is_pack &&
        primary_tpl->template_param_is_pack[pi];
    const bool is_nttp =
        primary_tpl->template_param_is_nttp &&
        primary_tpl->template_param_is_nttp[pi];
    if (is_pack) {
      append_pack_entries(primary_tpl->template_param_names[pi], is_nttp);
    } else if (is_nttp) {
      bool wrote_value = false;
      for (const auto& [name, value] : resolved.nttp_bindings) {
        if (name == primary_tpl->template_param_names[pi]) {
          mangled += std::to_string(value);
          wrote_value = true;
          break;
        }
      }
      if (!wrote_value) mangled += "0";
    } else {
      bool wrote_type = false;
      for (const auto& [name, type] : resolved.type_bindings) {
        if (name == primary_tpl->template_param_names[pi]) {
          append_type_suffix(type);
          wrote_type = true;
          break;
        }
      }
      if (!wrote_type) mangled += "T";
    }
  }
  return mangled;
}

}  // namespace c4c::hir
