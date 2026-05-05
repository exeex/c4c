#pragma once

// Public compile-time retry / realization engine contract for HIR.
//
// This header is intentionally retained as an app-facing and pipeline-facing
// shim: c4cll dump/emission paths and the HIR pipeline call the compile-time
// reduction and materialization APIs directly. Implementation files should
// include impl/compile_time/compile_time.hpp instead.
//
// Initial HIR lowering is allowed to leave some work deferred when full
// template/dependent information is not yet ready. This engine owns the
// follow-up loop for that deferred work, including:
// - deferred template instantiation retries
// - deferred consteval reductions unlocked by new instantiations
// - deferred NTTP-related work and dependent template-type realization
//
// Conceptually, HIR acts as a second semantic layer for higher-level C++
// constructs, and compile_time_engine is the fixpoint driver that materializes
// those deferred compile-time semantics into concrete HIR state.

#include "hir_ir.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::hir {

// ── Template instantiation types ────────────────────────────────────────────

/// Origin of a template seed — tracks how the instantiation was discovered.
enum class TemplateSeedOrigin {
  DirectCall,
  EnclosingTemplateExpansion,
  DeducedCall,
  ConstevalSeed,
  ConstevalEnclosingExpansion,
};

/// A discovered template application candidate (todo list item).
struct TemplateSeedWorkItem {
  std::string template_name;
  TypeBindings bindings;
  NttpBindings nttp_bindings;
  NttpTextBindings nttp_bindings_by_text;
  std::string mangled_name;
  SpecializationKey spec_key;
  TemplateSeedOrigin origin = TemplateSeedOrigin::DirectCall;
  const Node* primary_def = nullptr;  // structured identity (when available)
};

/// A realized template instantiation (authoritative for lowering).
struct TemplateInstance {
  TypeBindings bindings;
  NttpBindings nttp_bindings;  // non-type template param → constant value
  NttpTextBindings nttp_bindings_by_text;  // non-type template param TextId → value
  std::string mangled_name;
  SpecializationKey spec_key;  // stable identity for dedup/caching
  const Node* primary_def = nullptr;  // structured identity (when available)
};

struct TemplateStructEnv {
  const Node* primary_def = nullptr;
  const std::vector<const Node*>* specialization_patterns = nullptr;
};

struct SelectedTemplateStructPattern {
  const Node* primary_def = nullptr;
  const Node* selected_pattern = nullptr;
  TypeBindings type_bindings;
  NttpBindings nttp_bindings;
};

struct TemplateStructInstanceKey {
  const Node* primary_def = nullptr;
  SpecializationKey spec_key;

  bool operator==(const TemplateStructInstanceKey& other) const {
    return primary_def == other.primary_def && spec_key == other.spec_key;
  }
};

struct TemplateStructInstanceKeyHash {
  std::size_t operator()(const TemplateStructInstanceKey& key) const noexcept {
    std::size_t h1 = std::hash<const Node*>{}(key.primary_def);
    std::size_t h2 = SpecializationKeyHash{}(key.spec_key);
    return h1 ^ (h2 + 0x9e3779b9u + (h1 << 6) + (h1 >> 2));
  }
};

// ── Structured function-template identity types ─────────────────────────────

struct FunctionTemplateEnv {
  const Node* primary_def = nullptr;
  const std::vector<const Node*>* specialization_patterns = nullptr;
};

struct SelectedFunctionTemplatePattern {
  const Node* primary_def = nullptr;
  const Node* selected_pattern = nullptr;  // == primary_def if no spec matched
  TypeBindings type_bindings;
  NttpBindings nttp_bindings;
  SpecializationKey spec_key;
};

struct FunctionTemplateInstanceKey {
  const Node* primary_def = nullptr;
  SpecializationKey spec_key;

  bool operator==(const FunctionTemplateInstanceKey& other) const {
    return primary_def == other.primary_def && spec_key == other.spec_key;
  }
};

struct FunctionTemplateInstanceKeyHash {
  std::size_t operator()(const FunctionTemplateInstanceKey& key) const noexcept {
    std::size_t h1 = std::hash<const Node*>{}(key.primary_def);
    std::size_t h2 = SpecializationKeyHash{}(key.spec_key);
    return h1 ^ (h2 + 0x9e3779b9u + (h1 << 6) + (h1 >> 2));
  }
};

// ── Compile-time definition registry identity ───────────────────────────────

enum class CompileTimeRegistryKeyKind : uint8_t {
  TemplateFunction,
  PrimaryTemplateStruct,
  TemplateStructSpecializationOwner,
  ConstevalFunction,
};

struct CompileTimeRegistryKey {
  CompileTimeRegistryKeyKind registry_kind =
      CompileTimeRegistryKeyKind::TemplateFunction;
  NodeKind declaration_kind = NK_FUNCTION;
  int namespace_context_id = -1;
  bool is_global_qualified = false;
  std::vector<TextId> qualifier_segment_text_ids;
  TextId unqualified_text_id = kInvalidText;
  const Node* declaration_fallback = nullptr;

  [[nodiscard]] bool operator==(const CompileTimeRegistryKey& other) const {
    return registry_kind == other.registry_kind &&
           declaration_kind == other.declaration_kind &&
           namespace_context_id == other.namespace_context_id &&
           is_global_qualified == other.is_global_qualified &&
           qualifier_segment_text_ids == other.qualifier_segment_text_ids &&
           unqualified_text_id == other.unqualified_text_id &&
           declaration_fallback == other.declaration_fallback;
  }
};

struct CompileTimeRegistryKeyHash {
  [[nodiscard]] std::size_t operator()(
      const CompileTimeRegistryKey& key) const noexcept {
    std::size_t h = std::hash<int>{}(static_cast<int>(key.registry_kind));
    auto mix = [&](std::size_t part) {
      h ^= part + 0x9e3779b9u + (h << 6) + (h >> 2);
    };
    mix(std::hash<int>{}(static_cast<int>(key.declaration_kind)));
    mix(std::hash<int>{}(key.namespace_context_id));
    mix(std::hash<bool>{}(key.is_global_qualified));
    mix(std::hash<TextId>{}(key.unqualified_text_id));
    for (TextId segment : key.qualifier_segment_text_ids) {
      mix(std::hash<TextId>{}(segment));
    }
    mix(std::hash<const Node*>{}(key.declaration_fallback));
    return h;
  }
};

inline std::optional<CompileTimeRegistryKey> make_compile_time_registry_key(
    CompileTimeRegistryKeyKind registry_kind, const Node* declaration) {
  if (!declaration) return std::nullopt;

  CompileTimeRegistryKey key;
  key.registry_kind = registry_kind;
  key.declaration_kind = declaration->kind;
  key.namespace_context_id = declaration->namespace_context_id;
  key.is_global_qualified = declaration->is_global_qualified;
  key.unqualified_text_id = declaration->unqualified_text_id;

  bool complete_text_identity = key.unqualified_text_id != kInvalidText;
  const int qualifier_count = std::max(0, declaration->n_qualifier_segments);
  key.qualifier_segment_text_ids.reserve(
      static_cast<std::size_t>(qualifier_count));
  for (int i = 0; i < qualifier_count; ++i) {
    TextId segment_text_id = kInvalidText;
    if (declaration->qualifier_text_ids) {
      segment_text_id = declaration->qualifier_text_ids[i];
    }
    if (segment_text_id == kInvalidText) complete_text_identity = false;
    key.qualifier_segment_text_ids.push_back(segment_text_id);
  }

  if (!complete_text_identity) {
    key.declaration_fallback = declaration;
  }
  return key;
}

// ── Structured compile-time value-binding identity types ────────────────────

enum class CompileTimeValueBindingKeyKind : uint8_t {
  GlobalEnumConstant,
  GlobalConstInt,
};

struct CompileTimeValueBindingKey {
  CompileTimeValueBindingKeyKind binding_kind =
      CompileTimeValueBindingKeyKind::GlobalEnumConstant;
  NodeKind declaration_kind = NK_VAR;
  int namespace_context_id = -1;
  bool is_global_qualified = false;
  std::vector<TextId> qualifier_segment_text_ids;
  TextId unqualified_text_id = kInvalidText;

  [[nodiscard]] bool valid() const {
    return namespace_context_id >= 0 && unqualified_text_id != kInvalidText;
  }

  [[nodiscard]] bool operator==(
      const CompileTimeValueBindingKey& other) const {
    return binding_kind == other.binding_kind &&
           declaration_kind == other.declaration_kind &&
           namespace_context_id == other.namespace_context_id &&
           is_global_qualified == other.is_global_qualified &&
           qualifier_segment_text_ids == other.qualifier_segment_text_ids &&
           unqualified_text_id == other.unqualified_text_id;
  }
};

