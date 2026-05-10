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

bool qualified_name_key_has_base(const QualifiedNameKey& key) {
  return key.base_text_id != kInvalidText;
}

bool record_matches_template_origin_key(const Node* record,
                                        const QualifiedNameKey& key) {
  if (!record || !qualified_name_key_has_base(key)) return false;
  if (record->unqualified_text_id != key.base_text_id) return false;
  return key.context_id < 0 || record->namespace_context_id < 0 ||
         record->namespace_context_id == key.context_id;
}

const char* record_template_origin_name_for_deduction(const Node* record) {
  if (!record) return nullptr;
  if (record->template_origin_name && record->template_origin_name[0]) {
    return record->template_origin_name;
  }
  if (record->n_template_params > 0) {
    if (record->unqualified_name && record->unqualified_name[0]) {
      return record->unqualified_name;
    }
    return record->name;
  }
  return nullptr;
}

const char* typespec_template_origin_name_for_deduction(const TypeSpec& ts) {
  if (ts.tpl_struct_origin && ts.tpl_struct_origin[0]) return ts.tpl_struct_origin;
  return record_template_origin_name_for_deduction(ts.record_def);
}

bool template_origin_identity_match_for_deduction(const TypeSpec& param_ts,
                                                  const TypeSpec& arg_ts) {
  const bool param_has_key =
      qualified_name_key_has_base(param_ts.tpl_struct_origin_key);
  const bool arg_has_key =
      qualified_name_key_has_base(arg_ts.tpl_struct_origin_key);
  if (param_has_key || arg_has_key) {
    if (param_has_key && arg_has_key) {
      return param_ts.tpl_struct_origin_key == arg_ts.tpl_struct_origin_key;
    }
    if (param_has_key) {
      return record_matches_template_origin_key(
          arg_ts.record_def, param_ts.tpl_struct_origin_key);
    }
    return record_matches_template_origin_key(
        param_ts.record_def, arg_ts.tpl_struct_origin_key);
  }

  const char* param_origin =
      typespec_template_origin_name_for_deduction(param_ts);
  const char* arg_origin = typespec_template_origin_name_for_deduction(arg_ts);
  return param_origin && arg_origin && std::strcmp(param_origin, arg_origin) == 0;
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

int template_param_index_for_type_deduction(const TypeSpec& ts,
                                            const Node* fn_def) {
  if (ts.base != TB_TYPEDEF || !fn_def || fn_def->n_template_params <= 0 ||
      !fn_def->template_param_names) {
    return -1;
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
    if (owner_matches) return ts.template_param_index;
  }

  const TextId carrier_text_id = template_param_carrier_text_id_for_deduction(ts);
  if (carrier_text_id == kInvalidText || !fn_def->template_param_name_text_ids) {
    return -1;
  }
  for (int i = 0; i < fn_def->n_template_params; ++i) {
    if (!fn_def->template_param_names[i]) continue;
    if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) {
      continue;
    }
    if (fn_def->template_param_name_text_ids[i] == carrier_text_id) return i;
  }
  return -1;
}

int template_param_index_for_nttp_deduction(const TemplateArgRef& arg,
                                            const Node* fn_def) {
  if (!fn_def || fn_def->n_template_params <= 0 ||
      !fn_def->template_param_names || !fn_def->template_param_name_text_ids ||
      arg.nttp_text_id == kInvalidText) {
    return -1;
  }
  for (int i = 0; i < fn_def->n_template_params; ++i) {
    if (!fn_def->template_param_names[i]) continue;
    if (!fn_def->template_param_is_nttp || !fn_def->template_param_is_nttp[i]) {
      continue;
    }
    if (fn_def->template_param_name_text_ids[i] == arg.nttp_text_id) return i;
  }
  return -1;
}

