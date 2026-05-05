#include "templates.hpp"
#include "consteval.hpp"

#include <climits>
#include <cstring>
#include <functional>
#include <type_traits>
#include <utility>

namespace c4c::hir {

std::string encode_template_type_arg_ref_hir(const TypeSpec& ts);

namespace {

template <typename T, typename = void>
struct HirHasLegacyTypespecTag : std::false_type {};

template <typename T>
struct HirHasLegacyTypespecTag<T, std::void_t<decltype(std::declval<const T&>().tag)>>
    : std::true_type {};

template <typename T>
const char* hir_legacy_typespec_tag_compat(const T& ts) {
  if constexpr (HirHasLegacyTypespecTag<T>::value) {
    return ts.tag;
  } else {
    return nullptr;
  }
}

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

std::string encode_template_arg_debug_compat_hir(const TypeSpec& ts) {
  std::string out;
  if (!ts.tpl_struct_args.data || ts.tpl_struct_args.size <= 0) return out;
  for (int i = 0; i < ts.tpl_struct_args.size; ++i) {
    if (i > 0) out += ",";
    out += encode_template_arg_ref_hir(ts.tpl_struct_args.data[i]);
  }
  return out;
}

template <typename T>
auto typespec_legacy_tag_if_present(const T& ts, int)
    -> decltype(ts.tag, static_cast<const char*>(nullptr)) {
  return ts.tag;
}

const char* typespec_legacy_tag_if_present(const TypeSpec&, long) {
  return nullptr;
}

template <typename T>
auto assign_typespec_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype((void)(ts.tag = tag)) {
  ts.tag = tag;
}

void assign_typespec_legacy_tag_if_present(TypeSpec&, const char*, long) {}

std::optional<HirRecordOwnerKey> template_origin_owner_key_hir(
    const TypeSpec& ts) {
  const QualifiedNameKey& origin_key = ts.tpl_struct_origin_key;
  if (origin_key.base_text_id == kInvalidText) return std::nullopt;
  NamespaceQualifier ns_qual;
  ns_qual.context_id = origin_key.context_id;
  ns_qual.is_global_qualified = origin_key.is_global_qualified;
  if (origin_key.qualifier_path_id != kInvalidNamePath &&
      (!ts.qualifier_text_ids || ts.n_qualifier_segments <= 0)) {
    return std::nullopt;
  }
  if (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) {
    ns_qual.segment_text_ids.assign(
        ts.qualifier_text_ids,
        ts.qualifier_text_ids + ts.n_qualifier_segments);
  }
  HirRecordOwnerKey key =
      make_hir_record_owner_key(ns_qual, origin_key.base_text_id);
  if (!hir_record_owner_key_has_complete_metadata(key)) return std::nullopt;
  return key;
}

std::optional<std::string> typespec_tag_text_spelling_hir(
    const TypeSpec& ts,
    const TextTable* texts) {
  if (ts.tag_text_id == kInvalidText || !texts) return std::nullopt;
  const std::string_view spelling = texts->lookup(ts.tag_text_id);
  if (spelling.empty()) return std::nullopt;
  return std::string(spelling);
}

std::string template_type_arg_nominal_payload_hir(const TypeSpec& ts,
                                                  const char* fallback) {
  if (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) {
    if (ts.record_def->name && ts.record_def->name[0]) {
      return ts.record_def->name;
    }
    if (ts.record_def->unqualified_name && ts.record_def->unqualified_name[0]) {
      return ts.record_def->unqualified_name;
    }
    if (ts.record_def->unqualified_text_id != kInvalidText) {
      return "record_ctx" + std::to_string(ts.record_def->namespace_context_id) +
             "_text" + std::to_string(ts.record_def->unqualified_text_id);
    }
  }
  if (ts.tag_text_id != kInvalidText) {
    std::string out = "tag_ctx" + std::to_string(ts.namespace_context_id);
    out += ts.is_global_qualified ? "_global" : "_local";
    if (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) {
      out += "_q";
      for (int i = 0; i < ts.n_qualifier_segments; ++i) {
        if (i > 0) out += "_";
        out += std::to_string(ts.qualifier_text_ids[i]);
      }
    }
    out += "_text" + std::to_string(ts.tag_text_id);
    return out;
  }
  if (const char* legacy_tag = typespec_legacy_tag_if_present(ts, 0);
      legacy_tag && legacy_tag[0]) {
    return legacy_tag;
  }
  return fallback ? fallback : "";
}

const Node* resolve_static_member_base_def_hir(
    const TypeSpec& base_ts,
    const std::unordered_map<std::string, const Node*>& struct_defs) {
  if (base_ts.record_def && base_ts.record_def->kind == NK_STRUCT_DEF) {
    return base_ts.record_def;
  }
  if (base_ts.tag_text_id != kInvalidText) {
    for (const auto& entry : struct_defs) {
      const Node* candidate = entry.second;
      if (!candidate || candidate->kind != NK_STRUCT_DEF) continue;
      if (candidate->unqualified_text_id != base_ts.tag_text_id) continue;
      if (base_ts.namespace_context_id >= 0 &&
          candidate->namespace_context_id != base_ts.namespace_context_id) {
        continue;
      }
      return candidate;
    }
  }
  if (const char* legacy_tag = typespec_legacy_tag_if_present(base_ts, 0);
      legacy_tag && legacy_tag[0]) {
    auto it = struct_defs.find(legacy_tag);
    if (it != struct_defs.end()) return it->second;
  }
  return nullptr;
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

  const std::string nominal_payload =
      template_type_arg_nominal_payload_hir(ts, "");
  const bool plain_tag_ref =
      !nominal_payload.empty() && !ts.is_const && !ts.is_volatile &&
      ts.ptr_level == 0 && !ts.is_lvalue_ref && !ts.is_rvalue_ref;
  if (plain_tag_ref) return nominal_payload;

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
    case TB_STRUCT:
      ref += "struct_";
      ref += template_type_arg_nominal_payload_hir(ts, "anon");
      break;
    case TB_UNION:
      ref += "union_";
      ref += template_type_arg_nominal_payload_hir(ts, "anon");
      break;
    case TB_ENUM:
      ref += "enum_";
      ref += template_type_arg_nominal_payload_hir(ts, "anon");
      break;
    case TB_TYPEDEF:
      ref += template_type_arg_nominal_payload_hir(ts, "typedef");
      break;
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
    if (const Node* base_def =
            resolve_static_member_base_def_hir(base_ts, struct_defs)) {
      if (eval_struct_static_member_value_hir(
              base_def, struct_defs, member_name, nttp_bindings, out)) {
        return true;
      }
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

namespace {

int hir_type_template_param_index(const Node* tpl_def, const TypeSpec& ts) {
  if (!tpl_def || !tpl_def->template_param_names) return -1;

  const bool has_structured_param_carrier =
      ts.template_param_text_id != kInvalidText ||
      ts.tag_text_id != kInvalidText ||
      ts.template_param_index >= 0 ||
      ts.template_param_owner_text_id != kInvalidText ||
      ts.template_param_owner_namespace_context_id >= 0;

  auto is_type_param_index = [&](int index) {
    return index >= 0 && index < tpl_def->n_template_params &&
           (!tpl_def->template_param_is_nttp ||
            !tpl_def->template_param_is_nttp[index]) &&
           tpl_def->template_param_names[index];
  };

  auto matches_text_id = [&](TextId text_id) {
    if (text_id == kInvalidText || !tpl_def->template_param_name_text_ids) {
      return -1;
    }
    for (int i = 0; i < tpl_def->n_template_params; ++i) {
      if (is_type_param_index(i) &&
          tpl_def->template_param_name_text_ids[i] == text_id) {
        return i;
      }
    }
    return -1;
  };

  if (int index = matches_text_id(ts.template_param_text_id); index >= 0) {
    return index;
  }
  if (int index = matches_text_id(ts.tag_text_id); index >= 0) return index;

  if (ts.template_param_index >= 0) {
    if (!is_type_param_index(ts.template_param_index)) return -1;
    if (ts.template_param_owner_text_id != kInvalidText &&
        tpl_def->unqualified_text_id != kInvalidText &&
        ts.template_param_owner_text_id != tpl_def->unqualified_text_id) {
      return -1;
    }
    if (ts.template_param_owner_namespace_context_id >= 0 &&
        tpl_def->namespace_context_id >= 0 &&
        ts.template_param_owner_namespace_context_id !=
            tpl_def->namespace_context_id) {
      return -1;
    }
    return ts.template_param_index;
  }

  if (has_structured_param_carrier) return -1;

  const char* legacy_tag = hir_legacy_typespec_tag_compat(ts);
  if (!legacy_tag || !legacy_tag[0]) return -1;
  for (int i = 0; i < tpl_def->n_template_params; ++i) {
    if (is_type_param_index(i) &&
        std::strcmp(tpl_def->template_param_names[i], legacy_tag) == 0) {
      return i;
    }
  }
  return -1;
}

const char* hir_type_template_param_name(const Node* tpl_def,
                                         const TypeSpec& ts) {
  const int index = hir_type_template_param_index(tpl_def, ts);
  return index >= 0 ? tpl_def->template_param_names[index] : nullptr;
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

  if (pattern.base == TB_TYPEDEF) {
    const char* param_name = hir_type_template_param_name(tpl_def, pattern);
    if (!param_name) return types_compatible_p(pattern, actual, kEmptyTypedefs);
    TypeSpec bound = hir_strip_pattern_qualifiers(actual, pattern);
    auto it = type_bindings->find(param_name);
    if (it == type_bindings->end()) {
      (*type_bindings)[param_name] = bound;
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
    else if (!hir_type_template_param_name(spec, ts)) score += 4;
    if (ts.is_const || ts.is_volatile || ts.ptr_level > 0 ||
        ts.is_lvalue_ref || ts.is_rvalue_ref || ts.array_rank > 0)
      score += 4;
  }
  return score;
}

}  // namespace

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
  const Node* rendered_primary =
      it != template_struct_defs_.end() ? it->second : nullptr;
  record_template_struct_primary_lookup_parity(rendered_primary);
  return rendered_primary;
}

const Node* Lowerer::find_template_struct_primary(
    const Node* declaration,
    const std::string& rendered_name) const {
  const Node* primary = nullptr;
  const std::optional<HirRecordOwnerKey> owner_key =
      make_struct_def_node_owner_key(declaration);
  if (owner_key) {
    auto owner_it = template_struct_defs_by_owner_.find(*owner_key);
    if (owner_it != template_struct_defs_by_owner_.end()) {
      primary = owner_it->second;
    }
  }
  if (!primary) {
    primary = ct_state_->find_template_struct_def(declaration);
  }
  if (!primary && !owner_key && !rendered_name.empty()) {
    primary = find_template_struct_primary(rendered_name);
  } else {
    record_template_struct_primary_lookup_parity(primary);
  }
  return primary;
}

const std::vector<const Node*>* Lowerer::find_template_struct_specializations(
    const Node* primary_tpl) const {
  if (!primary_tpl || !primary_tpl->name) return nullptr;
  const std::vector<const Node*>* selected_specializations = nullptr;
  if (auto key = make_struct_def_node_owner_key(primary_tpl)) {
    auto owner_it = template_struct_specializations_by_owner_.find(*key);
    if (owner_it != template_struct_specializations_by_owner_.end()) {
      selected_specializations = &owner_it->second;
    }
  }
  if (!selected_specializations) {
    selected_specializations =
        ct_state_->find_template_struct_specializations(primary_tpl);
  }
  if (!selected_specializations) {
    if (const auto* specializations =
            ct_state_->find_template_struct_specializations(primary_tpl,
                                                            primary_tpl->name)) {
      selected_specializations = specializations;
    } else {
      auto it = template_struct_specializations_.find(primary_tpl->name);
      selected_specializations =
          it != template_struct_specializations_.end() ? &it->second : nullptr;
    }
  }
  record_template_struct_specialization_lookup_parity(
      primary_tpl, selected_specializations);
  return selected_specializations;
}

void Lowerer::record_template_struct_primary_lookup_parity(
    const Node* rendered_primary) const {
  if (!rendered_primary) return;
  auto key = make_struct_def_node_owner_key(rendered_primary);
  if (!key) return;
  auto it = template_struct_defs_by_owner_.find(*key);
  ++template_struct_primary_lookup_parity_checks_;
  if (it == template_struct_defs_by_owner_.end() ||
      it->second != rendered_primary) {
    ++template_struct_primary_lookup_parity_mismatches_;
  }
}

void Lowerer::record_template_struct_specialization_lookup_parity(
    const Node* primary_tpl,
    const std::vector<const Node*>* rendered_specializations) const {
  if (!primary_tpl) return;
  auto key = make_struct_def_node_owner_key(primary_tpl);
  if (!key) return;
  auto it = template_struct_specializations_by_owner_.find(*key);

  ++template_struct_specialization_lookup_parity_checks_;
  const std::vector<const Node*>* structured_specializations =
      it == template_struct_specializations_by_owner_.end() ? nullptr
                                                            : &it->second;
  const bool both_missing =
      (!rendered_specializations || rendered_specializations->empty()) &&
      (!structured_specializations || structured_specializations->empty());
  if (both_missing) return;
  if (!rendered_specializations ||
      !structured_specializations ||
      rendered_specializations->size() != structured_specializations->size()) {
    ++template_struct_specialization_lookup_parity_mismatches_;
    return;
  }
  for (size_t i = 0; i < rendered_specializations->size(); ++i) {
    if ((*rendered_specializations)[i] != (*structured_specializations)[i]) {
      ++template_struct_specialization_lookup_parity_mismatches_;
      return;
    }
  }
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
  if (auto key = make_struct_def_node_owner_key(node)) {
    template_struct_defs_by_owner_[*key] = node;
  }
  ct_state_->register_template_struct_def(name, node);
}

void Lowerer::register_template_struct_specialization(
    const Node* primary_tpl,
    const Node* node) {
  if (!primary_tpl || !primary_tpl->name || !node) return;
  template_struct_specializations_[primary_tpl->name].push_back(node);
  if (auto key = make_struct_def_node_owner_key(primary_tpl)) {
    template_struct_specializations_by_owner_[*key].push_back(node);
  }
  ct_state_->register_template_struct_specialization(primary_tpl, node);
}

void Lowerer::seed_pending_template_type(const TypeSpec& ts,
                                         const TypeBindings& tpl_bindings,
                                         const NttpBindings& nttp_bindings,
                                         const Node* span_node,
                                         PendingTemplateTypeKind kind,
                                         const std::string& context_name) {
  if (!ts.tpl_struct_origin && !ts.deferred_member_type_name) return;
  const Node* owner_primary_def = canonical_template_struct_primary(ts, nullptr);
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
  if (auto origin_key = template_origin_owner_key_hir(ts)) {
    auto origin_it = template_struct_defs_by_owner_.find(*origin_key);
    if (origin_it != template_struct_defs_by_owner_.end()) {
      record_template_struct_primary_lookup_parity(origin_it->second);
      return origin_it->second;
    }
    return nullptr;
  }
  const std::string rendered_origin =
      (ts.tpl_struct_origin && ts.tpl_struct_origin[0])
          ? std::string(ts.tpl_struct_origin)
          : std::string{};
  if (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) {
    if (const Node* primary =
            find_template_struct_primary(ts.record_def, rendered_origin)) {
      return primary;
    }
    if (make_struct_def_node_owner_key(ts.record_def)) return nullptr;
  }
  if (rendered_origin.empty()) return nullptr;
  if (const Node* primary = find_template_struct_primary(ts.tpl_struct_origin)) {
    return primary;
  }
  if (const size_t scope_sep = rendered_origin.rfind("::");
      scope_sep != std::string::npos) {
    const std::string unqualified = rendered_origin.substr(scope_sep + 2);
    if (!unqualified.empty()) {
      if (const Node* primary = find_template_struct_primary(unqualified)) {
        return primary;
      }
    }
  }
  auto try_family_root = [&](const std::string& raw_name) -> const Node* {
    const size_t scope_sep = raw_name.rfind("::");
    const size_t search_from = (scope_sep == std::string::npos) ? 0 : (scope_sep + 2);
    const size_t tpl_sep = raw_name.find("_T", search_from);
    if (tpl_sep == std::string::npos) return nullptr;
    const std::string family_name = raw_name.substr(0, tpl_sep);
    if (family_name.empty()) return nullptr;
    if (const Node* primary = find_template_struct_primary(family_name)) {
      return primary;
    }
    if (scope_sep != std::string::npos) {
      const std::string unqualified = family_name.substr(scope_sep + 2);
      if (!unqualified.empty()) return find_template_struct_primary(unqualified);
    }
    return nullptr;
  };
  if (const Node* primary = try_family_root(ts.tpl_struct_origin)) return primary;
  if (auto tag_text = typespec_tag_text_spelling_hir(
          ts, module_ ? module_->link_name_texts.get() : nullptr)) {
    if (const Node* primary = try_family_root(*tag_text)) return primary;
  }
  return nullptr;
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
  bool has_generic_type_arg = false;
  for (const auto& arg : resolved.concrete_args) {
    if (!arg.is_value && !has_concrete_type(arg.type) && !arg.type.tpl_struct_origin) {
      has_generic_type_arg = true;
    }
  }

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

  const auto realized_it = module_->struct_defs.find(mangled);
  const HirStructDef* realized_def =
      realized_it == module_->struct_defs.end() ? nullptr : &realized_it->second;
  assign_typespec_legacy_tag_if_present(
      ts, realized_def ? realized_def->tag.c_str() : nullptr, 0);
  if (realized_def) {
    ts.tag_text_id = realized_def->tag_text_id;
    ts.namespace_context_id = realized_def->ns_qual.context_id;
  }
  if (ts.deferred_member_type_name) {
    assign_template_arg_refs_from_hir_args(&ts, resolved.concrete_args);
  }
  while (resolve_struct_member_typedef_if_ready(&ts)) {
  }
  if (!ts.deferred_member_type_name) {
    ts.tpl_struct_origin = nullptr;
    ts.tpl_struct_args.data = nullptr;
    ts.tpl_struct_args.size = 0;
  }
}

}  // namespace c4c::hir