struct CompileTimeValueBindingKeyHash {
  [[nodiscard]] std::size_t operator()(
      const CompileTimeValueBindingKey& key) const noexcept {
    std::size_t h = std::hash<int>{}(static_cast<int>(key.binding_kind));
    auto mix = [&](std::size_t part) {
      h ^= part + 0x9e3779b9u + (h << 6) + (h >> 2);
    };
    mix(std::hash<int>{}(static_cast<int>(key.declaration_kind)));
    mix(std::hash<int>{}(key.namespace_context_id));
    mix(std::hash<bool>{}(key.is_global_qualified));
    mix(std::hash<TextId>{}(key.unqualified_text_id));
    for (TextId segment : key.qualifier_segment_text_ids) {
      mix(std::hash<TextId>{}(segment));
    }
    return h;
  }
};

inline std::optional<CompileTimeValueBindingKey>
make_global_const_int_value_binding_key(const Node* declaration) {
  if (!declaration || declaration->namespace_context_id < 0 ||
      declaration->unqualified_text_id == kInvalidText) {
    return std::nullopt;
  }

  CompileTimeValueBindingKey key;
  key.binding_kind = CompileTimeValueBindingKeyKind::GlobalConstInt;
  key.declaration_kind = declaration->kind;
  key.namespace_context_id = declaration->namespace_context_id;
  key.is_global_qualified = declaration->is_global_qualified;
  key.unqualified_text_id = declaration->unqualified_text_id;

  const int qualifier_count = std::max(0, declaration->n_qualifier_segments);
  key.qualifier_segment_text_ids.reserve(
      static_cast<std::size_t>(qualifier_count));
  for (int i = 0; i < qualifier_count; ++i) {
    if (!declaration->qualifier_text_ids ||
        declaration->qualifier_text_ids[i] == kInvalidText) {
      return std::nullopt;
    }
    key.qualifier_segment_text_ids.push_back(declaration->qualifier_text_ids[i]);
  }
  return key;
}

inline std::optional<CompileTimeValueBindingKey>
make_global_enum_const_value_binding_key(const Node* enum_def,
                                         int variant_index) {
  if (!enum_def || enum_def->kind != NK_ENUM_DEF ||
      enum_def->namespace_context_id < 0 || variant_index < 0 ||
      variant_index >= enum_def->n_enum_variants ||
      !enum_def->enum_name_text_ids) {
    return std::nullopt;
  }
  const TextId text_id = enum_def->enum_name_text_ids[variant_index];
  if (text_id == kInvalidText) return std::nullopt;

  CompileTimeValueBindingKey key;
  key.binding_kind = CompileTimeValueBindingKeyKind::GlobalEnumConstant;
  key.declaration_kind = enum_def->kind;
  key.namespace_context_id = enum_def->namespace_context_id;
  key.unqualified_text_id = text_id;
  return key;
}

enum class PendingTemplateTypeKind {
  DeclarationType,
  OwnerStruct,
  MemberTypedef,
  BaseType,
  CastTarget,
};

struct PendingTemplateTypeWorkItem {
  PendingTemplateTypeKind kind = PendingTemplateTypeKind::DeclarationType;
  TypeSpec pending_type{};
  const Node* owner_primary_def = nullptr;
  TypeBindings type_bindings;
  NttpBindings nttp_bindings;
  SourceSpan span{};
  std::string context_name;
  std::string dedup_key;
};

enum class DeferredTemplateTypeResultKind {
  Resolved,
  Blocked,
  Terminal,
};

struct DeferredTemplateTypeResult {
  DeferredTemplateTypeResultKind kind =
      DeferredTemplateTypeResultKind::Blocked;
  bool spawned_new_work = false;
  std::string diagnostic;

  static DeferredTemplateTypeResult resolved() {
    return {DeferredTemplateTypeResultKind::Resolved, false, {}};
  }

  static DeferredTemplateTypeResult blocked(
      bool spawned_new_work = false,
      std::string diagnostic = {}) {
    return {DeferredTemplateTypeResultKind::Blocked,
            spawned_new_work, std::move(diagnostic)};
  }

  static DeferredTemplateTypeResult terminal(
      std::string diagnostic = {}) {
    return {DeferredTemplateTypeResultKind::Terminal, false,
            std::move(diagnostic)};
  }
};

// ── Template mangling utilities ─────────────────────────────────────────────