bool bind_deduced_type(TypeBindings& deduced,
                       const std::string& key,
                       TypeSpec value) {
  value.is_lvalue_ref = false;
  value.is_rvalue_ref = false;
  auto it = deduced.find(key);
  if (it == deduced.end()) {
    deduced[key] = value;
    return true;
  }
  TypeSpec existing = it->second;
  existing.is_lvalue_ref = false;
  existing.is_rvalue_ref = false;
  return existing.base == value.base &&
         existing.ptr_level == value.ptr_level &&
         typespec_nominal_match_for_deduction(existing, value);
}

bool bind_deduced_nttp(NttpBindings& deduced,
                       const std::string& key,
                       long long value) {
  auto it = deduced.find(key);
  if (it == deduced.end()) {
    deduced[key] = value;
    return true;
  }
  return it->second == value;
}

std::string pack_binding_key(const char* base, int index) {
  return std::string(base ? base : "") + "#" + std::to_string(index);
}

TemplateArgRefList template_arg_list_for_deduction(
    const TypeSpec& ts,
    std::vector<TemplateArgRef>& scratch) {
  if (ts.tpl_struct_args.data && ts.tpl_struct_args.size > 0) {
    return ts.tpl_struct_args;
  }
  if (!ts.record_def || ts.record_def->n_template_args <= 0) return {};
  scratch.reserve(ts.record_def->n_template_args);
  for (int i = 0; i < ts.record_def->n_template_args; ++i) {
    TemplateArgRef ref{};
    const bool is_value =
        ts.record_def->template_arg_is_value &&
        ts.record_def->template_arg_is_value[i];
    ref.kind = is_value ? TemplateArgKind::Value : TemplateArgKind::Type;
    if (is_value) {
      ref.value =
          ts.record_def->template_arg_values ? ts.record_def->template_arg_values[i] : 0;
      ref.nttp_text_id =
          ts.record_def->template_arg_nttp_text_ids
              ? ts.record_def->template_arg_nttp_text_ids[i]
              : kInvalidText;
      ref.debug_text =
          ts.record_def->template_arg_nttp_names
              ? ts.record_def->template_arg_nttp_names[i]
              : nullptr;
    } else if (ts.record_def->template_arg_types) {
      ref.type = ts.record_def->template_arg_types[i];
    }
    scratch.push_back(ref);
  }
  return TemplateArgRefList{scratch.data(), static_cast<int>(scratch.size())};
}

bool deduce_type_pattern_from_type(TypeSpec param_ts,
                                   TypeSpec arg_ts,
                                   const Node* fn_def,
                                   TypeBindings& deduced_types,
                                   NttpBindings& deduced_nttp);

bool deduce_template_arg_pattern(const TemplateArgRef& param_arg,
                                 const TemplateArgRef& arg,
                                 const Node* fn_def,
                                 TypeBindings& deduced_types,
                                 NttpBindings& deduced_nttp) {
  if (param_arg.kind == TemplateArgKind::Type) {
    if (arg.kind != TemplateArgKind::Type) return false;
    return deduce_type_pattern_from_type(
        param_arg.type, arg.type, fn_def, deduced_types, deduced_nttp);
  }
  if (param_arg.kind != TemplateArgKind::Value || arg.kind != TemplateArgKind::Value) {
    return false;
  }
  const int nttp_index = template_param_index_for_nttp_deduction(param_arg, fn_def);
  if (nttp_index < 0) return param_arg.value == arg.value;
  const char* pname = fn_def->template_param_names[nttp_index];
  if (!pname) return false;
  const bool is_pack =
      fn_def->template_param_is_pack && fn_def->template_param_is_pack[nttp_index];
  return bind_deduced_nttp(
      deduced_nttp, is_pack ? pack_binding_key(pname, 0) : std::string(pname),
      arg.value);
}

