#include "consteval.hpp"

#include <cctype>
#include <cstdint>
#include <cstring>

namespace c4c::hir {
namespace {

bool is_any_int_base_local(TypeBase b) {
  switch (b) {
    case TB_BOOL:
    case TB_CHAR:
    case TB_SCHAR:
    case TB_UCHAR:
    case TB_SHORT:
    case TB_USHORT:
    case TB_INT:
    case TB_UINT:
    case TB_LONG:
    case TB_ULONG:
    case TB_LONGLONG:
    case TB_ULONGLONG:
    case TB_INT128:
    case TB_UINT128:
    case TB_ENUM:
      return true;
    default:
      return false;
  }
}

bool is_signed_int_base_local(TypeBase b) {
  switch (b) {
    case TB_CHAR:
    case TB_SCHAR:
    case TB_SHORT:
    case TB_INT:
    case TB_LONG:
    case TB_LONGLONG:
    case TB_INT128:
    case TB_ENUM:
      return true;
    default:
      return false;
  }
}

int int_bits_local(TypeBase b) {
  switch (b) {
    case TB_BOOL: return 1;
    case TB_CHAR:
    case TB_SCHAR:
    case TB_UCHAR: return 8;
    case TB_SHORT:
    case TB_USHORT: return 16;
    case TB_INT:
    case TB_UINT: return 32;
    case TB_LONG:
    case TB_ULONG:
    case TB_LONGLONG:
    case TB_ULONGLONG: return 64;
    case TB_INT128:
    case TB_UINT128: return 128;
    default: return 32;
  }
}

template <typename T>
auto typespec_legacy_display_tag_if_present(const T& ts, int) -> decltype(ts.tag) {
  return ts.tag;
}

const char* typespec_legacy_display_tag_if_present(const TypeSpec&, long) {
  return nullptr;
}

ConstValue apply_integer_cast(long long value, const TypeSpec& ts) {
  if (ts.ptr_level != 0 || ts.array_rank != 0 || !is_any_int_base_local(ts.base)) {
    return ConstValue::make_int(value);
  }
  const int bits = int_bits_local(ts.base);
  if (bits <= 0 || bits >= 64) return ConstValue::make_int(value);

  const unsigned long long mask = (1ULL << bits) - 1;
  const unsigned long long uv = static_cast<unsigned long long>(value) & mask;
  if (is_signed_int_base_local(ts.base) && bits > 1 && (uv >> (bits - 1))) {
    return ConstValue::make_int(static_cast<long long>(uv | ~mask));
  }
  return ConstValue::make_int(static_cast<long long>(uv));
}

enum class TypeBindingLookupStatus {
  NoMetadata,
  Miss,
  Found,
};

struct TypeBindingLookupResult {
  TypeBindingLookupStatus status = TypeBindingLookupStatus::NoMetadata;
  const TypeSpec* type = nullptr;
};

TypeBindingLookupResult lookup_type_binding_by_text(const ConstEvalEnv& env,
                                                    const std::string& name) {
  if (!env.type_bindings_by_text || !env.type_binding_text_ids_by_name) return {};
  auto text_it = env.type_binding_text_ids_by_name->find(name);
  if (text_it == env.type_binding_text_ids_by_name->end()) return {};
  if (text_it->second == kInvalidText) return {TypeBindingLookupStatus::Miss, nullptr};
  auto it = env.type_bindings_by_text->find(text_it->second);
  if (it == env.type_bindings_by_text->end()) {
    return {TypeBindingLookupStatus::Miss, nullptr};
  }
  return {TypeBindingLookupStatus::Found, &it->second};
}

TypeBindingLookupResult lookup_type_binding_by_text_id(const ConstEvalEnv& env,
                                                       TextId text_id) {
  if (!env.type_bindings_by_text || text_id == kInvalidText) return {};
  auto it = env.type_bindings_by_text->find(text_id);
  if (it == env.type_bindings_by_text->end()) {
    return {TypeBindingLookupStatus::Miss, nullptr};
  }
  return {TypeBindingLookupStatus::Found, &it->second};
}

TypeBindingLookupResult lookup_type_binding_by_key(const ConstEvalEnv& env,
                                                   const std::string& name) {
  if (!env.type_bindings_by_key || !env.type_binding_keys_by_name) return {};
  auto key_it = env.type_binding_keys_by_name->find(name);
  if (key_it == env.type_binding_keys_by_name->end()) return {};
  if (!key_it->second.valid()) return {TypeBindingLookupStatus::Miss, nullptr};
  auto it = env.type_bindings_by_key->find(key_it->second);
  if (it == env.type_bindings_by_key->end()) {
    return {TypeBindingLookupStatus::Miss, nullptr};
  }
  return {TypeBindingLookupStatus::Found, &it->second};
}

TypeBindingLookupResult lookup_type_binding_by_typespec_key(const ConstEvalEnv& env,
                                                            const TypeSpec& ts) {
  if (!env.type_bindings_by_key ||
      ts.template_param_owner_text_id == kInvalidText ||
      ts.template_param_index < 0) {
    return {};
  }
  TypeBindingStructuredKey key;
  key.namespace_context_id = ts.template_param_owner_namespace_context_id;
  key.template_text_id = ts.template_param_owner_text_id;
  key.param_index = ts.template_param_index;
  key.param_text_id = ts.template_param_text_id;
  auto it = env.type_bindings_by_key->find(key);
  if (it == env.type_bindings_by_key->end()) {
    return {TypeBindingLookupStatus::Miss, nullptr};
  }
  return {TypeBindingLookupStatus::Found, &it->second};
}

bool has_type_binding_typespec_key_carrier(const TypeSpec& ts) {
  return ts.template_param_owner_text_id != kInvalidText &&
         ts.template_param_index >= 0;
}

bool has_type_binding_typespec_text_carrier(const TypeSpec& ts) {
  return ts.tag_text_id != kInvalidText;
}

bool has_type_binding_metadata_channel(const ConstEvalEnv& env) {
  return (env.type_bindings_by_text && !env.type_bindings_by_text->empty()) ||
         (env.type_bindings_by_key && !env.type_bindings_by_key->empty()) ||
         (env.type_binding_text_ids_by_name &&
          !env.type_binding_text_ids_by_name->empty()) ||
         (env.type_binding_keys_by_name && !env.type_binding_keys_by_name->empty());
}

// Resolve a TypeSpec through type_bindings if it's a TB_TYPEDEF with a known substitution.
TypeSpec resolve_type(const TypeSpec& ts, const ConstEvalEnv& env) {
  if (ts.base != TB_TYPEDEF) return ts;
  const bool has_intrinsic_carrier =
      has_type_binding_typespec_key_carrier(ts) ||
      has_type_binding_typespec_text_carrier(ts);

  const TypeBindingLookupResult intrinsic_key =
      lookup_type_binding_by_typespec_key(env, ts);
  if (intrinsic_key.status == TypeBindingLookupStatus::Found) return *intrinsic_key.type;
  if (intrinsic_key.status == TypeBindingLookupStatus::Miss) return ts;

  const TypeBindingLookupResult intrinsic_text =
      lookup_type_binding_by_text_id(env, ts.tag_text_id);
  if (intrinsic_text.status == TypeBindingLookupStatus::Found) return *intrinsic_text.type;
  if (intrinsic_text.status == TypeBindingLookupStatus::Miss) return ts;
  if (has_intrinsic_carrier && has_type_binding_metadata_channel(env)) return ts;

  if (has_type_binding_metadata_channel(env)) return ts;

  const char* compatibility_tag = typespec_legacy_display_tag_if_present(ts, 0);
  if (!compatibility_tag || !compatibility_tag[0] || !env.type_bindings) return ts;
  const std::string name = compatibility_tag;
  auto it = env.type_bindings->find(name);
  return it != env.type_bindings->end() ? it->second : ts;
}

TextId template_param_text_id_for_param(const Node* func_def, int param_index) {
  if (!func_def || param_index < 0 || param_index >= func_def->n_template_params ||
      !func_def->template_param_name_text_ids) {
    return kInvalidText;
  }
  return func_def->template_param_name_text_ids[param_index];
}

TypeBindingStructuredKey type_binding_key_for_param(const Node* func_def, int param_index) {
  TypeBindingStructuredKey key;
  if (!func_def || param_index < 0) {
    return key;
  }
  key.namespace_context_id = func_def->namespace_context_id;
  key.template_text_id = func_def->unqualified_text_id;
  key.param_index = param_index;
  key.param_text_id = template_param_text_id_for_param(func_def, param_index);
  return key;
}

ConstEvalStructuredNameKey nttp_binding_key_for_param(const Node* func_def, int param_index) {
  ConstEvalStructuredNameKey key;
  key.base_text_id = template_param_text_id_for_param(func_def, param_index);
  return key;
}

void record_type_binding_mirrors(
    const std::string& param_name,
    const TypeSpec& arg_ts,
    const TypeBindingStructuredKey& structured_key,
    TypeBindingTextMap* out_type_bindings_by_text,
    TypeBindingStructuredMap* out_type_bindings_by_key,
    TypeBindingNameTextMap* out_type_binding_text_ids_by_name,
    TypeBindingNameStructuredMap* out_type_binding_keys_by_name) {
  if (structured_key.param_text_id != kInvalidText) {
    if (out_type_bindings_by_text) {
      (*out_type_bindings_by_text)[structured_key.param_text_id] = arg_ts;
    }
    if (out_type_binding_text_ids_by_name) {
      (*out_type_binding_text_ids_by_name)[param_name] = structured_key.param_text_id;
    }
  }
  if (structured_key.valid()) {
    if (out_type_bindings_by_key) (*out_type_bindings_by_key)[structured_key] = arg_ts;
    if (out_type_binding_keys_by_name) {
      (*out_type_binding_keys_by_name)[param_name] = structured_key;
    }
  }
}

void record_nttp_binding_mirrors(
    long long value,
    const ConstEvalStructuredNameKey& structured_key,
    ConstTextMap* out_nttp_bindings_by_text,
    ConstStructuredMap* out_nttp_bindings_by_key) {
  if (!structured_key.valid()) return;
  if (out_nttp_bindings_by_text) {
    (*out_nttp_bindings_by_text)[structured_key.base_text_id] = value;
  }
  if (out_nttp_bindings_by_key) {
    (*out_nttp_bindings_by_key)[structured_key] = value;
  }
}

ConstEvalValueLookupResult lookup_forwarded_nttp_arg_by_text(
    const ConstEvalEnv& env,
    TextId text_id) {
  if (text_id == kInvalidText) return {};
  bool saw_metadata = false;
  if (env.nttp_bindings_by_text && !env.nttp_bindings_by_text->empty()) {
    auto it = env.nttp_bindings_by_text->find(text_id);
    if (it != env.nttp_bindings_by_text->end()) {
      return {ConstEvalValueLookupStatus::Found, it->second};
    }
    saw_metadata = true;
  }
  if (env.nttp_bindings_by_key && !env.nttp_bindings_by_key->empty()) {
    ConstEvalStructuredNameKey key;
    key.base_text_id = text_id;
    auto it = env.nttp_bindings_by_key->find(key);
    if (it != env.nttp_bindings_by_key->end()) {
      return {ConstEvalValueLookupStatus::Found, it->second};
    }
    saw_metadata = true;
  }
  return saw_metadata
             ? ConstEvalValueLookupResult{ConstEvalValueLookupStatus::Miss, 0}
             : ConstEvalValueLookupResult{};
}

std::optional<HirRecordOwnerKey> record_owner_key_from_node(const Node* record_def) {
  if (!record_def || record_def->kind != NK_STRUCT_DEF ||
      record_def->unqualified_text_id == kInvalidText) {
    return std::nullopt;
  }

  NamespaceQualifier ns_qual;
  ns_qual.context_id = record_def->namespace_context_id;
  ns_qual.is_global_qualified = record_def->is_global_qualified;
  if (record_def->qualifier_text_ids && record_def->n_qualifier_segments > 0) {
    ns_qual.segment_text_ids.assign(
        record_def->qualifier_text_ids,
        record_def->qualifier_text_ids + record_def->n_qualifier_segments);
  }

  HirRecordOwnerKey key =
      make_hir_record_owner_key(ns_qual, record_def->unqualified_text_id);
  if (!hir_record_owner_key_has_complete_metadata(key)) return std::nullopt;
  return key;
}

std::optional<HirRecordOwnerKey> record_owner_key_from_typespec(
    const TypeSpec& ts) {
  if (auto key = record_owner_key_from_node(ts.record_def); key.has_value()) {
    return key;
  }
  if (ts.namespace_context_id < 0 || ts.tag_text_id == kInvalidText) {
    return std::nullopt;
  }

  NamespaceQualifier ns_qual;
  ns_qual.context_id = ts.namespace_context_id;
  ns_qual.is_global_qualified = ts.is_global_qualified;
  if (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) {
    ns_qual.segment_text_ids.assign(
        ts.qualifier_text_ids,
        ts.qualifier_text_ids + ts.n_qualifier_segments);
  }

  HirRecordOwnerKey key = make_hir_record_owner_key(ns_qual, ts.tag_text_id);
  if (!hir_record_owner_key_has_complete_metadata(key)) return std::nullopt;
  return key;
}

bool has_record_layout_metadata(const TypeSpec& ts, const ConstEvalEnv& env) {
  return (ts.record_def != nullptr || record_owner_key_from_typespec(ts).has_value()) &&
         env.struct_def_owner_index;
}

const HirStructDef* lookup_record_layout(const TypeSpec& ts, const ConstEvalEnv& env) {
  if (ts.base != TB_STRUCT && ts.base != TB_UNION) return nullptr;
  if (!env.struct_defs) return nullptr;

  if (env.struct_def_owner_index) {
    if (auto owner_key = record_owner_key_from_typespec(ts); owner_key.has_value()) {
      const auto owner_it = env.struct_def_owner_index->find(*owner_key);
      if (owner_it != env.struct_def_owner_index->end()) {
        const auto layout_it = env.struct_defs->find(owner_it->second);
        if (layout_it != env.struct_defs->end()) return &layout_it->second;
      }
    }
  }

  const char* compatibility_tag = typespec_legacy_display_tag_if_present(ts, 0);
  if (!compatibility_tag || !compatibility_tag[0]) return nullptr;

  // When owner_index is the structured authority, recover from TextId
  // intern-table mismatches by accepting an owner entry whose rendered tag
  // and namespace match this TypeSpec. A bare rendered-tag match without an
  // owner_index entry would let stale spelling silently substitute identity,
  // so refuse it.
  if (env.struct_def_owner_index) {
    for (const auto& [k, v] : *env.struct_def_owner_index) {
      if (std::string_view(v) != compatibility_tag) continue;
      if (k.namespace_context_id != ts.namespace_context_id) continue;
      if (k.is_global_qualified != ts.is_global_qualified) continue;
      if (k.qualifier_segment_text_ids.size() !=
          static_cast<size_t>(ts.n_qualifier_segments)) continue;
      bool seg_match = true;
      for (size_t i = 0; i < k.qualifier_segment_text_ids.size(); ++i) {
        if (!ts.qualifier_text_ids ||
            k.qualifier_segment_text_ids[i] != ts.qualifier_text_ids[i]) {
          seg_match = false;
          break;
        }
      }
      if (!seg_match) continue;
      const auto layout_it = env.struct_defs->find(v);
      if (layout_it != env.struct_defs->end()) return &layout_it->second;
    }
    return nullptr;
  }

  // No owner_index supplied: fall back to rendered-tag compatibility.
  auto it = env.struct_defs->find(compatibility_tag);
  if (it == env.struct_defs->end()) return nullptr;
  return &it->second;
}

// Compute sizeof for a TypeSpec using the optional HIR layout map when a
// tagged record became complete later in the translation unit.
ConstEvalResult compute_sizeof_type(const TypeSpec& ts, const ConstEvalEnv& env) {
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return ConstEvalResult::success(ConstValue::make_int(8));
  if (ts.base == TB_VOID || ts.base == TB_TYPEDEF)
    return ConstEvalResult::failure("sizeof: unresolved type");
  int sz = 0;
  switch (ts.base) {
    case TB_BOOL: case TB_CHAR: case TB_SCHAR: case TB_UCHAR: sz = 1; break;
    case TB_SHORT: case TB_USHORT: sz = 2; break;
    case TB_INT: case TB_UINT: case TB_FLOAT: sz = 4; break;
    case TB_LONG: case TB_ULONG: case TB_LONGLONG: case TB_ULONGLONG:
    case TB_DOUBLE: sz = 8; break;
    case TB_LONGDOUBLE: sz = 16; break;
    case TB_INT128: case TB_UINT128: sz = 16; break;
    case TB_STRUCT:
    case TB_UNION: {
      const HirStructDef* layout = lookup_record_layout(ts, env);
      if (!layout) return ConstEvalResult::failure("sizeof: unresolved record layout");
      sz = layout->size_bytes;
      break;
    }
    default:
      return ConstEvalResult::failure("sizeof: unsupported type base");
  }
  if (ts.array_rank > 0) {
    if (ts.array_size <= 0) {
      return ConstEvalResult::failure("sizeof: unresolved array bound");
    }
    long long total = sz;
    total *= ts.array_size;
    for (int i = 1; i < ts.array_rank && i < 4; ++i) {
      if (ts.array_dims[i] <= 0) {
        return ConstEvalResult::failure("sizeof: unresolved array bound");
      }
      total *= ts.array_dims[i];
    }
    return ConstEvalResult::success(ConstValue::make_int(total));
  }
  return ConstEvalResult::success(ConstValue::make_int(sz));
}

// Compute alignof for a TypeSpec (scalar, pointer, array).
// Uses HIR layout metadata when available for tagged records.
ConstEvalResult compute_alignof_type(const TypeSpec& ts, const ConstEvalEnv& env) {
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return ConstEvalResult::success(ConstValue::make_int(8));
  if (ts.base == TB_VOID || ts.base == TB_TYPEDEF)
    return ConstEvalResult::failure("alignof: unresolved type");
  // For arrays, alignment is that of the element type.
  TypeSpec elem = ts;
  if (elem.array_rank > 0) {
    elem.array_rank = 0;
    elem.array_size = -1;
  }
  int align = 1;
  switch (elem.base) {
    case TB_BOOL: case TB_CHAR: case TB_SCHAR: case TB_UCHAR: align = 1; break;
    case TB_SHORT: case TB_USHORT: align = 2; break;
    case TB_INT: case TB_UINT: case TB_FLOAT: case TB_ENUM: align = 4; break;
    case TB_LONG: case TB_ULONG: case TB_LONGLONG: case TB_ULONGLONG:
    case TB_DOUBLE: align = 8; break;
    case TB_LONGDOUBLE: align = 16; break;
    case TB_INT128: case TB_UINT128: align = 16; break;
    case TB_STRUCT:
    case TB_UNION: {
      const HirStructDef* layout = lookup_record_layout(ts, env);
      if (!layout) return ConstEvalResult::failure("alignof: unresolved record layout");
      align = layout->align_bytes;
      break;
    }
    default: align = 4; break;
  }
  if (ts.align_bytes > 0 && ts.align_bytes > align) align = ts.align_bytes;
  return ConstEvalResult::success(ConstValue::make_int(align));
}

ConstEvalResult eval_impl(const Node* n, const ConstEvalEnv& env) {
  if (!n) return ConstEvalResult::failure("null expression");
  switch (n->kind) {
    case NK_INT_LIT:
    case NK_CHAR_LIT:
      return ConstEvalResult::success(ConstValue::make_int(n->ival));
    case NK_VAR: {
      if (n->name && n->name[0]) {
        if (n->is_concept_id) {
          // The parser only tags names it already recognized as concept-ids.
          // Treat them as constant true until full concept satisfaction exists.
          return ConstEvalResult::success(ConstValue::make_bool(true));
        }
        auto v = env.lookup(n);
        if (v) return ConstEvalResult::success(ConstValue::make_int(*v));
        return ConstEvalResult::failure(
            std::string("read of non-constant variable '") + n->name + "'");
      }
      return ConstEvalResult::failure("unnamed variable reference");
    }
    case NK_CAST: {
      auto r = eval_impl(n->left, env);
      if (!r.ok()) return r;
      return ConstEvalResult::success(apply_integer_cast(r.as_int(), n->type));
    }
    case NK_UNARY: {
      auto r = eval_impl(n->left, env);
      if (!r.ok()) return r;
      if (!n->op) return ConstEvalResult::failure("unary operator missing");
      long long v = r.as_int();
      if (std::strcmp(n->op, "+") == 0) return ConstEvalResult::success(ConstValue::make_int(v));
      if (std::strcmp(n->op, "-") == 0) return ConstEvalResult::success(ConstValue::make_int(-v));
      if (std::strcmp(n->op, "~") == 0) return ConstEvalResult::success(ConstValue::make_int(~v));
      if (std::strcmp(n->op, "!") == 0) return ConstEvalResult::success(ConstValue::make_int(!v));
      return ConstEvalResult::failure(
          std::string("unsupported unary operator '") + n->op + "' in constant expression");
    }
    case NK_BINOP: {
      if (!n->op) return ConstEvalResult::failure("binary operator missing");
      if (std::strcmp(n->op, "&&") == 0) {
        auto lr = eval_impl(n->left, env);
        if (!lr.ok()) return lr;
        if (!lr.as_int()) return ConstEvalResult::success(ConstValue::make_int(0));
        auto rr = eval_impl(n->right, env);
        if (!rr.ok()) return rr;
        return ConstEvalResult::success(ConstValue::make_int(static_cast<long long>(!!rr.as_int())));
      }
      if (std::strcmp(n->op, "||") == 0) {
        auto lr = eval_impl(n->left, env);
        if (!lr.ok()) return lr;
        if (lr.as_int()) return ConstEvalResult::success(ConstValue::make_int(1));
        auto rr = eval_impl(n->right, env);
        if (!rr.ok()) return rr;
        return ConstEvalResult::success(ConstValue::make_int(static_cast<long long>(!!rr.as_int())));
      }
      auto lr = eval_impl(n->left, env);
      auto rr = eval_impl(n->right, env);
      if (!lr.ok()) return lr;
      if (!rr.ok()) return rr;
      long long l = lr.as_int(), r = rr.as_int();
      long long result;
      if (std::strcmp(n->op, "+") == 0) result = l + r;
      else if (std::strcmp(n->op, "-") == 0) result = l - r;
      else if (std::strcmp(n->op, "*") == 0) result = l * r;
      else if (std::strcmp(n->op, "/") == 0) {
        if (r == 0) return ConstEvalResult::failure("division by zero in constant expression");
        result = l / r;
      } else if (std::strcmp(n->op, "%") == 0) {
        if (r == 0) return ConstEvalResult::failure("modulo by zero in constant expression");
        result = l % r;
      }
      else if (std::strcmp(n->op, "<<") == 0) result = l << r;
      else if (std::strcmp(n->op, ">>") == 0) result = l >> r;
      else if (std::strcmp(n->op, "&") == 0) result = l & r;
      else if (std::strcmp(n->op, "|") == 0) result = l | r;
      else if (std::strcmp(n->op, "^") == 0) result = l ^ r;
      else if (std::strcmp(n->op, "<") == 0) result = static_cast<long long>(l < r);
      else if (std::strcmp(n->op, "<=") == 0) result = static_cast<long long>(l <= r);
      else if (std::strcmp(n->op, ">") == 0) result = static_cast<long long>(l > r);
      else if (std::strcmp(n->op, ">=") == 0) result = static_cast<long long>(l >= r);
      else if (std::strcmp(n->op, "==") == 0) result = static_cast<long long>(l == r);
      else if (std::strcmp(n->op, "!=") == 0) result = static_cast<long long>(l != r);
      else return ConstEvalResult::failure(
          std::string("unsupported binary operator '") + n->op + "' in constant expression");
      return ConstEvalResult::success(ConstValue::make_int(result));
    }
    case NK_TERNARY: {
      auto cr = eval_impl(n->cond ? n->cond : n->left, env);
      if (!cr.ok()) return cr;
      return eval_impl(cr.as_int() ? n->then_ : n->else_, env);
    }
    case NK_SIZEOF_TYPE:
      return compute_sizeof_type(resolve_type(n->type, env), env);
    case NK_SIZEOF_EXPR:
      // sizeof(expr) — use the expression's type from the AST.
      if (n->left) return compute_sizeof_type(resolve_type(n->left->type, env), env);
      return ConstEvalResult::failure("sizeof(expr): missing expression");
    case NK_SIZEOF_PACK:
      return ConstEvalResult::failure("sizeof...(pack) requires template pack instantiation");
    case NK_ALIGNOF_TYPE:
      return compute_alignof_type(resolve_type(n->type, env), env);
    case NK_ALIGNOF_EXPR:
      // alignof(expr) — use the expression's type from the AST.
      if (n->left) return compute_alignof_type(resolve_type(n->left->type, env), env);
      return ConstEvalResult::failure("alignof(expr): missing expression");
    default:
      return ConstEvalResult::failure(
          std::string("unsupported expression kind (NK=") +
          std::to_string(static_cast<int>(n->kind)) + ") in constant expression");
  }
}

std::string decode_c_escaped_bytes_local(const std::string& raw) {
  std::string out;
  for (size_t i = 0; i < raw.size();) {
    const unsigned char c = static_cast<unsigned char>(raw[i]);
    if (c != '\\') {
      out.push_back(static_cast<char>(c));
      ++i;
      continue;
    }
    ++i;
    if (i >= raw.size()) break;
    const char esc = raw[i++];
    switch (esc) {
      case 'n': out.push_back('\n'); break;
      case 't': out.push_back('\t'); break;
      case 'r': out.push_back('\r'); break;
      case 'a': out.push_back('\a'); break;
      case 'b': out.push_back('\b'); break;
      case 'f': out.push_back('\f'); break;
      case 'v': out.push_back('\v'); break;
      case '\\': out.push_back('\\'); break;
      case '\'': out.push_back('\''); break;
      case '"': out.push_back('"'); break;
      case '?': out.push_back('\?'); break;
      case 'x': {
        unsigned value = 0;
        while (i < raw.size() && std::isxdigit(static_cast<unsigned char>(raw[i]))) {
          const unsigned char h = static_cast<unsigned char>(raw[i++]);
          value = value * 16 + (std::isdigit(h) ? h - '0' : std::tolower(h) - 'a' + 10);
        }
        out.push_back(static_cast<char>(value & 0xFF));
        break;
      }
      default:
        if (esc >= '0' && esc <= '7') {
          unsigned value = static_cast<unsigned>(esc - '0');
          for (int k = 0; k < 2 && i < raw.size() && raw[i] >= '0' && raw[i] <= '7'; ++k, ++i) {
            value = value * 8 + static_cast<unsigned>(raw[i] - '0');
          }
          out.push_back(static_cast<char>(value & 0xFF));
        } else {
          out.push_back(esc);
        }
        break;
    }
  }
  return out;
}

}  // namespace

// ── Unified evaluator entry point ────────────────────────────────────────────

ConstEvalResult evaluate_constant_expr(const Node* n, const ConstEvalEnv& env) {
  return eval_impl(n, env);
}

ConstEvalEnv bind_consteval_call_env(
    const Node* callee_expr,
    const Node* func_def,
    const ConstEvalEnv& outer_env,
    TypeBindings* out_type_bindings,
    std::unordered_map<std::string, long long>* out_nttp_bindings,
    TypeBindingTextMap* out_type_bindings_by_text,
    TypeBindingStructuredMap* out_type_bindings_by_key,
    TypeBindingNameTextMap* out_type_binding_text_ids_by_name,
    TypeBindingNameStructuredMap* out_type_binding_keys_by_name,
    ConstTextMap* out_nttp_bindings_by_text,
    ConstStructuredMap* out_nttp_bindings_by_key) {
  if (out_type_bindings) out_type_bindings->clear();
  if (out_nttp_bindings) out_nttp_bindings->clear();
  if (out_type_bindings_by_text) out_type_bindings_by_text->clear();
  if (out_type_bindings_by_key) out_type_bindings_by_key->clear();
  if (out_type_binding_text_ids_by_name) out_type_binding_text_ids_by_name->clear();
  if (out_type_binding_keys_by_name) out_type_binding_keys_by_name->clear();
  if (out_nttp_bindings_by_text) out_nttp_bindings_by_text->clear();
  if (out_nttp_bindings_by_key) out_nttp_bindings_by_key->clear();

  ConstEvalEnv env = outer_env;
  if (!callee_expr || !func_def || func_def->n_template_params <= 0 ||
      !(callee_expr->n_template_args > 0 || callee_expr->has_template_args)) {
    return env;
  }

  const int count = std::min(callee_expr->n_template_args, func_def->n_template_params);
  for (int i = 0; i < count; ++i) {
    if (!func_def->template_param_names || !func_def->template_param_names[i]) continue;
    const std::string param_name = func_def->template_param_names[i];
    const bool is_nttp =
        func_def->template_param_is_nttp && func_def->template_param_is_nttp[i];
    if (is_nttp) {
      if (!out_nttp_bindings || !callee_expr->template_arg_is_value ||
          !callee_expr->template_arg_is_value[i]) {
        continue;
      }
      const bool has_expr_carrier =
          callee_expr->template_arg_exprs && callee_expr->template_arg_exprs[i];
      if (has_expr_carrier) {
        auto expr_value =
            evaluate_constant_expr(callee_expr->template_arg_exprs[i], outer_env);
        if (expr_value.ok()) {
          (*out_nttp_bindings)[param_name] = expr_value.as_int();
          record_nttp_binding_mirrors(
              expr_value.as_int(), nttp_binding_key_for_param(func_def, i),
              out_nttp_bindings_by_text, out_nttp_bindings_by_key);
          continue;
        }
      }
      const TextId forwarded_text_id =
          callee_expr->template_arg_nttp_text_ids
              ? callee_expr->template_arg_nttp_text_ids[i]
              : kInvalidText;
      const ConstEvalValueLookupResult text_result =
          lookup_forwarded_nttp_arg_by_text(outer_env, forwarded_text_id);
      if (text_result.status == ConstEvalValueLookupStatus::Found) {
        (*out_nttp_bindings)[param_name] = text_result.value;
        record_nttp_binding_mirrors(
            text_result.value, nttp_binding_key_for_param(func_def, i),
            out_nttp_bindings_by_text, out_nttp_bindings_by_key);
        continue;
      }
      if (forwarded_text_id != kInvalidText) {
        continue;
      }
      if (text_result.status == ConstEvalValueLookupStatus::Miss) {
        continue;
      }
      if (has_expr_carrier) {
        continue;
      }
      if (callee_expr->template_arg_nttp_names && callee_expr->template_arg_nttp_names[i] &&
          outer_env.nttp_bindings) {
        auto it = outer_env.nttp_bindings->find(callee_expr->template_arg_nttp_names[i]);
        if (it != outer_env.nttp_bindings->end()) {
          (*out_nttp_bindings)[param_name] = it->second;
          record_nttp_binding_mirrors(
              it->second, nttp_binding_key_for_param(func_def, i),
              out_nttp_bindings_by_text, out_nttp_bindings_by_key);
          continue;
        }
      }
      if (callee_expr->template_arg_values) {
        (*out_nttp_bindings)[param_name] = callee_expr->template_arg_values[i];
        record_nttp_binding_mirrors(
            callee_expr->template_arg_values[i], nttp_binding_key_for_param(func_def, i),
            out_nttp_bindings_by_text, out_nttp_bindings_by_key);
      }
      continue;
    }

    if (!out_type_bindings || !callee_expr->template_arg_types) continue;
    TypeSpec arg_ts = callee_expr->template_arg_types[i];
    arg_ts = resolve_type(arg_ts, outer_env);
    (*out_type_bindings)[param_name] = arg_ts;
    record_type_binding_mirrors(
        param_name, arg_ts, type_binding_key_for_param(func_def, i),
        out_type_bindings_by_text, out_type_bindings_by_key,
        out_type_binding_text_ids_by_name, out_type_binding_keys_by_name);
  }

  if (func_def->template_param_has_default) {
    for (int i = count; i < func_def->n_template_params; ++i) {
      if (!func_def->template_param_names || !func_def->template_param_names[i] ||
          !func_def->template_param_has_default[i]) {
        continue;
      }
      const std::string param_name = func_def->template_param_names[i];
      const bool is_nttp =
          func_def->template_param_is_nttp && func_def->template_param_is_nttp[i];
      if (is_nttp) {
        if (!out_nttp_bindings) continue;
        (*out_nttp_bindings)[param_name] = func_def->template_param_default_values[i];
        record_nttp_binding_mirrors(
            func_def->template_param_default_values[i], nttp_binding_key_for_param(func_def, i),
            out_nttp_bindings_by_text, out_nttp_bindings_by_key);
      } else {
        if (!out_type_bindings) continue;
        TypeSpec arg_ts = func_def->template_param_default_types[i];
        arg_ts = resolve_type(arg_ts, outer_env);
        (*out_type_bindings)[param_name] = arg_ts;
        record_type_binding_mirrors(
            param_name, arg_ts, type_binding_key_for_param(func_def, i),
            out_type_bindings_by_text, out_type_bindings_by_key,
            out_type_binding_text_ids_by_name, out_type_binding_keys_by_name);
      }
    }
  }

  if (out_type_bindings && !out_type_bindings->empty()) {
    env.type_bindings = out_type_bindings;
    if (out_type_bindings_by_text && !out_type_bindings_by_text->empty()) {
      env.type_bindings_by_text = out_type_bindings_by_text;
    }
    if (out_type_bindings_by_key && !out_type_bindings_by_key->empty()) {
      env.type_bindings_by_key = out_type_bindings_by_key;
    }
    if (out_type_binding_text_ids_by_name && !out_type_binding_text_ids_by_name->empty()) {
      env.type_binding_text_ids_by_name = out_type_binding_text_ids_by_name;
    }
    if (out_type_binding_keys_by_name && !out_type_binding_keys_by_name->empty()) {
      env.type_binding_keys_by_name = out_type_binding_keys_by_name;
    }
  }
  if (out_nttp_bindings && !out_nttp_bindings->empty()) {
    env.nttp_bindings = out_nttp_bindings;
    if (out_nttp_bindings_by_text && !out_nttp_bindings_by_text->empty()) {
      env.nttp_bindings_by_text = out_nttp_bindings_by_text;
    }
    if (out_nttp_bindings_by_key && !out_nttp_bindings_by_key->empty()) {
      env.nttp_bindings_by_key = out_nttp_bindings_by_key;
    }
  }
  return env;
}

// ── Consteval function-body interpreter ──────────────────────────────────────

namespace {

constexpr int kMaxConstevalDepth = 64;

constexpr int kMaxConstevalSteps = 1000000;  // prevent infinite loops

// Result of interpreting a statement: either continue executing, or return/break/continue.
struct StmtResult {
  bool returned = false;
  bool did_break = false;
  bool did_continue = false;
  ConstValue return_val = ConstValue::unknown();
  std::string error;  // non-empty if evaluation failed
};

std::optional<ConstEvalStructuredNameKey> consteval_local_key(const Node* n) {
  if (!n || n->unqualified_text_id == kInvalidText) return std::nullopt;
  if (n->is_global_qualified || n->n_qualifier_segments > 0) return std::nullopt;
  ConstEvalStructuredNameKey key;
  key.base_text_id = n->unqualified_text_id;
  return key;
}

std::optional<ConstEvalStructuredNameKey> consteval_symbol_key(const Node* n) {
  if (!n || n->namespace_context_id < 0 ||
      n->unqualified_text_id == kInvalidText) {
    return std::nullopt;
  }
  if (n->n_qualifier_segments < 0) return std::nullopt;
  if (n->n_qualifier_segments > 0 && !n->qualifier_text_ids) {
    return std::nullopt;
  }
  ConstEvalStructuredNameKey key;
  key.namespace_context_id = n->namespace_context_id;
  key.is_global_qualified = n->is_global_qualified;
  key.base_text_id = n->unqualified_text_id;
  key.qualifier_text_ids.reserve(static_cast<std::size_t>(n->n_qualifier_segments));
  for (int i = 0; i < n->n_qualifier_segments; ++i) {
    const TextId segment = n->qualifier_text_ids[i];
    if (segment == kInvalidText) return std::nullopt;
    key.qualifier_text_ids.push_back(segment);
  }
  return key;
}

template <typename T>
bool compare_ptrs(const T* legacy, const T* structured) {
  if (!structured) return true;
  return legacy && legacy == structured;
}

const Node* lookup_consteval_function_by_text(const ConstEvalFunctionTextMap* map,
                                              const Node* callee) {
  if (!map || !callee || callee->unqualified_text_id == kInvalidText ||
      callee->is_global_qualified || callee->n_qualifier_segments > 0) {
    return nullptr;
  }
  auto it = map->find(callee->unqualified_text_id);
  return it != map->end() ? it->second : nullptr;
}

const Node* lookup_consteval_function_by_key(const ConstEvalFunctionStructuredMap* map,
                                             const Node* callee) {
  if (!map) return nullptr;
  auto key = consteval_symbol_key(callee);
  if (!key.has_value() || !key->valid()) return nullptr;
  auto it = map->find(*key);
  return it != map->end() ? it->second : nullptr;
}

const Node* lookup_consteval_function(
    const Node* callee,
    const std::unordered_map<std::string, const Node*>& consteval_fns,
    const ConstEvalFunctionTextMap* consteval_fns_by_text,
    const ConstEvalFunctionStructuredMap* consteval_fns_by_key) {
  if (!callee || !callee->name) return nullptr;
  auto it = consteval_fns.find(callee->name);
  const Node* legacy = it != consteval_fns.end() ? it->second : nullptr;
  const Node* text = lookup_consteval_function_by_text(consteval_fns_by_text, callee);
  (void)compare_ptrs(legacy, text);
  bool has_authoritative_metadata = text != nullptr;
  if (auto key = consteval_symbol_key(callee); key.has_value() && key->valid()) {
    has_authoritative_metadata = has_authoritative_metadata || consteval_fns_by_key != nullptr;
    const Node* structured = lookup_consteval_function_by_key(consteval_fns_by_key, callee);
    (void)compare_ptrs(legacy, structured);
    if (structured) return structured;
  }
  if (consteval_fns_by_text && callee->unqualified_text_id != kInvalidText &&
      !callee->is_global_qualified && callee->n_qualifier_segments == 0) {
    has_authoritative_metadata = true;
  }
  if (text) return text;
  if (has_authoritative_metadata) return nullptr;
  return legacy;
}

struct InterpreterBindingSnapshot {
  std::string name;
  bool had_name = false;
  long long name_value = 0;
  TextId text_id = kInvalidText;
  bool had_text = false;
  long long text_value = 0;
  std::optional<ConstEvalStructuredNameKey> key;
  bool had_key = false;
  long long key_value = 0;
};

struct InterpreterBindings {
  ConstMap by_name;
  ConstTextMap by_text;
  ConstStructuredMap by_key;