inline std::string nominal_type_suffix_for_mangling(const TypeSpec& ts) {
  std::function<bool(const TypeSpec&)> has_unresolved_template_param_arg;
  has_unresolved_template_param_arg = [&](const TypeSpec& arg) {
    if (arg.template_param_text_id != kInvalidText ||
        arg.template_param_owner_text_id != kInvalidText ||
        arg.template_param_index >= 0) {
      return true;
    }
    if (arg.tag_text_id != kInvalidText && !arg.record_def &&
        !(arg.tpl_struct_origin && arg.tpl_struct_origin[0])) {
      return true;
    }
    if (arg.tpl_struct_origin && arg.tpl_struct_origin[0] &&
        arg.tpl_struct_args.data && arg.tpl_struct_args.size > 0) {
      for (int i = 0; i < arg.tpl_struct_args.size; ++i) {
        const TemplateArgRef& nested = arg.tpl_struct_args.data[i];
        if (nested.kind == TemplateArgKind::Type &&
            has_unresolved_template_param_arg(nested.type)) {
          return true;
        }
      }
    }
    return false;
  };
  bool can_use_origin_carrier = ts.tpl_struct_origin && ts.tpl_struct_origin[0] &&
                                ts.tpl_struct_args.data &&
                                ts.tpl_struct_args.size > 0;
  if (can_use_origin_carrier) {
    for (int i = 0; i < ts.tpl_struct_args.size; ++i) {
      const TemplateArgRef& arg = ts.tpl_struct_args.data[i];
      if (arg.kind == TemplateArgKind::Type &&
          has_unresolved_template_param_arg(arg.type)) {
        can_use_origin_carrier = false;
        break;
      }
    }
  }
  if (can_use_origin_carrier) {
    auto append_type_name = [](std::string& out, const TypeSpec& arg) {
      if (arg.is_const) out += "const_";
      if (arg.is_volatile) out += "volatile_";
      switch (arg.base) {
        case TB_INT: out += "int"; break;
        case TB_UINT: out += "uint"; break;
        case TB_CHAR: out += "char"; break;
        case TB_SCHAR: out += "schar"; break;
        case TB_UCHAR: out += "uchar"; break;
        case TB_SHORT: out += "short"; break;
        case TB_USHORT: out += "ushort"; break;
        case TB_LONG: out += "long"; break;
        case TB_ULONG: out += "ulong"; break;
        case TB_LONGLONG: out += "llong"; break;
        case TB_ULONGLONG: out += "ullong"; break;
        case TB_FLOAT: out += "float"; break;
        case TB_DOUBLE: out += "double"; break;
        case TB_LONGDOUBLE: out += "ldouble"; break;
        case TB_VOID: out += "void"; break;
        case TB_BOOL: out += "bool"; break;
        case TB_INT128: out += "i128"; break;
        case TB_UINT128: out += "u128"; break;
        case TB_STRUCT:
        case TB_UNION:
        case TB_ENUM:
        case TB_TYPEDEF:
          out += nominal_type_suffix_for_mangling(arg).substr(1);
          break;
        default:
          out += "unknown";
          break;
      }
      for (int i = 0; i < arg.ptr_level; ++i) out += "_ptr";
      if (arg.is_lvalue_ref) out += "_ref";
      if (arg.is_rvalue_ref) out += "_rref";
    };
    std::string out = "T";
    out += ts.tpl_struct_origin;
    for (int i = 0; i < ts.tpl_struct_args.size; ++i) {
      const TemplateArgRef& arg = ts.tpl_struct_args.data[i];
      out += "_";
      if (arg.kind == TemplateArgKind::Value) {
        out += std::to_string(arg.value);
      } else {
        out += "T_";
        append_type_name(out, arg.type);
      }
    }
    return out;
  }
  if (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) {
    if (ts.record_def->name && ts.record_def->name[0]) {
      return std::string("T") + ts.record_def->name;
    }
    if (ts.record_def->unqualified_name && ts.record_def->unqualified_name[0]) {
      return std::string("T") + ts.record_def->unqualified_name;
    }
    if (ts.record_def->unqualified_text_id != kInvalidText) {
      return "Trecord_ctx" +
             std::to_string(ts.record_def->namespace_context_id) + "_text" +
             std::to_string(ts.record_def->unqualified_text_id);
    }
  }
  if (ts.tag_text_id != kInvalidText) {
    std::string out = "Ttag_ctx" + std::to_string(ts.namespace_context_id);
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
  if (ts.template_param_text_id != kInvalidText) {
    return "Tparam_owner_ctx" +
           std::to_string(ts.template_param_owner_namespace_context_id) +
           "_owner" + std::to_string(ts.template_param_owner_text_id) +
           "_index" + std::to_string(ts.template_param_index) + "_text" +
           std::to_string(ts.template_param_text_id);
  }
  if (ts.deferred_member_type_text_id != kInvalidText) {
    return "Tdeferred_text" +
           std::to_string(ts.deferred_member_type_text_id);
  }
  if (ts.tpl_struct_origin_key.base_text_id != kInvalidText) {
    return "Ttpl_origin_ctx" +
           std::to_string(ts.tpl_struct_origin_key.context_id) +
           (ts.tpl_struct_origin_key.is_global_qualified ? "_global" : "_local") +
           "_qpath" +
           std::to_string(ts.tpl_struct_origin_key.qualifier_path_id) + "_text" +
           std::to_string(ts.tpl_struct_origin_key.base_text_id);
  }
  return "unknown";
}

/// Produce a deterministic type suffix for name mangling.
inline std::string type_suffix_for_mangling(const TypeSpec& ts) {
  std::string out;
  if (ts.is_const) out += "c_";
  if (ts.is_volatile) out += "v_";
  if (ts.ptr_level > 0) out += "p" + std::to_string(ts.ptr_level);
  switch (ts.base) {
    case TB_BOOL: out += "b"; break;
    case TB_CHAR: case TB_SCHAR: out += "c"; break;
    case TB_UCHAR: out += "uc"; break;
    case TB_SHORT: out += "s"; break;
    case TB_USHORT: out += "us"; break;
    case TB_INT: out += "i"; break;
    case TB_UINT: out += "ui"; break;
    case TB_LONG: out += "l"; break;
    case TB_ULONG: out += "ul"; break;
    case TB_LONGLONG: out += "ll"; break;
    case TB_ULONGLONG: out += "ull"; break;
    case TB_FLOAT: out += "f"; break;
    case TB_DOUBLE: out += "d"; break;
    case TB_LONGDOUBLE: out += "ld"; break;
    case TB_INT128: out += "i128"; break;
    case TB_UINT128: out += "u128"; break;
    case TB_VOID: out += "v"; break;
    default:
      out += nominal_type_suffix_for_mangling(ts);
      break;
  }
  if (ts.is_lvalue_ref) out += "_ref";
  if (ts.is_rvalue_ref) out += "_rref";
  return out;
}

/// Check whether all type bindings are concrete (no unresolved typedefs).
inline bool bindings_are_concrete(const TypeBindings& bindings) {
  for (const auto& [param, ts] : bindings) {
    if (ts.base == TB_TYPEDEF) return false;
  }
  return true;
}

/// Build a mangled name from base name and template bindings.
inline std::string mangle_template_name(const std::string& base,
                                        const TypeBindings& bindings,
                                        const NttpBindings& nttp_bindings = {}) {
  if (bindings.empty() && nttp_bindings.empty()) return base;
  // Collect all param names for deterministic ordering.
  std::vector<std::string> all_params;
  for (const auto& [k, v] : bindings) all_params.push_back(k);
  for (const auto& [k, v] : nttp_bindings) all_params.push_back(k);
  std::sort(all_params.begin(), all_params.end());
  std::string result = base;
  for (const auto& param : all_params) {
    result += "_";
    auto nttp_it = nttp_bindings.find(param);
    if (nttp_it != nttp_bindings.end()) {
      // NTTP: encode value (use 'n' prefix for negatives).
      if (nttp_it->second < 0)
        result += "n" + std::to_string(-nttp_it->second);
      else
        result += std::to_string(nttp_it->second);
    } else {
      auto it = bindings.find(param);
      if (it != bindings.end()) result += type_suffix_for_mangling(it->second);
    }
  }
  return result;
}

inline constexpr const char* kDeferredNttpExprRefPrefix = "$expr:";

inline bool is_deferred_nttp_expr_ref(const std::string& ref) {
  return ref.rfind(kDeferredNttpExprRefPrefix, 0) == 0;
}

inline std::string deferred_nttp_expr_text(const std::string& ref) {
  if (!is_deferred_nttp_expr_ref(ref)) return {};
  return ref.substr(std::strlen(kDeferredNttpExprRefPrefix));
}

inline std::vector<std::string> split_deferred_template_arg_refs(
    const std::string& refs) {
  std::vector<std::string> parts;
  if (refs.empty()) return parts;
  size_t start = 0;
  int angle_depth = 0;
  int paren_depth = 0;
  int bracket_depth = 0;
  int brace_depth = 0;
  for (size_t i = 0; i < refs.size(); ++i) {
    const char ch = refs[i];
    switch (ch) {
      case '<': ++angle_depth; break;
      case '>': if (angle_depth > 0) --angle_depth; break;
      case '(': ++paren_depth; break;
      case ')': if (paren_depth > 0) --paren_depth; break;
      case '[': ++bracket_depth; break;
      case ']': if (bracket_depth > 0) --bracket_depth; break;
      case '{': ++brace_depth; break;
      case '}': if (brace_depth > 0) --brace_depth; break;
      case ',':
        if (angle_depth == 0 && paren_depth == 0 &&
            bracket_depth == 0 && brace_depth == 0) {
          parts.push_back(refs.substr(start, i - start));
          start = i + 1;
        }
        break;
      default:
        break;
    }
  }
  parts.push_back(refs.substr(start));
  return parts;
}

inline const char* pending_template_type_kind_name(PendingTemplateTypeKind kind) {
  switch (kind) {
    case PendingTemplateTypeKind::DeclarationType: return "declaration";
    case PendingTemplateTypeKind::OwnerStruct: return "owner-struct";
    case PendingTemplateTypeKind::MemberTypedef: return "member-typedef";
    case PendingTemplateTypeKind::BaseType: return "base-type";
    case PendingTemplateTypeKind::CastTarget: return "cast-target";
  }
  return "unknown";
}

inline std::string encode_pending_type_ref(const TypeSpec& ts) {
  auto has_structured_type_payload = [](const TypeSpec& type) {
    return type.tpl_struct_origin || type.base != TB_VOID || type.ptr_level > 0 ||
           type.array_rank > 0 || type.is_fn_ptr || type.is_vector;
  };
  std::function<std::string(const TemplateArgRef&)> encode_arg =
      [&](const TemplateArgRef& arg) -> std::string {
    if (arg.kind == TemplateArgKind::Value && arg.value == 0 &&
        arg.debug_text && arg.debug_text[0]) {
      return arg.debug_text;
    }
    if (arg.kind == TemplateArgKind::Value) {
      return std::string("v:") + std::to_string(arg.value);
    }
    if (arg.kind == TemplateArgKind::Type &&
        has_structured_type_payload(arg.type)) {
      return std::string("t:{") + encode_pending_type_ref(arg.type) + "}";
    }
    if (arg.debug_text && arg.debug_text[0]) return arg.debug_text;
    return "t:?";
  };
  std::string out;
  out += "base=" + std::to_string(static_cast<int>(ts.base));
  out += "|tag_text_id=" + std::to_string(ts.tag_text_id);
  out += "|template_param_text_id=" +
         std::to_string(ts.template_param_text_id);
  out += "|deferred_member_type_text_id=" +
         std::to_string(ts.deferred_member_type_text_id);
  out += "|record_def_text_id=" +
         std::to_string(ts.record_def ? ts.record_def->unqualified_text_id
                                      : kInvalidText);
  out += "|ptr=" + std::to_string(ts.ptr_level);
  out += "|arr=" + std::to_string(ts.array_rank);
  out += "|origin=";
  out += ts.tpl_struct_origin ? ts.tpl_struct_origin : "";
  out += "|args=";
  if (ts.tpl_struct_args.data && ts.tpl_struct_args.size > 0) {
    for (int i = 0; i < ts.tpl_struct_args.size; ++i) {
      if (i > 0) out += ",";
      out += encode_arg(ts.tpl_struct_args.data[i]);
    }
  }
  out += "|member=";
  out += ts.deferred_member_type_name ? ts.deferred_member_type_name : "";
  return out;
}

inline std::string make_pending_template_type_key(
    PendingTemplateTypeKind kind, const TypeSpec& pending_type,
    const Node* owner_primary_def,
    const TypeBindings& type_bindings, const NttpBindings& nttp_bindings,
    const std::string& context_name, const SourceSpan& span) {
  std::string key = pending_template_type_kind_name(kind);
  key += "|ctx=" + context_name;
  key += "|span=" + std::to_string(span.begin.line) + ":" +
         std::to_string(span.end.line);
  key += "|owner=";
  if (owner_primary_def && owner_primary_def->name) key += owner_primary_def->name;
  key += "|type=" + encode_pending_type_ref(pending_type);

  std::vector<std::string> tb_names;
  tb_names.reserve(type_bindings.size());
  for (const auto& [name, _] : type_bindings) tb_names.push_back(name);
  std::sort(tb_names.begin(), tb_names.end());
  key += "|tb=";
  for (const auto& name : tb_names) {
    key += name + "=" + type_suffix_for_mangling(type_bindings.at(name)) + ";";
  }

  std::vector<std::string> nb_names;
  nb_names.reserve(nttp_bindings.size());
  for (const auto& [name, _] : nttp_bindings) nb_names.push_back(name);
  std::sort(nb_names.begin(), nb_names.end());
  key += "|nb=";
  for (const auto& name : nb_names) {
    key += name + "=" + std::to_string(nttp_bindings.at(name)) + ";";
  }

  return key;
}

inline bool typespec_has_complete_text_identity(const TypeSpec& ts) {
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

inline bool typespec_has_structured_nominal_identity(const TypeSpec& ts) {
  return (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) ||
         typespec_has_complete_text_identity(ts);
}

inline bool typespec_requires_nominal_identity(const TypeSpec& ts) {
  return ts.base == TB_STRUCT || ts.base == TB_UNION || ts.base == TB_ENUM ||
         ts.base == TB_TYPEDEF;
}

inline std::optional<bool> structured_typespec_nominal_match(
    const TypeSpec& spec_ts,
    const TypeSpec& bind_ts) {
  const bool spec_has_record =
      spec_ts.record_def && spec_ts.record_def->kind == NK_STRUCT_DEF;
  const bool bind_has_record =
      bind_ts.record_def && bind_ts.record_def->kind == NK_STRUCT_DEF;
  if (spec_has_record && bind_has_record) {
    return spec_ts.record_def == bind_ts.record_def;
  }

  const bool spec_has_text = typespec_has_complete_text_identity(spec_ts);
  const bool bind_has_text = typespec_has_complete_text_identity(bind_ts);
  if (spec_has_text && bind_has_text) {
    if (spec_ts.namespace_context_id != bind_ts.namespace_context_id ||
        spec_ts.tag_text_id != bind_ts.tag_text_id ||
        spec_ts.is_global_qualified != bind_ts.is_global_qualified ||
        spec_ts.n_qualifier_segments != bind_ts.n_qualifier_segments) {
      return false;
    }
    for (int i = 0; i < spec_ts.n_qualifier_segments; ++i) {
      if (spec_ts.qualifier_text_ids[i] != bind_ts.qualifier_text_ids[i]) {
        return false;
      }
    }
    return true;
  }

  if (typespec_has_structured_nominal_identity(spec_ts) ||
      typespec_has_structured_nominal_identity(bind_ts)) {
    return false;
  }
  return std::nullopt;
}

// ── InstantiationRegistry ───────────────────────────────────────────────────
//
// Centralized bookkeeping for template instantiation discovery and
// realization.  Both AST-side seed collection and HIR-side deferred
// instantiation go through this registry so that dedup, specialization
// lookup, and instance recording share a single code path.
//
// Ownership split:
//   seed_work   – discovered template application candidates (todo list)
//   instances   – realized instantiations (authoritative for lowering)
//   function_specializations – explicit specialization AST nodes keyed by owner

class InstantiationRegistry {
 public:
  // ── Queries ──────────────────────────────────────────────────────────

  /// Look up realized instances for a template function (may be empty).
  const std::vector<TemplateInstance>* find_instances(
      const std::string& fn_name) const {
    auto it = instances_.find(fn_name);
    if (it == instances_.end()) return nullptr;
    return &it->second;
  }

  /// All realized instances (for iteration, e.g., metadata population).
  const std::unordered_map<std::string, std::vector<TemplateInstance>>&
  all_instances() const { return instances_; }

  /// All seed work items (for debugging / realize_seeds()).
  const std::unordered_map<std::string, std::vector<TemplateSeedWorkItem>>&
  all_seeds() const { return seed_work_; }

  // ── Mutations ────────────────────────────────────────────────────────

  /// Register an explicit specialization under its primary template def (owner-based).
  void register_function_specialization(const Node* primary_def,
                                        const Node* spec_node) {
    if (!primary_def || !spec_node) return;
    function_specializations_[primary_def].push_back(spec_node);
  }

  /// Look up registered function specializations for a primary def.
  const std::vector<const Node*>* find_function_specializations(
      const Node* primary_def) const {
    if (!primary_def) return nullptr;
    auto it = function_specializations_.find(primary_def);
    return it != function_specializations_.end() ? &it->second : nullptr;
  }

  /// Build a FunctionTemplateEnv from a primary def.
  FunctionTemplateEnv build_function_template_env(const Node* primary_def) const {
    FunctionTemplateEnv env;
    env.primary_def = primary_def;
    env.specialization_patterns = find_function_specializations(primary_def);
    return env;
  }

  /// Select the best function specialization pattern for given concrete bindings.
  /// Returns the selected pattern (specialization node or primary_def if none matches).
  SelectedFunctionTemplatePattern select_function_specialization(
      const Node* primary_def,
      const TypeBindings& bindings,
      const NttpBindings& nttp_bindings,
      const SpecializationKey& spec_key) const {
    SelectedFunctionTemplatePattern result;
    result.primary_def = primary_def;
    result.selected_pattern = primary_def;  // default: use primary
    result.type_bindings = bindings;
    result.nttp_bindings = nttp_bindings;
    result.spec_key = spec_key;
    if (!primary_def || primary_def->n_template_params <= 0) return result;
    auto* specs = find_function_specializations(primary_def);
    if (!specs || specs->empty()) return result;
    for (const Node* spec : *specs) {
      if (!spec || spec->n_template_args <= 0) continue;
      // Compare each declared arg against the concrete bindings.
      int n = std::min(spec->n_template_args, primary_def->n_template_params);
      bool match = true;
      for (int i = 0; i < n && match; ++i) {
        const char* pname = primary_def->template_param_names[i];
        if (!pname) continue;
        bool is_nttp = primary_def->template_param_is_nttp &&
                       primary_def->template_param_is_nttp[i];
        if (is_nttp) {
          if (spec->template_arg_is_value && spec->template_arg_is_value[i]) {
            auto it = nttp_bindings.find(pname);
            if (it == nttp_bindings.end() || it->second != spec->template_arg_values[i])
              match = false;
          }
        } else {
          auto it = bindings.find(pname);
          if (it == bindings.end()) { match = false; continue; }
          const TypeSpec& spec_ts = spec->template_arg_types[i];
          const TypeSpec& bind_ts = it->second;
          if (spec_ts.base != bind_ts.base || spec_ts.ptr_level != bind_ts.ptr_level)
            match = false;
          else if (std::optional<bool> structured_match =
                       structured_typespec_nominal_match(spec_ts, bind_ts))
            match = *structured_match;
          else if (typespec_requires_nominal_identity(spec_ts) ||
                   typespec_requires_nominal_identity(bind_ts))
            match = false;
        }
      }
      if (match) {
        result.selected_pattern = spec;
        return result;
      }
    }
    return result;
  }

  /// Record a template instantiation seed.  Returns the mangled name,
  /// or "" if bindings are not concrete.  Instances are created
  /// exclusively by realize_seeds().
  std::string record_seed(
      const std::string& fn_name, TypeBindings bindings,
      NttpBindings nttp_bindings,
      NttpTextBindings nttp_bindings_by_text,
      const std::vector<std::string>& param_order,
      TemplateSeedOrigin origin = TemplateSeedOrigin::DirectCall,
      const Node* primary_def = nullptr) {
    if (!bindings_are_concrete(bindings)) return "";
    std::string mangled = mangle_template_name(fn_name, bindings,
                                                nttp_bindings);
    SpecializationKey sk = nttp_bindings.empty()
        ? make_specialization_key(fn_name, param_order, bindings)
        : make_specialization_key(fn_name, param_order, bindings,
                                  nttp_bindings);
    // Semantic dedup uses owner identity when available.
    if (primary_def) {
      FunctionTemplateInstanceKey fk{primary_def, sk};
      if (structured_seed_keys_.count(fk) > 0 ||
          structured_instance_keys_.count(fk) > 0) {
        return mangled;
      }
      structured_seed_keys_.insert(fk);
    } else {
      // Legacy fallback is intentionally non-authoritative and only used
      // when no owner identity is available.
      const auto* seeds = [&]() -> const std::vector<TemplateSeedWorkItem>* {
        auto it = seed_work_.find(fn_name);
        return it == seed_work_.end() ? nullptr : &it->second;
      }();
      const auto* insts = [&]() -> const std::vector<TemplateInstance>* {
        auto it = instances_.find(fn_name);
        return it == instances_.end() ? nullptr : &it->second;
      }();
      if (has_legacy_mangled_entry(seeds, insts, mangled)) {
        return mangled;
      }
    }
    seed_work_[fn_name].push_back(
        TemplateSeedWorkItem{fn_name, bindings, nttp_bindings,
                             std::move(nttp_bindings_by_text), mangled,
                             sk, origin, primary_def});
    return mangled;
  }

  /// Realize all seeds that have not yet been converted to instances.
  /// Returns the number of newly realized instances.
  size_t realize_seeds() {
    size_t realized = 0;
    for (const auto& [fn_name, seeds] : seed_work_) {
      for (const auto& seed : seeds) {
        if (seed.primary_def) {
          FunctionTemplateInstanceKey fk{seed.primary_def, seed.spec_key};
          if (structured_instance_keys_.count(fk) > 0) continue;
          structured_instance_keys_.insert(fk);
        } else {
          auto it = instances_.find(fn_name);
          const auto* insts = it == instances_.end() ? nullptr : &it->second;
          if (has_legacy_mangled_entry(nullptr, insts, seed.mangled_name))
            continue;
        }
        instances_[fn_name].push_back(
            {seed.bindings, seed.nttp_bindings, seed.nttp_bindings_by_text,
             seed.mangled_name,
             seed.spec_key, seed.primary_def});
        ++realized;
      }
    }
    return realized;
  }

  /// Compute total instance count across all template functions.
  size_t total_instance_count() const {
    size_t n = 0;
    for (const auto& [k, v] : instances_) n += v.size();
    return n;
  }

  /// Compute total seed count across all template functions.
  size_t total_seed_count() const {
    size_t n = 0;
    for (const auto& [k, v] : seed_work_) n += v.size();
    return n;
  }

  /// Verify that every seed has a corresponding realized instance and
  /// vice versa.  Returns true when seeds and instances are in perfect
  /// parity.
  bool verify_parity() const {
    auto seed_keys = build_semantic_seed_keys();
    auto instance_keys = build_semantic_instance_keys();
    if (seed_keys != instance_keys) return false;
    return total_seed_count() == total_instance_count();
  }

  /// Dump seed/instance parity report to the given stream.
  void dump_parity(FILE* out) const {
    size_t seeds = total_seed_count();
    size_t insts = total_instance_count();
    std::fprintf(out, "[InstantiationRegistry] seeds=%zu instances=%zu",
                 seeds, insts);
    if (seeds == insts)
      std::fprintf(out, " (parity OK)\n");
    else
      std::fprintf(out, " (MISMATCH)\n");

    // Report per-function detail.
    std::set<std::string> all_fns;
    for (const auto& [k, _] : seed_work_) all_fns.insert(k);
    for (const auto& [k, _] : instances_) all_fns.insert(k);
    for (const auto& fn : all_fns) {
      auto sit = seed_work_.find(fn);
      auto iit = instances_.find(fn);
      size_t s = sit != seed_work_.end() ? sit->second.size() : 0;
      size_t i = iit != instances_.end() ? iit->second.size() : 0;
      const char* status = (s == i) ? "ok" : "MISMATCH";
      std::fprintf(out, "  %-40s seeds=%-3zu instances=%-3zu %s\n",
                   fn.c_str(), s, i, status);

      // List unrealized seeds.
      if (sit != seed_work_.end()) {
        for (const auto& seed : sit->second) {
          bool realized = has_matching_instance(fn, seed);
          if (!realized)
            std::fprintf(out, "    UNREALIZED seed: %s\n",
                         seed.mangled_name.c_str());
        }
      }
      // List orphan instances (instance without seed).
      if (iit != instances_.end()) {
        for (const auto& inst : iit->second) {
          bool has_s = has_matching_seed(fn, inst);
          if (!has_s)
            std::fprintf(out, "    ORPHAN instance: %s\n",
                         inst.mangled_name.c_str());
        }
      }
    }
  }

 private:
  using SemanticKey = std::string;

  static SemanticKey make_semantic_key(
      const std::string& fn_name,
      const std::string& mangled_name,
      const FunctionTemplateInstanceKey* structured_key) {
    if (structured_key && structured_key->primary_def) {
      return std::to_string(reinterpret_cast<std::uintptr_t>(
                 structured_key->primary_def)) +
             "::" + structured_key->spec_key.canonical;
    }
    return fn_name + "::" + mangled_name;
  }

  static bool has_legacy_mangled_entry(
      const std::vector<TemplateSeedWorkItem>* seeds,
      const std::vector<TemplateInstance>* instances,
      const std::string& mangled_name) {
    if (seeds) {
      for (const auto& seed : *seeds)
        if (seed.mangled_name == mangled_name) return true;
    }
    if (instances) {
      for (const auto& inst : *instances)
        if (inst.mangled_name == mangled_name) return true;
    }
    return false;
  }

  std::unordered_set<SemanticKey> build_semantic_seed_keys() const {
    std::unordered_set<SemanticKey> keys;
    for (const auto& [fn_name, seeds] : seed_work_) {
      for (const auto& seed : seeds) {
        const FunctionTemplateInstanceKey fk{seed.primary_def, seed.spec_key};
        keys.insert(make_semantic_key(fn_name, seed.mangled_name,
                                      seed.primary_def ? &fk : nullptr));
      }
    }
    return keys;
  }

  std::unordered_set<SemanticKey> build_semantic_instance_keys() const {
    std::unordered_set<SemanticKey> keys;
    for (const auto& [fn_name, insts] : instances_) {
      for (const auto& inst : insts) {
        const FunctionTemplateInstanceKey fk{inst.primary_def, inst.spec_key};
        keys.insert(make_semantic_key(fn_name, inst.mangled_name,
                                      inst.primary_def ? &fk : nullptr));
      }
    }
    return keys;
  }

  bool has_matching_instance(const std::string& fn_name,
                             const TemplateSeedWorkItem& seed) const {
    auto it = instances_.find(fn_name);
    if (it == instances_.end()) return false;
    for (const auto& inst : it->second) {
      if (seed.primary_def && inst.primary_def) {
        if (seed.primary_def == inst.primary_def &&
            seed.spec_key == inst.spec_key) {
          return true;
        }
        continue;
      }
      if (seed.mangled_name == inst.mangled_name) return true;
    }
    return false;
  }

  bool has_matching_seed(const std::string& fn_name,
                         const TemplateInstance& inst) const {
    auto it = seed_work_.find(fn_name);
    if (it == seed_work_.end()) return false;
    for (const auto& seed : it->second) {
      if (seed.primary_def && inst.primary_def) {
        if (seed.primary_def == inst.primary_def &&
            seed.spec_key == inst.spec_key) {
          return true;
        }
        continue;
      }
      if (seed.mangled_name == inst.mangled_name) return true;
    }
    return false;
  }

  // Seed / todo list — discovered template application candidates.
  std::unordered_map<std::string, std::vector<TemplateSeedWorkItem>>
      seed_work_;
  // Realized instances — authoritative for lowering.
  std::unordered_map<std::string, std::vector<TemplateInstance>>
      instances_;
  // Owner-based function specializations: primary_def → list of spec nodes.
  std::unordered_map<const Node*, std::vector<const Node*>> function_specializations_;
  // Structured dedup sets for seeds and instances (primary_def + spec_key).
  std::unordered_set<FunctionTemplateInstanceKey, FunctionTemplateInstanceKeyHash>
      structured_seed_keys_;
  std::unordered_set<FunctionTemplateInstanceKey, FunctionTemplateInstanceKeyHash>
      structured_instance_keys_;
};

// ── CompileTimeState ─────────────────────────────────────────────────────
//
// Engine-owned state that travels through the HIR pipeline.
// Created during initial HIR construction (build_initial_hir), then
// passed to the compile-time engine (run_compile_time_engine) for
// iterative reduction.  This is the intended single source of truth
// for compile-time bookkeeping.

struct CompileTimeState {
  InstantiationRegistry registry;

  // ── Template / consteval definition registries ──────────────────────
  //
  // These maps give the compile-time engine direct visibility into which
  // template and consteval functions exist, without having to probe the
  // opaque Lowerer callback.  The Lowerer populates them during initial
  // HIR construction; the engine reads them during the fixpoint loop.

  /// Register a template function definition (AST node pointer).
  void register_template_def(const std::string& name, const Node* node) {
    template_fn_defs_[name] = node;
    if (auto key = make_compile_time_registry_key(
            CompileTimeRegistryKeyKind::TemplateFunction, node)) {
      template_fn_defs_by_key_[*key] = node;
    }
  }

  /// Register a template struct primary definition (AST node pointer).
  void register_template_struct_def(const std::string& name, const Node* node) {
    if (!is_primary_template_struct_def(node)) return;
    template_struct_defs_[name] = node;
    if (auto key = make_compile_time_registry_key(
            CompileTimeRegistryKeyKind::PrimaryTemplateStruct, node)) {
      template_struct_defs_by_key_[*key] = node;
    }
  }

  /// Register a template struct specialization under its primary template name.
  void register_template_struct_specialization(const std::string& primary_name,
                                              const Node* node) {
    template_struct_specializations_[primary_name].push_back(node);
  }

  /// Register a template struct specialization under a primary definition.
  void register_template_struct_specialization(const Node* primary_def,
                                              const Node* node) {
    if (!primary_def || !primary_def->name) return;
    register_template_struct_specialization(primary_def->name, node);
    if (auto key = make_compile_time_registry_key(
            CompileTimeRegistryKeyKind::TemplateStructSpecializationOwner,
            primary_def)) {
      template_struct_specializations_by_owner_key_[*key].push_back(node);
    }
  }

  /// Register a template function specialization under its primary definition.
  void register_function_specialization(const Node* primary_def,
                                        const Node* spec_node) {
    registry.register_function_specialization(primary_def, spec_node);
  }

  /// Register a consteval function definition (AST node pointer).
  void register_consteval_def(const std::string& name, const Node* node) {
    consteval_fn_defs_[name] = node;
    if (auto key = make_compile_time_registry_key(
            CompileTimeRegistryKeyKind::ConstevalFunction, node)) {
      consteval_fn_defs_by_key_[*key] = node;
    }
  }

  /// Register a non-template-owned static_assert for engine-time resolution.
  void register_static_assert(const Node* node) {
    if (!node) return;
    static_assert_nodes_.push_back(node);
  }

  /// Check whether a template function definition is known.
  bool has_template_def(const std::string& name) const {
    return template_fn_defs_.count(name) > 0;
  }

  /// Check whether a template function definition is known by declaration identity.
  bool has_template_def(const Node* declaration,
                        const std::string& rendered_name = {}) const {
    return find_template_def(declaration, rendered_name) != nullptr;
  }

  /// Check whether a template struct definition is known.
  bool has_template_struct_def(const std::string& name) const {
    return template_struct_defs_.count(name) > 0;
  }

  /// Check whether a template struct definition is known by declaration identity.
  bool has_template_struct_def(const Node* declaration,
                               const std::string& rendered_name = {}) const {
    return find_template_struct_def(declaration, rendered_name) != nullptr;
  }

  /// Check whether a consteval function definition is known.
  bool has_consteval_def(const std::string& name) const {
    return consteval_fn_defs_.count(name) > 0;
  }

  /// Check whether a consteval function definition is known by declaration identity.
  bool has_consteval_def(const Node* declaration,
                         const std::string& rendered_name = {}) const {
    return find_consteval_def(declaration, rendered_name) != nullptr;
  }

  /// Check whether a registered template definition is marked consteval.
  /// Returns true only if the definition exists AND has is_consteval set.
  bool is_consteval_template(const std::string& name) const {
    auto it = template_fn_defs_.find(name);
    if (it == template_fn_defs_.end()) return false;
    return it->second && it->second->is_consteval;
  }

  /// Check whether a registered template definition is marked consteval by
  /// declaration identity.  A rendered name is a legacy fallback only.
  bool is_consteval_template(const Node* declaration,
                             const std::string& rendered_name = {}) const {
    const Node* def = find_template_def(declaration, rendered_name);
    return def && def->is_consteval;
  }

  /// Look up a template function definition by name (nullptr if unknown).
  const Node* find_template_def(const std::string& name) const {
    auto it = template_fn_defs_.find(name);
    return it != template_fn_defs_.end() ? it->second : nullptr;
  }

  /// Look up a template function definition by declaration identity, falling
  /// back to the rendered name only when one is supplied.
  const Node* find_template_def(const Node* declaration,
                                const std::string& rendered_name = {}) const {
    return find_structured_node_entry(
        CompileTimeRegistryKeyKind::TemplateFunction, declaration,
        rendered_name, template_fn_defs_by_key_, template_fn_defs_);
  }

  /// Look up a template struct definition by name (nullptr if unknown).
  const Node* find_template_struct_def(const std::string& name) const {
    auto it = template_struct_defs_.find(name);
    return it != template_struct_defs_.end() ? it->second : nullptr;
  }

  /// Look up a template struct definition by declaration identity, falling
  /// back to the rendered name only when one is supplied.
  const Node* find_template_struct_def(
      const Node* declaration,
      const std::string& rendered_name = {}) const {
    return find_structured_node_entry(
        CompileTimeRegistryKeyKind::PrimaryTemplateStruct, declaration,
        rendered_name, template_struct_defs_by_key_, template_struct_defs_);
  }

  /// Look up a template struct definition by structured parser-owned origin.
  const Node* find_template_struct_def(const QualifiedNameKey& key) const {
    if (key.base_text_id == kInvalidText ||
        key.qualifier_path_id != kInvalidNamePath) {
      return nullptr;
    }
    CompileTimeRegistryKey registry_key;
    registry_key.registry_kind =
        CompileTimeRegistryKeyKind::PrimaryTemplateStruct;
    registry_key.declaration_kind = NK_STRUCT_DEF;
    registry_key.namespace_context_id = key.context_id;
    registry_key.is_global_qualified = key.is_global_qualified;
    registry_key.unqualified_text_id = key.base_text_id;
    const auto it = template_struct_defs_by_key_.find(registry_key);
    return it == template_struct_defs_by_key_.end() ? nullptr : it->second;
  }

  /// Look up registered template struct specializations (nullptr if unknown).
  const std::vector<const Node*>* find_template_struct_specializations(
      const std::string& name) const {
    auto it = template_struct_specializations_.find(name);
    return it != template_struct_specializations_.end() ? &it->second : nullptr;
  }

  /// Look up registered template struct specializations for a primary definition.
  const std::vector<const Node*>* find_template_struct_specializations(
      const Node* primary_def) const {
    if (!primary_def || !primary_def->name) return nullptr;
    return find_template_struct_specializations(primary_def, primary_def->name);
  }

  /// Look up registered template struct specializations by primary declaration
  /// identity, falling back to the rendered primary name only when supplied.
  const std::vector<const Node*>* find_template_struct_specializations(
      const Node* primary_def,
      const std::string& rendered_name) const {
    return find_structured_vector_entry(
        CompileTimeRegistryKeyKind::TemplateStructSpecializationOwner,
        primary_def, rendered_name,
        template_struct_specializations_by_owner_key_,
        template_struct_specializations_);
  }

  /// Look up a consteval function definition by name (nullptr if unknown).
  const Node* find_consteval_def(const std::string& name) const {
    auto it = consteval_fn_defs_.find(name);
    return it != consteval_fn_defs_.end() ? it->second : nullptr;
  }

  /// Look up a consteval function definition by declaration identity, falling
  /// back to the rendered name only when one is supplied.
  const Node* find_consteval_def(const Node* declaration,
                                 const std::string& rendered_name = {}) const {
    return find_structured_node_entry(
        CompileTimeRegistryKeyKind::ConstevalFunction, declaration,
        rendered_name, consteval_fn_defs_by_key_, consteval_fn_defs_);
  }

  /// Const reference to the internal consteval function definition map.
  /// Useful for passing to evaluate_consteval_call() which expects a map ref.
  const std::unordered_map<std::string, const Node*>& consteval_fn_defs() const {
    return consteval_fn_defs_;
  }

  /// Const reference to registered enum constants used during consteval.
  const std::unordered_map<std::string, long long>& enum_consts() const {
    return enum_consts_;
  }

  /// Structured mirror of registered enum constants.
  const std::unordered_map<CompileTimeValueBindingKey, long long,
                           CompileTimeValueBindingKeyHash>&
  enum_consts_by_key() const {
    return enum_consts_by_key_;
  }

  /// Const reference to registered global const-int bindings used during consteval.
  const std::unordered_map<std::string, long long>& const_int_bindings() const {
    return const_int_bindings_;
  }

  /// Structured mirror of registered global const-int bindings.
  const std::unordered_map<CompileTimeValueBindingKey, long long,
                           CompileTimeValueBindingKeyHash>&
  const_int_bindings_by_key() const {
    return const_int_bindings_by_key_;
  }

  const std::vector<const Node*>& static_assert_nodes() const {
    return static_assert_nodes_;
  }

  /// Number of registered template function definitions.
  size_t template_def_count() const { return template_fn_defs_.size(); }

  /// Number of registered consteval function definitions.
  size_t consteval_def_count() const { return consteval_fn_defs_.size(); }

  /// Iterate over all registered template function definitions.
  template<typename Fn>
  void for_each_template_def(Fn&& fn) const {
    for (const auto& [name, node] : template_fn_defs_)
      fn(name, node);
  }

  /// Record a deferred template instance discovered during the engine's
  /// fixpoint loop.  Updates the registry (seed + realize) and returns a
  /// HirTemplateInstantiation suitable for appending to module metadata.
  HirTemplateInstantiation record_deferred_instance(
      const std::string& source_template,
      const TypeBindings& bindings,
      const NttpBindings& nttp_bindings,
      const NttpTextBindings& nttp_bindings_by_text,
      const std::string& mangled_name,
      const std::vector<std::string>& template_params) {
    const Node* primary_def = find_template_def(source_template);
    registry.record_seed(source_template, bindings, nttp_bindings,
                         nttp_bindings_by_text,
                         template_params,
                         TemplateSeedOrigin::EnclosingTemplateExpansion,
                         primary_def);
    registry.realize_seeds();
    HirTemplateInstantiation hi;
    hi.mangled_name = mangled_name;
    hi.bindings = bindings;
    hi.nttp_bindings = nttp_bindings;
    hi.nttp_bindings_by_text = nttp_bindings_by_text;
    hi.spec_key = nttp_bindings.empty()
        ? make_specialization_key(source_template, template_params, bindings)
        : make_specialization_key(source_template, template_params, bindings,
                                  nttp_bindings);
    return hi;
  }

  /// Convert all realized instances for a given template into
  /// HirTemplateInstantiation metadata objects.  Used to populate
  /// module.template_defs during initial HIR construction.
  std::vector<HirTemplateInstantiation> instances_to_hir_metadata(
      const std::string& fn_name,
      const std::vector<std::string>& template_params) const {
    std::vector<HirTemplateInstantiation> result;
    auto* inst_list = registry.find_instances(fn_name);
    if (!inst_list) return result;
    result.reserve(inst_list->size());
    for (const auto& inst : *inst_list) {
      HirTemplateInstantiation hi;
      hi.mangled_name = inst.mangled_name;
      hi.bindings = inst.bindings;
      hi.nttp_bindings = inst.nttp_bindings;
      hi.nttp_bindings_by_text = inst.nttp_bindings_by_text;
      hi.spec_key = inst.nttp_bindings.empty()
          ? make_specialization_key(fn_name, template_params, inst.bindings)
          : make_specialization_key(fn_name, template_params, inst.bindings,
                                    inst.nttp_bindings);
      result.push_back(std::move(hi));
    }
    return result;
  }

  // ── Constant environment (for consteval evaluation) ──────────────────
  //
  // The Lowerer populates these maps during initial HIR construction
  // by calling register_enum_const() / register_const_int_binding().
  // The engine uses them to build a ConstEvalEnv when evaluating
  // PendingConstevalExpr nodes directly, without going through the
  // Lowerer callback.
  //
  // NOTE: The Lowerer also keeps its own working copies of these maps
  // because it needs mutable save/restore semantics for block-scoped
  // enum definitions (see NK_BLOCK handling in lower_stmt_node).
  // The CompileTimeState copies accumulate the final global state and
  // are used exclusively during the engine normalization phase.

  /// Register an enum constant value.
  void register_enum_const(const std::string& name, long long value) {
    enum_consts_[name] = value;
  }

  /// Register an enum constant value with a structured global mirror.
  void register_enum_const(const CompileTimeValueBindingKey& key,
                           const std::string& rendered_name,
                           long long value) {
    register_enum_const(rendered_name, value);
    if (key.valid() &&
        key.binding_kind == CompileTimeValueBindingKeyKind::GlobalEnumConstant) {
      enum_consts_by_key_[key] = value;
    }
  }

  /// Register a global const-integer binding.
  void register_const_int_binding(const std::string& name, long long value) {
    const_int_bindings_[name] = value;
  }

  /// Register a global const-integer binding with a structured global mirror.
  void register_const_int_binding(const CompileTimeValueBindingKey& key,
                                  const std::string& rendered_name,
                                  long long value) {
    register_const_int_binding(rendered_name, value);
    if (key.valid() &&
        key.binding_kind == CompileTimeValueBindingKeyKind::GlobalConstInt) {
      const_int_bindings_by_key_[key] = value;
    }
  }

  bool record_pending_template_type(PendingTemplateTypeKind kind,
                                    const TypeSpec& pending_type,
                                    const Node* owner_primary_def,
                                    const TypeBindings& type_bindings,
                                    const NttpBindings& nttp_bindings,
                                    const SourceSpan& span,
                                    const std::string& context_name = {}) {
    if (!pending_type.tpl_struct_origin && !pending_type.deferred_member_type_name)
      return false;
    PendingTemplateTypeWorkItem item;
    const Node* canonical_owner_primary_def = owner_primary_def;
    if (!canonical_owner_primary_def) {
      const std::string rendered_origin =
          pending_type.tpl_struct_origin ? pending_type.tpl_struct_origin : "";
      if (pending_type.record_def && pending_type.record_def->kind == NK_STRUCT_DEF) {
        canonical_owner_primary_def =
            find_template_struct_def(pending_type.record_def, rendered_origin);
      } else if (!rendered_origin.empty()) {
        canonical_owner_primary_def = find_template_struct_def(rendered_origin);
      }
    }
    TypeSpec canonical_pending_type = pending_type;
    if (canonical_owner_primary_def && canonical_owner_primary_def->name &&
        canonical_pending_type.tpl_struct_origin) {
      canonical_pending_type.tpl_struct_origin = canonical_owner_primary_def->name;
    }
    item.kind = kind;
    item.pending_type = canonical_pending_type;
    item.owner_primary_def = canonical_owner_primary_def;
    item.type_bindings = type_bindings;
    item.nttp_bindings = nttp_bindings;
    item.span = span;
    item.context_name = context_name;
    item.dedup_key = make_pending_template_type_key(
        kind, canonical_pending_type, canonical_owner_primary_def,
        type_bindings, nttp_bindings,
        context_name, span);
    if (pending_template_type_keys_.count(item.dedup_key) > 0) return false;
    pending_template_type_keys_.insert(item.dedup_key);
    pending_template_types_.push_back(std::move(item));
    return true;
  }

  const std::vector<PendingTemplateTypeWorkItem>& pending_template_types() const {
    return pending_template_types_;
  }

  bool is_pending_template_type_resolved(const std::string& key) const {
    return resolved_pending_template_type_keys_.count(key) > 0;
  }

  void mark_pending_template_type_resolved(const std::string& key) {
    if (!key.empty()) resolved_pending_template_type_keys_.insert(key);
  }

  size_t pending_template_type_count() const {
    return pending_template_types_.size();
  }

  /// Dump debug visibility for seed work vs realized instances.
  void dump(FILE* out) const {
    std::fprintf(out, "[CompileTimeState] template_defs=%zu consteval_defs=%zu\n",
                 template_fn_defs_.size(), consteval_fn_defs_.size());
    std::fprintf(out, "[CompileTimeState] template_struct_defs=%zu template_struct_specializations=%zu\n",
                 template_struct_defs_.size(), template_struct_specializations_.size());
    std::fprintf(out, "[CompileTimeState] structured_template_defs=%zu structured_consteval_defs=%zu\n",
                 template_fn_defs_by_key_.size(), consteval_fn_defs_by_key_.size());
    std::fprintf(out, "[CompileTimeState] structured_template_struct_defs=%zu structured_template_struct_specialization_owners=%zu\n",
                 template_struct_defs_by_key_.size(),
                 template_struct_specializations_by_owner_key_.size());
    std::fprintf(out, "[CompileTimeState] enum_consts=%zu const_int_bindings=%zu\n",
                 enum_consts_.size(), const_int_bindings_.size());
    std::fprintf(out, "[CompileTimeState] structured_enum_consts=%zu structured_const_int_bindings=%zu\n",
                 enum_consts_by_key_.size(), const_int_bindings_by_key_.size());
    std::fprintf(out, "[CompileTimeState] pending_template_types=%zu\n",
                 pending_template_types_.size());
    std::fprintf(out, "[CompileTimeState] registry parity:\n");
    registry.dump_parity(out);
  }

 private:
  static const Node* find_structured_node_entry(
      CompileTimeRegistryKeyKind registry_kind,
      const Node* declaration,
      const std::string& rendered_name,
      const std::unordered_map<CompileTimeRegistryKey, const Node*,
                               CompileTimeRegistryKeyHash>& structured_map,
      const std::unordered_map<std::string, const Node*>& rendered_map) {
    if (auto key = make_compile_time_registry_key(registry_kind, declaration)) {
      auto structured_it = structured_map.find(*key);
      if (structured_it != structured_map.end()) return structured_it->second;
    }
    if (!rendered_name.empty()) {
      auto rendered_it = rendered_map.find(rendered_name);
      if (rendered_it != rendered_map.end()) return rendered_it->second;
    }
    return nullptr;
  }

  static const std::vector<const Node*>* find_structured_vector_entry(
      CompileTimeRegistryKeyKind registry_kind,
      const Node* declaration,
      const std::string& rendered_name,
      const std::unordered_map<CompileTimeRegistryKey,
                               std::vector<const Node*>,
                               CompileTimeRegistryKeyHash>& structured_map,
      const std::unordered_map<std::string,
                               std::vector<const Node*>>& rendered_map) {
    if (auto key = make_compile_time_registry_key(registry_kind, declaration)) {
      auto structured_it = structured_map.find(*key);
      if (structured_it != structured_map.end()) return &structured_it->second;
    }
    if (!rendered_name.empty()) {
      auto rendered_it = rendered_map.find(rendered_name);
      if (rendered_it != rendered_map.end()) return &rendered_it->second;
    }
    return nullptr;
  }

  // Step 5 classification: rendered-name maps are compatibility fallbacks for
  // callers that cannot yet supply declaration keys; *_by_key_ mirrors are the
  // structured authority when present.
  // Template function definitions indexed by name (AST node pointers).
  std::unordered_map<std::string, const Node*> template_fn_defs_;
  // Best-effort structured mirrors indexed by declaration identity.
  std::unordered_map<CompileTimeRegistryKey, const Node*,
                     CompileTimeRegistryKeyHash>
      template_fn_defs_by_key_;
  // Template struct primary definitions indexed by name (AST node pointers).
  std::unordered_map<std::string, const Node*> template_struct_defs_;
  std::unordered_map<CompileTimeRegistryKey, const Node*,
                     CompileTimeRegistryKeyHash>
      template_struct_defs_by_key_;
  // Template struct specializations indexed by primary name.
  std::unordered_map<std::string, std::vector<const Node*>> template_struct_specializations_;
  std::unordered_map<CompileTimeRegistryKey, std::vector<const Node*>,
                     CompileTimeRegistryKeyHash>
      template_struct_specializations_by_owner_key_;
  // Consteval function definitions indexed by name (AST node pointers).
  std::unordered_map<std::string, const Node*> consteval_fn_defs_;
  std::unordered_map<CompileTimeRegistryKey, const Node*,
                     CompileTimeRegistryKeyHash>
      consteval_fn_defs_by_key_;
  // Enum/const integer name maps remain compatibility and diagnostic-facing
  // consteval inputs; structured value-binding mirrors are preferred when a
  // declaration key is available.
  // Enum constant values (name → value).
  std::unordered_map<std::string, long long> enum_consts_;
  std::unordered_map<CompileTimeValueBindingKey, long long,
                     CompileTimeValueBindingKeyHash>
      enum_consts_by_key_;
  // Global const-integer bindings (name → value).
  std::unordered_map<std::string, long long> const_int_bindings_;
  std::unordered_map<CompileTimeValueBindingKey, long long,
                     CompileTimeValueBindingKeyHash>
      const_int_bindings_by_key_;
  std::vector<const Node*> static_assert_nodes_;
  // Pending type-driven template work discovered during AST-to-HIR lowering.
  // String keys here are unresolved-boundary dedup/progress tokens rather than
  // final semantic lookup authority.
  std::vector<PendingTemplateTypeWorkItem> pending_template_types_;
  std::unordered_set<std::string> pending_template_type_keys_;
  std::unordered_set<std::string> resolved_pending_template_type_keys_;
};

/// A diagnostic for a single irreducible compile-time node.
struct CompileTimeDiagnostic {
  enum Kind { UnresolvedTemplate, UnreducedConsteval, StaticAssert };
  Kind kind;
  const char* file = nullptr;
  int line = 0;
  int column = 1;
  std::string description;  // human-readable description of the irreducible node
};

/// Result of a single compile-time reduction iteration.
struct CompileTimeEngineStats {
  size_t templates_instantiated = 0;  // template calls with resolved target functions
  size_t templates_pending = 0;       // template calls whose target is missing
  size_t templates_deferred = 0;      // template instantiations created by this pass
  size_t template_types_resolved = 0; // pending type work items resolved by the pass
  size_t template_types_terminal = 0; // pending type work items that reached terminal state
  size_t template_types_pending = 0;  // pending type-driven template work items observed
  size_t consteval_reduced = 0;       // consteval calls successfully reduced to constants
  size_t consteval_pending = 0;       // consteval calls whose result is missing or invalid
  size_t consteval_deferred = 0;      // consteval reductions unlocked by deferred template instantiation
  size_t iterations = 0;              // total fixpoint iterations performed
  bool converged = false;             // true if no new work was found
  // Definition registries (populated when ct_state is provided).
  size_t template_defs_known = 0;      // template function definitions registered
  size_t consteval_defs_known = 0;     // consteval function definitions registered
  // Registry parity (populated when ct_state is provided).
  size_t registry_seeds = 0;           // total seeds in registry after engine run
  size_t registry_instances = 0;       // total realized instances in registry after engine run
  bool registry_parity = true;         // true if seeds == instances (all realized)
  std::vector<CompileTimeDiagnostic> diagnostics;  // details on irreducible nodes
};

/// Callback for deferred template instantiation.
///
/// Called by the compile-time reduction pass when it discovers a template
/// call whose target function has not been lowered yet.
///
/// Parameters:
///   template_name  - original template name (e.g. "add")
///   bindings       - resolved type bindings (e.g. {T: int})
///   mangled_name   - target mangled name (e.g. "add_i")
///
/// Returns true if the function was successfully lowered and added to the module.
using DeferredInstantiateFn = std::function<bool(
    const std::string& template_name,
    const TypeBindings& bindings,
    const NttpBindings& nttp_bindings,
    const NttpTextBindings& nttp_bindings_by_text,
    const std::string& mangled_name)>;

using DeferredInstantiateTypeFn = std::function<DeferredTemplateTypeResult(
    const PendingTemplateTypeWorkItem& work_item)>;

/// Run the HIR compile-time reduction loop.
///
/// This pass iterates:
///   1. template instantiation for newly-ready applications
///   2. consteval reduction for newly-ready immediate calls
///   3. pending consteval evaluation for engine-owned PendingConstevalExpr nodes
/// until convergence or the iteration limit is reached.
///
/// When instantiate_fn is provided, the pass will invoke it to lower template
/// functions that were not instantiated during initial AST-to-HIR lowering
/// (e.g., nested template calls like add<T>() inside twice<T>()).
/// Consteval evaluation uses engine-owned CompileTimeState definitions and
/// constant bindings directly. When callbacks are null, the pass only verifies
/// existing state.
CompileTimeEngineStats run_compile_time_engine(
    Module& module,
    std::shared_ptr<CompileTimeState> ct_state = nullptr,
    DeferredInstantiateFn instantiate_fn = nullptr,
    DeferredInstantiateTypeFn instantiate_type_fn = nullptr);

/// Format pass statistics for debug output (used by --dump-hir).
std::string format_compile_time_stats(const CompileTimeEngineStats& stats);

/// Result of materialization.
struct MaterializationStats {
  size_t materialized = 0;      // functions marked for emission
  size_t non_materialized = 0;  // functions kept for compile-time only
};

/// Mark functions in the module for materialization.
///
/// Current policy: all concrete functions are materialized.  This pass
/// makes the materialization decision explicit and separable from codegen,
/// so future policies (lazy emission, JIT deferral) can override it.
MaterializationStats materialize_ready_functions(Module& module);

/// Format materialization stats for debug output.
std::string format_materialization_stats(const MaterializationStats& stats);

}  // namespace c4c::hir