bool deduce_template_arg_list(const TemplateArgRefList& param_args,
                              const TemplateArgRefList& arg_args,
                              const Node* fn_def,
                              TypeBindings& deduced_types,
                              NttpBindings& deduced_nttp) {
  if (!param_args.data || param_args.size <= 0) {
    return !arg_args.data || arg_args.size <= 0;
  }
  if (!arg_args.data) return false;

  int ai = 0;
  for (int pi = 0; pi < param_args.size; ++pi) {
    const TemplateArgRef& param_arg = param_args.data[pi];
    if (param_arg.kind == TemplateArgKind::Type) {
      const int type_index =
          template_param_index_for_type_deduction(param_arg.type, fn_def);
      const bool is_pack =
          type_index >= 0 && fn_def->template_param_is_pack &&
          fn_def->template_param_is_pack[type_index];
      if (is_pack) {
        const int remaining_patterns = param_args.size - pi - 1;
        const int pack_count = arg_args.size - ai - remaining_patterns;
        if (pack_count < 0) return false;
        const char* pname = fn_def->template_param_names[type_index];
        for (int pack_index = 0; pack_index < pack_count; ++pack_index, ++ai) {
          if (arg_args.data[ai].kind != TemplateArgKind::Type) return false;
          if (!bind_deduced_type(
                  deduced_types, pack_binding_key(pname, pack_index),
                  arg_args.data[ai].type)) {
            return false;
          }
        }
        continue;
      }
    } else if (param_arg.kind == TemplateArgKind::Value) {
      const int nttp_index =
          template_param_index_for_nttp_deduction(param_arg, fn_def);
      const bool is_pack =
          nttp_index >= 0 && fn_def->template_param_is_pack &&
          fn_def->template_param_is_pack[nttp_index];
      if (is_pack) {
        const int remaining_patterns = param_args.size - pi - 1;
        const int pack_count = arg_args.size - ai - remaining_patterns;
        if (pack_count < 0) return false;
        const char* pname = fn_def->template_param_names[nttp_index];
        for (int pack_index = 0; pack_index < pack_count; ++pack_index, ++ai) {
          if (arg_args.data[ai].kind != TemplateArgKind::Value) return false;
          if (!bind_deduced_nttp(
                  deduced_nttp, pack_binding_key(pname, pack_index),
                  arg_args.data[ai].value)) {
            return false;
          }
        }
        continue;
      }
    }

    if (ai >= arg_args.size) return false;
    if (!deduce_template_arg_pattern(
            param_arg, arg_args.data[ai], fn_def, deduced_types, deduced_nttp)) {
      return false;
    }
    ++ai;
  }
  return ai == arg_args.size;
}

bool deduce_type_pattern_from_type(TypeSpec param_ts,
                                   TypeSpec arg_ts,
                                   const Node* fn_def,
                                   TypeBindings& deduced_types,
                                   NttpBindings& deduced_nttp) {
  param_ts.is_lvalue_ref = false;
  param_ts.is_rvalue_ref = false;
  arg_ts.is_lvalue_ref = false;
  arg_ts.is_rvalue_ref = false;

  const int type_index = template_param_index_for_type_deduction(param_ts, fn_def);
  if (type_index >= 0) {
    const char* pname = fn_def->template_param_names[type_index];
    if (!pname) return false;
    const bool is_pack =
        fn_def->template_param_is_pack && fn_def->template_param_is_pack[type_index];
    return bind_deduced_type(
        deduced_types, is_pack ? pack_binding_key(pname, 0) : std::string(pname),
        arg_ts);
  }

  if (param_ts.base != arg_ts.base ||
      param_ts.ptr_level != arg_ts.ptr_level ||
      param_ts.array_rank != arg_ts.array_rank) {
    return false;
  }

  std::vector<TemplateArgRef> param_arg_scratch;
  std::vector<TemplateArgRef> arg_arg_scratch;
  const TemplateArgRefList param_args =
      template_arg_list_for_deduction(param_ts, param_arg_scratch);
  const TemplateArgRefList arg_args =
      template_arg_list_for_deduction(arg_ts, arg_arg_scratch);
  if (param_args.data && param_args.size > 0) {
    if (!template_origin_identity_match_for_deduction(param_ts, arg_ts)) {
      return false;
    }
    return deduce_template_arg_list(
        param_args, arg_args, fn_def, deduced_types, deduced_nttp);
  }

  if (!typespec_nominal_match_for_deduction(param_ts, arg_ts)) return false;
  return true;
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

TypeBindings Lowerer::build_call_bindings(
    const Node* call_var, const Node* fn_def,
    const TypeBindings* enclosing_bindings,
    HirTemplateTypeBindings* structured_bindings) {
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
    if (structured_bindings) {
      if (auto structured_key = make_hir_template_parameter_binding_key(
              fn_def, i, HirTemplateParameterBindingKind::Type)) {
        (*structured_bindings)[*structured_key] = arg_ts;
      }
    }
  }
  if (fn_def->template_param_has_default) {
    for (int i = 0; i < fn_def->n_template_params; ++i) {
      if (!fn_def->template_param_names[i]) continue;
      if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) continue;
      if (fn_def->template_param_is_pack && fn_def->template_param_is_pack[i]) continue;
      if (bindings.count(fn_def->template_param_names[i])) continue;
      if (fn_def->template_param_has_default[i]) {
        const TypeSpec& default_ts = fn_def->template_param_default_types[i];
        bindings[fn_def->template_param_names[i]] = default_ts;
        if (structured_bindings) {
          if (auto structured_key = make_hir_template_parameter_binding_key(
                  fn_def, i, HirTemplateParameterBindingKind::Type)) {
            (*structured_bindings)[*structured_key] = default_ts;
          }
        }
      }
    }
  }
  return bindings;
}