  void bind(const Node* n, long long value) {
    if (!n || !n->name || !n->name[0]) return;
    by_name[n->name] = value;
    if (n->unqualified_text_id != kInvalidText &&
        !n->is_global_qualified && n->n_qualifier_segments == 0) {
      by_text[n->unqualified_text_id] = value;
    }
    if (auto key = consteval_local_key(n); key.has_value() && key->valid()) {
      by_key[*key] = value;
    }
  }

  InterpreterBindingSnapshot snapshot(const Node* n) const {
    InterpreterBindingSnapshot out;
    if (!n || !n->name || !n->name[0]) return out;
    out.name = n->name;
    auto name_it = by_name.find(out.name);
    if (name_it != by_name.end()) {
      out.had_name = true;
      out.name_value = name_it->second;
    }
    if (n->unqualified_text_id != kInvalidText &&
        !n->is_global_qualified && n->n_qualifier_segments == 0) {
      out.text_id = n->unqualified_text_id;
      auto text_it = by_text.find(out.text_id);
      if (text_it != by_text.end()) {
        out.had_text = true;
        out.text_value = text_it->second;
      }
    }
    out.key = consteval_local_key(n);
    if (out.key.has_value() && out.key->valid()) {
      auto key_it = by_key.find(*out.key);
      if (key_it != by_key.end()) {
        out.had_key = true;
        out.key_value = key_it->second;
      }
    }
    return out;
  }