NttpBindings Lowerer::build_call_nttp_bindings(
    const Node* call_var, const Node* fn_def,
    const NttpBindings* enclosing_nttp,
    const NttpTextBindings* enclosing_nttp_by_text,
    HirTemplateNttpBindings* structured_bindings) {
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
          if (forwarded_text_id != kInvalidText) {
            continue;
          }
          // No TextId carrier: retain legacy rendered-name forwarding.
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
      const auto bind_nttp = [&](long long value) {
        bindings[fn_def->template_param_names[i]] = value;
        if (structured_bindings) {
          if (auto structured_key = make_hir_template_parameter_binding_key(
                  fn_def, i, HirTemplateParameterBindingKind::NonType)) {
            (*structured_bindings)[*structured_key] = value;
          }
        }
      };
      long long expr_value = 0;
      if (call_var->template_arg_exprs && call_var->template_arg_exprs[arg_index]) {
        if (eval_template_arg_expr_with_nttp_bindings(
                call_var->template_arg_exprs[arg_index], enclosing_nttp,
                enclosing_nttp_by_text,
                &expr_value)) {
          bind_nttp(expr_value);
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
            bind_nttp(text_it->second);
            ++arg_index;
            continue;
          }
        }
        if (forwarded_text_id != kInvalidText) {
          ++arg_index;
          continue;
        }
        // No TextId carrier: retain legacy rendered-name forwarding.
        if (enclosing_nttp) {
          auto it = enclosing_nttp->find(call_var->template_arg_nttp_names[arg_index]);
          if (it != enclosing_nttp->end()) {
            bind_nttp(it->second);
            ++arg_index;
            continue;
          }
        }
        ++arg_index;
        continue;
      }
      bind_nttp(call_var->template_arg_values[arg_index]);
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
        const long long default_value = fn_def->template_param_default_values[i];
        bindings[fn_def->template_param_names[i]] = default_value;
        if (structured_bindings) {
          if (auto structured_key = make_hir_template_parameter_binding_key(
                  fn_def, i, HirTemplateParameterBindingKind::NonType)) {
            (*structured_bindings)[*structured_key] = default_value;
          }
        }
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
    } else {
      auto arg_type = try_infer_arg_type_for_deduction(arg, enclosing_fn);
      if (!arg_type) continue;
      NttpBindings ignored_nttp;
      if (!deduce_type_pattern_from_type(
              param_ts, *arg_type, fn_def, deduced, ignored_nttp)) {
        return {};
      }
    }
  }

  return deduced;
}

std::optional<TypeSpec> Lowerer::try_infer_template_call_result_for_deduction(
    FunctionCtx* ctx,
    const Node* call_node) {
  if (!call_node || !call_node->left || call_node->left->kind != NK_VAR ||
      !call_node->left->name || !ct_state_) {
    return std::nullopt;
  }
  if (call_node->kind != NK_CALL && call_node->kind != NK_BUILTIN_CALL) {
    return std::nullopt;
  }

  const Node* callee_def = ct_state_->find_template_def(call_node->left->name);
  if (!callee_def || callee_def->n_template_params <= 0) return std::nullopt;

  HirTemplateTypeBindings structured_call_bindings;
  HirTemplateNttpBindings structured_call_nttp_bindings;
  TypeBindings call_bindings = build_call_bindings(
      call_node->left, callee_def,
      ctx && !ctx->tpl_bindings.empty() ? &ctx->tpl_bindings : nullptr,
      &structured_call_bindings);
  NttpBindings call_nttp_bindings = build_call_nttp_bindings(
      call_node->left, callee_def,
      ctx && !ctx->nttp_bindings.empty() ? &ctx->nttp_bindings : nullptr,
      ctx && !ctx->nttp_bindings_by_text.empty()
          ? &ctx->nttp_bindings_by_text
          : nullptr,
      &structured_call_nttp_bindings);
  observe_template_call_binding_structured_parity(
      callee_def, call_bindings, structured_call_bindings, call_nttp_bindings,
      structured_call_nttp_bindings);

  TypeBindings deduced_types;
  NttpBindings deduced_nttp;
  if (deduce_template_bindings_from_call_args(
          callee_def, ctx, call_node->children, call_node->n_children,
          &deduced_types, &deduced_nttp)) {
    for (const auto& [name, ts] : deduced_types) {
      call_bindings.emplace(name, ts);
    }
    for (const auto& [name, value] : deduced_nttp) {
      call_nttp_bindings.emplace(name, value);
    }
  }
  fill_deduced_defaults(call_bindings, callee_def);

  TypeSpec result_ts = callee_def->type;
  if (!call_bindings.empty()) {
    result_ts = substitute_signature_template_type(result_ts, &call_bindings);
  }
  if (ctx && !ctx->tpl_bindings.empty()) {
    result_ts = substitute_signature_template_type(result_ts, &ctx->tpl_bindings);
  }

  TypeBindings resolution_bindings = ctx ? ctx->tpl_bindings : TypeBindings{};
  for (const auto& [name, ts] : call_bindings) {
    resolution_bindings[name] = ts;
  }
  if (result_ts.tpl_struct_origin) {
    seed_and_resolve_pending_template_type_if_needed(
        result_ts, resolution_bindings, call_nttp_bindings, call_node,
        PendingTemplateTypeKind::OwnerStruct,
        "ctx-deduction-template-call-result");
  }
  resolve_typedef_to_struct(result_ts);
  return reference_value_ts(result_ts);
}

void Lowerer::observe_template_call_binding_structured_parity(
    const Node* fn_def,
    const TypeBindings& legacy_type_bindings,
    const HirTemplateTypeBindings& structured_type_bindings,
    const NttpBindings& legacy_nttp_bindings,
    const HirTemplateNttpBindings& structured_nttp_bindings) const {
  const HirTemplateCallBindingParity parity =
      observe_hir_template_call_binding_parity(
          fn_def, legacy_type_bindings, structured_type_bindings,
          legacy_nttp_bindings, structured_nttp_bindings);
  template_call_binding_structured_parity_checks_ +=
      parity.type_bindings_checked + parity.nttp_bindings_checked;
  template_call_binding_structured_parity_mismatches_ +=
      parity.type_bindings_missing + parity.type_bindings_mismatched +
      parity.nttp_bindings_missing + parity.nttp_bindings_mismatched;
}