  void restore(const InterpreterBindingSnapshot& snapshot) {
    if (!snapshot.name.empty()) {
      if (snapshot.had_name) by_name[snapshot.name] = snapshot.name_value;
      else by_name.erase(snapshot.name);
    }
    if (snapshot.text_id != kInvalidText) {
      if (snapshot.had_text) by_text[snapshot.text_id] = snapshot.text_value;
      else by_text.erase(snapshot.text_id);
    }
    if (snapshot.key.has_value() && snapshot.key->valid()) {
      if (snapshot.had_key) by_key[*snapshot.key] = snapshot.key_value;
      else by_key.erase(*snapshot.key);
    }
  }
};

StmtResult interp_stmt(const Node* n, InterpreterBindings& locals,
                       const ConstEvalEnv& outer_env,
                       const std::unordered_map<std::string, const Node*>& consteval_fns,
                       const ConstEvalFunctionTextMap* consteval_fns_by_text,
                       const ConstEvalFunctionStructuredMap* consteval_fns_by_key,
                       int depth, int& steps);

ConstEvalResult interp_expr(const Node* n, InterpreterBindings& locals,
                            const ConstEvalEnv& outer_env,
                            const std::unordered_map<std::string, const Node*>& consteval_fns,
                            const ConstEvalFunctionTextMap* consteval_fns_by_text,
                            const ConstEvalFunctionStructuredMap* consteval_fns_by_key,
                            int depth) {
  if (!n) return ConstEvalResult::failure("null expression in consteval body");

  // Build an env that includes the interpreter locals on top of the outer env.
  ConstEvalEnv env = outer_env;
  env.local_consts = &locals.by_name;
  env.local_consts_by_text = &locals.by_text;
  env.local_consts_by_key = &locals.by_key;

  switch (n->kind) {
    case NK_INT_LIT:
    case NK_CHAR_LIT:
      return eval_impl(n, env);

    case NK_VAR:
      return eval_impl(n, env);

    case NK_CAST: {
      auto r = interp_expr(n->left, locals, outer_env, consteval_fns,
                           consteval_fns_by_text, consteval_fns_by_key, depth);
      if (!r.ok()) return r;
      return ConstEvalResult::success(apply_integer_cast(r.as_int(),
          resolve_type(n->type, env)));
    }

    case NK_UNARY: {
      auto r = interp_expr(n->left, locals, outer_env, consteval_fns,
                           consteval_fns_by_text, consteval_fns_by_key, depth);
      if (!r.ok()) return r;
      if (!n->op) return ConstEvalResult::failure("unary operator missing");
      long long v = r.as_int();
      if (std::strcmp(n->op, "+") == 0) return ConstEvalResult::success(ConstValue::make_int(v));
      if (std::strcmp(n->op, "-") == 0) return ConstEvalResult::success(ConstValue::make_int(-v));
      if (std::strcmp(n->op, "~") == 0) return ConstEvalResult::success(ConstValue::make_int(~v));
      if (std::strcmp(n->op, "!") == 0) return ConstEvalResult::success(ConstValue::make_int(!v));
      return ConstEvalResult::failure(
          std::string("unsupported unary operator '") + n->op + "' in consteval context");
    }

    case NK_BINOP: {
      if (!n->op) return ConstEvalResult::failure("binary operator missing");
      if (std::strcmp(n->op, "&&") == 0) {
        auto lr = interp_expr(n->left, locals, outer_env, consteval_fns,
                              consteval_fns_by_text, consteval_fns_by_key, depth);
        if (!lr.ok()) return lr;
        if (!lr.as_int()) return ConstEvalResult::success(ConstValue::make_int(0));
        auto rr = interp_expr(n->right, locals, outer_env, consteval_fns,
                              consteval_fns_by_text, consteval_fns_by_key, depth);
        if (!rr.ok()) return rr;
        return ConstEvalResult::success(ConstValue::make_int(static_cast<long long>(!!rr.as_int())));
      }
      if (std::strcmp(n->op, "||") == 0) {
        auto lr = interp_expr(n->left, locals, outer_env, consteval_fns,
                              consteval_fns_by_text, consteval_fns_by_key, depth);
        if (!lr.ok()) return lr;
        if (lr.as_int()) return ConstEvalResult::success(ConstValue::make_int(1));
        auto rr = interp_expr(n->right, locals, outer_env, consteval_fns,
                              consteval_fns_by_text, consteval_fns_by_key, depth);
        if (!rr.ok()) return rr;
        return ConstEvalResult::success(ConstValue::make_int(static_cast<long long>(!!rr.as_int())));
      }
      auto lr = interp_expr(n->left, locals, outer_env, consteval_fns,
                            consteval_fns_by_text, consteval_fns_by_key, depth);
      auto rr = interp_expr(n->right, locals, outer_env, consteval_fns,
                            consteval_fns_by_text, consteval_fns_by_key, depth);
      if (!lr.ok()) return lr;
      if (!rr.ok()) return rr;
      long long l = lr.as_int(), r = rr.as_int();
      long long result;
      if (std::strcmp(n->op, "+") == 0) result = l + r;
      else if (std::strcmp(n->op, "-") == 0) result = l - r;
      else if (std::strcmp(n->op, "*") == 0) result = l * r;
      else if (std::strcmp(n->op, "/") == 0) {
        if (r == 0) return ConstEvalResult::failure("division by zero in consteval context");
        result = l / r;
      } else if (std::strcmp(n->op, "%") == 0) {
        if (r == 0) return ConstEvalResult::failure("modulo by zero in consteval context");
        result = l % r;
      }
      else if (std::strcmp(n->op, "<<") == 0) result = l << r;
      else if (std::strcmp(n->op, ">>") == 0) result = l >> r;
      else if (std::strcmp(n->op, "&") == 0) result = l & r;
      else if (std::strcmp(n->op, "|") == 0) result = l | r;
      else if (std::strcmp(n->op, "^") == 0) result = l ^ r;
      else if (std::strcmp(n->op, "<") == 0) result = static_cast<long long>(l < r);
      else if (std::strcmp(n->op, "<=") == 0) result = static_cast<long long>(l <= r);
      else if (std::strcmp(n->op, ">") == 0) result = static_cast<long long>(l > r);
      else if (std::strcmp(n->op, ">=") == 0) result = static_cast<long long>(l >= r);
      else if (std::strcmp(n->op, "==") == 0) result = static_cast<long long>(l == r);
      else if (std::strcmp(n->op, "!=") == 0) result = static_cast<long long>(l != r);
      else return ConstEvalResult::failure(
          std::string("unsupported binary operator '") + n->op + "' in consteval context");
      return ConstEvalResult::success(ConstValue::make_int(result));
    }

    case NK_TERNARY: {
      auto cr = interp_expr(n->cond ? n->cond : n->left, locals, outer_env, consteval_fns,
                            consteval_fns_by_text, consteval_fns_by_key, depth);
      if (!cr.ok()) return cr;
      return interp_expr(cr.as_int() ? n->then_ : n->else_, locals, outer_env, consteval_fns,
                         consteval_fns_by_text, consteval_fns_by_key, depth);
    }

    case NK_CALL: {
      // Check if the callee is a consteval function we can interpret.
      if (!n->left || n->left->kind != NK_VAR || !n->left->name)
        return ConstEvalResult::failure("non-trivial callee in consteval context");
      const Node* callee_def = lookup_consteval_function(
          n->left, consteval_fns, consteval_fns_by_text, consteval_fns_by_key);
      if (!callee_def)
        return ConstEvalResult::failure(
            std::string("call to non-consteval function '") + n->left->name +
            "' is not allowed in constant evaluation");

      // Evaluate all arguments.
      std::vector<ConstValue> args;
      for (int i = 0; i < n->n_children; ++i) {
        auto r = interp_expr(n->children[i], locals, outer_env, consteval_fns,
                             consteval_fns_by_text, consteval_fns_by_key, depth);
        if (!r.ok())
          return ConstEvalResult::failure(
              std::string("argument ") + std::to_string(i) +
              " of call to '" + n->left->name + "' is not a constant expression" +
              (r.error.empty() ? "" : ": " + r.error));
        args.push_back(*r.value);
      }

      TypeBindings tpl_bindings;
      TypeBindingTextMap tpl_bindings_by_text;
      TypeBindingStructuredMap tpl_bindings_by_key;
      TypeBindingNameTextMap tpl_binding_text_ids_by_name;
      TypeBindingNameStructuredMap tpl_binding_keys_by_name;
      std::unordered_map<std::string, long long> nttp_bindings;
      ConstTextMap nttp_bindings_by_text;
      ConstStructuredMap nttp_bindings_by_key;
      ConstEvalEnv call_env = bind_consteval_call_env(
          n->left, callee_def, outer_env, &tpl_bindings, &nttp_bindings,
          &tpl_bindings_by_text, &tpl_bindings_by_key,
          &tpl_binding_text_ids_by_name, &tpl_binding_keys_by_name,
          &nttp_bindings_by_text, &nttp_bindings_by_key);
      return evaluate_consteval_call(callee_def, args, call_env, consteval_fns, depth + 1,
                                     consteval_fns_by_text, consteval_fns_by_key);
    }

    case NK_ASSIGN: {
      // left = right: evaluate right, assign to left (must be NK_VAR).
      if (!n->left || !n->right || n->left->kind != NK_VAR || !n->left->name)
        return ConstEvalResult::failure("unsupported assignment target in consteval context");
      auto r = interp_expr(n->right, locals, outer_env, consteval_fns,
                           consteval_fns_by_text, consteval_fns_by_key, depth);
      if (!r.ok()) return r;
      locals.bind(n->left, r.as_int());
      return r;
    }

    case NK_COMMA_EXPR: {
      // left, right — evaluate both, return right.
      auto left = interp_expr(n->left, locals, outer_env, consteval_fns,
                              consteval_fns_by_text, consteval_fns_by_key, depth);
      if (!left.ok()) return left;
      return interp_expr(n->right, locals, outer_env, consteval_fns,
                         consteval_fns_by_text, consteval_fns_by_key, depth);
    }

    case NK_SIZEOF_TYPE:
      return compute_sizeof_type(resolve_type(n->type, env), env);
    case NK_SIZEOF_EXPR:
      if (n->left) return compute_sizeof_type(resolve_type(n->left->type, env), env);
      return ConstEvalResult::failure("sizeof(expr): missing expression");
    case NK_SIZEOF_PACK:
      return ConstEvalResult::failure("sizeof...(pack) requires template pack instantiation");
    case NK_ALIGNOF_TYPE:
      return compute_alignof_type(resolve_type(n->type, env), env);
    case NK_ALIGNOF_EXPR:
      if (n->left) return compute_alignof_type(resolve_type(n->left->type, env), env);
      return ConstEvalResult::failure("alignof(expr): missing expression");

    default:
      return ConstEvalResult::failure(
          std::string("unsupported expression kind (NK=") +
          std::to_string(static_cast<int>(n->kind)) + ") in consteval body");
  }
}

StmtResult interp_block(const Node* block, InterpreterBindings& locals,
                        const ConstEvalEnv& outer_env,
                        const std::unordered_map<std::string, const Node*>& consteval_fns,
                        const ConstEvalFunctionTextMap* consteval_fns_by_text,
                        const ConstEvalFunctionStructuredMap* consteval_fns_by_key,
                        int depth, int& steps) {
  if (!block) return {};
  if (block->kind == NK_BLOCK) {
    const bool new_scope = (block->ival != 1);
    std::unordered_map<std::string, InterpreterBindingSnapshot> shadowed;
    for (int i = 0; i < block->n_children; ++i) {
      if (new_scope) {
        const Node* child = block->children[i];
        if (child && child->kind == NK_DECL && child->name && !shadowed.count(child->name)) {
          shadowed.emplace(child->name, locals.snapshot(child));
        }
      }
      auto r = interp_stmt(block->children[i], locals, outer_env, consteval_fns,
                           consteval_fns_by_text, consteval_fns_by_key, depth, steps);
      if (r.returned || r.did_break || r.did_continue) {
        if (new_scope) {
          for (const auto& [name, old] : shadowed) {
            (void)name;
            locals.restore(old);
          }
        }
        return r;
      }
    }
    if (new_scope) {
      for (const auto& [name, old] : shadowed) {
        (void)name;
        locals.restore(old);
      }
    }
    return {};
  }
  // Single statement (not wrapped in block).
  return interp_stmt(block, locals, outer_env, consteval_fns,
                     consteval_fns_by_text, consteval_fns_by_key, depth, steps);
}

StmtResult interp_stmt(const Node* n, InterpreterBindings& locals,
                        const ConstEvalEnv& outer_env,
                        const std::unordered_map<std::string, const Node*>& consteval_fns,
                        const ConstEvalFunctionTextMap* consteval_fns_by_text,
                        const ConstEvalFunctionStructuredMap* consteval_fns_by_key,
                        int depth, int& steps) {
  if (!n) return {};
  if (++steps > kMaxConstevalSteps)
    return {true, false, false, ConstValue::unknown(), "step limit exceeded in consteval evaluation"};

  switch (n->kind) {
    case NK_RETURN: {
      if (!n->left) return {true, false, false, ConstValue::make_int(0)};
      auto r = interp_expr(n->left, locals, outer_env, consteval_fns,
                           consteval_fns_by_text, consteval_fns_by_key, depth);
      if (!r.ok())
        return {true, false, false, ConstValue::unknown(),
                "return expression is not a constant expression" +
                (r.error.empty() ? std::string{} : ": " + r.error)};
      return {true, false, false, *r.value};
    }

    case NK_DECL: {
      // Local variable declaration — mutable or const with initializer.
      if (n->name && n->init) {
        auto r = interp_expr(n->init, locals, outer_env, consteval_fns,
                             consteval_fns_by_text, consteval_fns_by_key, depth);
        if (r.ok()) {
          locals.bind(n, r.as_int());
        } else {
          return {true, false, false, ConstValue::unknown(),
                  std::string("initializer of '") + n->name +
                  "' is not a constant expression" +
                  (r.error.empty() ? std::string{} : ": " + r.error)};
        }
      } else if (n->name) {
        // Declaration without initializer — default to 0.
        locals.bind(n, 0);
      }
      return {};
    }

    case NK_IF: {
      auto cr = interp_expr(n->cond, locals, outer_env, consteval_fns,
                            consteval_fns_by_text, consteval_fns_by_key, depth);
      if (!cr.ok())
        return {true, false, false, ConstValue::unknown(),
                "if condition is not a constant expression" +
                (cr.error.empty() ? std::string{} : ": " + cr.error)};
      if (cr.as_int()) {
        return interp_block(n->then_, locals, outer_env, consteval_fns,
                            consteval_fns_by_text, consteval_fns_by_key, depth, steps);
      } else if (n->else_) {
        return interp_block(n->else_, locals, outer_env, consteval_fns,
                            consteval_fns_by_text, consteval_fns_by_key, depth, steps);
      }
      return {};
    }

    case NK_WHILE: {
      while (true) {
        auto cr = interp_expr(n->cond, locals, outer_env, consteval_fns,
                              consteval_fns_by_text, consteval_fns_by_key, depth);
        if (!cr.ok())
          return {true, false, false, ConstValue::unknown(),
                  "while condition is not a constant expression" +
                  (cr.error.empty() ? std::string{} : ": " + cr.error)};
        if (!cr.as_int()) break;
        auto r = interp_block(n->body, locals, outer_env, consteval_fns,
                              consteval_fns_by_text, consteval_fns_by_key, depth, steps);
        if (r.returned) return r;
        if (r.did_break) break;
        // did_continue: just continue the loop
        if (++steps > kMaxConstevalSteps)
          return {true, false, false, ConstValue::unknown(), "step limit exceeded in consteval evaluation"};
      }
      return {};
    }

    case NK_FOR: {
      // init
      if (n->init) {
        auto r = interp_stmt(n->init, locals, outer_env, consteval_fns,
                             consteval_fns_by_text, consteval_fns_by_key, depth, steps);
        if (r.returned) return r;
      }
      while (true) {
        // cond
        if (n->cond) {
          auto cr = interp_expr(n->cond, locals, outer_env, consteval_fns,
                                consteval_fns_by_text, consteval_fns_by_key, depth);
          if (!cr.ok())
            return {true, false, false, ConstValue::unknown(),
                    "for condition is not a constant expression" +
                    (cr.error.empty() ? std::string{} : ": " + cr.error)};
          if (!cr.as_int()) break;
        }
        // body
        auto r = interp_block(n->body, locals, outer_env, consteval_fns,
                              consteval_fns_by_text, consteval_fns_by_key, depth, steps);
        if (r.returned) return r;
        if (r.did_break) break;
        // update
        if (n->update) {
          auto ur = interp_expr(n->update, locals, outer_env, consteval_fns,
                                consteval_fns_by_text, consteval_fns_by_key, depth);
          if (!ur.ok())
            return {true, false, false, ConstValue::unknown(),
                    "for update is not a constant expression" +
                    (ur.error.empty() ? std::string{} : ": " + ur.error)};
        }
        if (++steps > kMaxConstevalSteps)
          return {true, false, false, ConstValue::unknown(), "step limit exceeded in consteval evaluation"};
      }
      return {};
    }

    case NK_RANGE_FOR:
      return {true, false, false, ConstValue::unknown(),
              "range-for is not supported in consteval context"};

    case NK_DO_WHILE: {
      do {
        auto r = interp_block(n->body, locals, outer_env, consteval_fns,
                              consteval_fns_by_text, consteval_fns_by_key, depth, steps);
        if (r.returned) return r;
        if (r.did_break) break;
        auto cr = interp_expr(n->cond, locals, outer_env, consteval_fns,
                              consteval_fns_by_text, consteval_fns_by_key, depth);
        if (!cr.ok())
          return {true, false, false, ConstValue::unknown(),
                  "do-while condition is not a constant expression" +
                  (cr.error.empty() ? std::string{} : ": " + cr.error)};
        if (!cr.as_int()) break;
        if (++steps > kMaxConstevalSteps)
          return {true, false, false, ConstValue::unknown(), "step limit exceeded in consteval evaluation"};
      } while (true);
      return {};
    }

    case NK_EXPR_STMT: {
      // Expression statement: evaluate the expression for side effects.
      if (n->left) {
        auto r = interp_expr(n->left, locals, outer_env, consteval_fns,
                             consteval_fns_by_text, consteval_fns_by_key, depth);
        if (!r.ok())
          return {true, false, false, ConstValue::unknown(),
                  "expression statement is not a constant expression" +
                  (r.error.empty() ? std::string{} : ": " + r.error)};
      }
      return {};
    }

    case NK_BREAK:
      return {false, true, false, ConstValue::unknown()};

    case NK_CONTINUE:
      return {false, false, true, ConstValue::unknown()};

    case NK_BLOCK:
      return interp_block(n, locals, outer_env, consteval_fns,
                          consteval_fns_by_text, consteval_fns_by_key, depth, steps);

    default:
      // Expression used as statement (e.g., assignment in for-init) — evaluate for side effects.
      if (auto r = interp_expr(n, locals, outer_env, consteval_fns,
                               consteval_fns_by_text, consteval_fns_by_key, depth); !r.ok()) {
        return {true, false, false, ConstValue::unknown(),
                "statement is not a constant expression" +
                (r.error.empty() ? std::string{} : ": " + r.error)};
      }
      return {};
  }
}

}  // namespace

ConstEvalResult evaluate_consteval_call(
    const Node* func_def,
    const std::vector<ConstValue>& args,
    const ConstEvalEnv& env,
    const std::unordered_map<std::string, const Node*>& consteval_fns,
    int depth,
    const ConstEvalFunctionTextMap* consteval_fns_by_text,
    const ConstEvalFunctionStructuredMap* consteval_fns_by_key) {
  if (!func_def || func_def->kind != NK_FUNCTION)
    return ConstEvalResult::failure("consteval target is not a function definition");
  if (depth > kMaxConstevalDepth)
    return ConstEvalResult::failure("consteval recursion depth limit exceeded");

  // Build local bindings from parameters.
  InterpreterBindings locals;
  const int n_params = func_def->n_params;
  for (int i = 0; i < n_params && i < static_cast<int>(args.size()); ++i) {
    const Node* p = func_def->params[i];
    if (p && p->name && args[i].is_known()) {
      locals.bind(p, args[i].as_int());
    }
  }

  // Interpret the body.
  int steps = 0;
  auto result = interp_block(func_def->body, locals, env, consteval_fns,
                             consteval_fns_by_text, consteval_fns_by_key, depth, steps);
  if (result.returned && result.return_val.is_known()) {
    return ConstEvalResult::success(result.return_val);
  }
  // Propagate the error from the statement result if available.
  std::string err = result.error;
  if (err.empty()) {
    if (!result.returned)
      err = "consteval function did not return a value";
    else
      err = "consteval function returned a non-constant value";
  }
  const char* fname = func_def->name ? func_def->name : "<anonymous>";
  return ConstEvalResult::failure(
      std::string("in consteval function '") + fname + "': " + err);
}

// ── String literal helpers ───────────────────────────────────────────────────

std::vector<long long> decode_string_literal_values(const char* sval, bool wide) {
  std::vector<long long> out;
  if (!sval) {
    out.push_back(0);
    return out;
  }
  const char* p = sval;
  while (*p && *p != '"') ++p;
  if (*p == '"') ++p;
  while (*p && *p != '"') {
    if (*p == '\\' && *(p + 1)) {
      ++p;
      switch (*p) {
        case 'n': out.push_back('\n'); ++p; break;
        case 't': out.push_back('\t'); ++p; break;
        case 'r': out.push_back('\r'); ++p; break;
        case '0': out.push_back(0); ++p; break;
        case '\\': out.push_back('\\'); ++p; break;
        case '\'': out.push_back('\''); ++p; break;
        case '"': out.push_back('"'); ++p; break;
        default: out.push_back(static_cast<unsigned char>(*p)); ++p; break;
      }
      continue;
    }

    const unsigned char c0 = static_cast<unsigned char>(*p);
    if (!wide || c0 < 0x80) {
      out.push_back(static_cast<long long>(c0));
      ++p;
      continue;
    }
    if ((c0 & 0xE0) == 0xC0 && p[1]) {
      const uint32_t cp = ((c0 & 0x1F) << 6) | (static_cast<unsigned char>(p[1]) & 0x3F);
      out.push_back(static_cast<long long>(cp));
      p += 2;
      continue;
    }
    if ((c0 & 0xF0) == 0xE0 && p[1] && p[2]) {
      const uint32_t cp = ((c0 & 0x0F) << 12) |
                          ((static_cast<unsigned char>(p[1]) & 0x3F) << 6) |
                          (static_cast<unsigned char>(p[2]) & 0x3F);
      out.push_back(static_cast<long long>(cp));
      p += 3;
      continue;
    }
    if ((c0 & 0xF8) == 0xF0 && p[1] && p[2] && p[3]) {
      const uint32_t cp = ((c0 & 0x07) << 18) |
                          ((static_cast<unsigned char>(p[1]) & 0x3F) << 12) |
                          ((static_cast<unsigned char>(p[2]) & 0x3F) << 6) |
                          (static_cast<unsigned char>(p[3]) & 0x3F);
      out.push_back(static_cast<long long>(cp));
      p += 4;
      continue;
    }
    out.push_back(static_cast<long long>(c0));
    ++p;
  }
  out.push_back(0);
  return out;
}

std::string bytes_from_string_literal(const StringLiteral& sl) {
  std::string bytes = sl.raw;
  if (bytes.size() >= 2 && bytes.front() == '"' && bytes.back() == '"') {
    bytes = bytes.substr(1, bytes.size() - 2);
  } else if (bytes.size() >= 3 && bytes[0] == 'L' && bytes[1] == '"' && bytes.back() == '"') {
    bytes = bytes.substr(2, bytes.size() - 3);
  }
  return decode_c_escaped_bytes_local(bytes);
}

}  // namespace c4c::hir