bool Lowerer::deduce_template_bindings_from_call_args(
    const Node* fn_def,
    FunctionCtx* ctx,
    Node* const* arg_nodes,
    int nargs,
    TypeBindings* out_type_bindings,
    NttpBindings* out_nttp_bindings) {
  if (!fn_def || !out_type_bindings || !out_nttp_bindings) return false;
  if (fn_def->n_template_params <= 0) return true;
  const int match_count = std::min(nargs, fn_def->n_params);
  for (int i = 0; i < match_count; ++i) {
    const Node* param = fn_def->params[i];
    const Node* arg = arg_nodes ? arg_nodes[i] : nullptr;
    if (!param || !arg) continue;

    TypeSpec arg_ts = infer_generic_ctrl_type(ctx, arg);
    if (ctx && !ctx->tpl_bindings.empty()) {
      arg_ts = substitute_signature_template_type(arg_ts, &ctx->tpl_bindings);
      if (arg_ts.tpl_struct_origin) {
        seed_and_resolve_pending_template_type_if_needed(
            arg_ts, ctx->tpl_bindings, ctx->nttp_bindings, arg,
            PendingTemplateTypeKind::OwnerStruct,
            "ctx-deduction-arg");
      }
    }
    resolve_typedef_to_struct(arg_ts);
    const bool arg_needs_template_call_result =
        arg->kind == NK_CALL && arg->left &&
        (arg_ts.base != TB_STRUCT ||
         (arg_ts.ptr_level == 0 && !arg_ts.record_def &&
          arg_ts.tag_text_id == kInvalidText));
    if (arg_needs_template_call_result) {
      if (auto template_result_ts =
              try_infer_template_call_result_for_deduction(ctx, arg)) {
        arg_ts = *template_result_ts;
        resolve_typedef_to_struct(arg_ts);
      }
    }
    if (arg_ts.base != TB_STRUCT && arg->kind == NK_CALL && arg->left) {
      TypeSpec constructed_ts = infer_generic_ctrl_type(ctx, arg->left);
      if (ctx && !ctx->tpl_bindings.empty()) {
        constructed_ts =
            substitute_signature_template_type(constructed_ts, &ctx->tpl_bindings);
        if (constructed_ts.tpl_struct_origin) {
          seed_and_resolve_pending_template_type_if_needed(
              constructed_ts, ctx->tpl_bindings, ctx->nttp_bindings, arg->left,
              PendingTemplateTypeKind::OwnerStruct,
              "ctx-deduction-constructed-arg");
        }
      }
      resolve_typedef_to_struct(constructed_ts);
      if (constructed_ts.base == TB_STRUCT && constructed_ts.ptr_level == 0) {
        arg_ts = constructed_ts;
      }
    }

    if (!deduce_type_pattern_from_type(
            param->type, arg_ts, fn_def, *out_type_bindings,
            *out_nttp_bindings)) {
      return false;
    }
  }
  fill_deduced_defaults(*out_type_bindings, fn_def);
  return true;
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
  HirTemplateTypeBindings structured_bindings;
  TypeBindings bindings = build_call_bindings(
      call_var, fn_def, enclosing_bindings, &structured_bindings);
  if (!call_node || !fn_def) return bindings;

  TypeBindings deduced = try_deduce_template_type_args(call_node, fn_def, enclosing_fn);
  for (const auto& [name, ts] : deduced) {
    bindings.emplace(name, ts);
  }
  fill_deduced_defaults(bindings, fn_def);
  return bindings;
}

TypeBindings Lowerer::merge_explicit_and_ctx_deduced_type_bindings(
    const Node* call_node, const Node* call_var, const Node* fn_def,
    FunctionCtx* ctx) {
  HirTemplateTypeBindings structured_bindings;
  TypeBindings bindings =
      build_call_bindings(call_var, fn_def,
                          ctx && !ctx->tpl_bindings.empty()
                              ? &ctx->tpl_bindings
                              : nullptr,
                          &structured_bindings);
  if (!call_node || !fn_def || !ctx) return bindings;

  TypeBindings deduced;
  NttpBindings deduced_nttp;
  if (deduce_template_bindings_from_call_args(
          fn_def, ctx, call_node->children, call_node->n_children,
          &deduced, &deduced_nttp)) {
    for (const auto& [name, ts] : deduced) {
      bindings.emplace(name, ts);
    }
  }
  fill_deduced_defaults(bindings, fn_def);
  return bindings;
}

}  // namespace c4c::hir
