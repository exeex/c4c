#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "hir_ir.hpp"

namespace c4c::hir {

// ── Constant-evaluation value model ──────────────────────────────────────────

enum class ConstValueKind {
  Integer,
  Boolean,
  NullPtr,
  Unknown,
};

struct ConstValue {
  ConstValueKind kind = ConstValueKind::Unknown;
  long long int_val = 0;

  static ConstValue make_int(long long v) { return {ConstValueKind::Integer, v}; }
  static ConstValue make_bool(bool v) { return {ConstValueKind::Boolean, v ? 1LL : 0LL}; }
  static ConstValue make_null() { return {ConstValueKind::NullPtr, 0}; }
  static ConstValue unknown() { return {ConstValueKind::Unknown, 0}; }

  bool is_known() const { return kind != ConstValueKind::Unknown; }
  long long as_int() const { return int_val; }
};

struct ConstEvalResult {
  std::optional<ConstValue> value;
  std::string error;  // non-empty on failure: describes why evaluation failed

  bool ok() const { return value.has_value() && value->is_known(); }
  long long as_int() const { return value->as_int(); }
  std::optional<long long> as_optional_int() const {
    if (ok()) return value->as_int();
    return std::nullopt;
  }

  static ConstEvalResult success(ConstValue v) { return {v, {}}; }
  static ConstEvalResult failure(std::string msg = {}) { return {std::nullopt, std::move(msg)}; }
};

// ── Constant-evaluation environment ──────────────────────────────────────────

// Rendered-name value map. Owner: legacy HIR/sema handoff and interpreter
// compatibility paths that still lack TextId or structured name metadata.
// Limitation: rendered strings are not semantic identity for metadata-rich
// paths. Removal condition: all producers provide TextId/structured maps.
using ConstMap = std::unordered_map<std::string, long long>;
using ConstTextMap = std::unordered_map<TextId, long long>;

struct ConstEvalStructuredNameKey {
  int namespace_context_id = -1;
  bool is_global_qualified = false;
  std::vector<TextId> qualifier_text_ids;
  TextId base_text_id = kInvalidText;

  bool valid() const { return base_text_id != kInvalidText; }
  bool operator==(const ConstEvalStructuredNameKey& other) const {
    return namespace_context_id == other.namespace_context_id &&
           is_global_qualified == other.is_global_qualified &&
           qualifier_text_ids == other.qualifier_text_ids &&
           base_text_id == other.base_text_id;
  }
  bool operator!=(const ConstEvalStructuredNameKey& other) const { return !(*this == other); }
};

struct ConstEvalStructuredNameKeyHash {
  std::size_t operator()(const ConstEvalStructuredNameKey& key) const {
    std::size_t h = std::hash<int>{}(key.namespace_context_id);
    h ^= std::hash<bool>{}(key.is_global_qualified) + 0x9e3779b9u + (h << 6) + (h >> 2);
    h ^= std::hash<TextId>{}(key.base_text_id) + 0x9e3779b9u + (h << 6) + (h >> 2);
    for (TextId segment : key.qualifier_text_ids) {
      h ^= std::hash<TextId>{}(segment) + 0x9e3779b9u + (h << 6) + (h >> 2);
    }
    return h;
  }
};

using ConstStructuredMap =
    std::unordered_map<ConstEvalStructuredNameKey, long long, ConstEvalStructuredNameKeyHash>;
using ConstEvalFunctionTextMap = std::unordered_map<TextId, const Node*>;
using ConstEvalFunctionStructuredMap =
    std::unordered_map<ConstEvalStructuredNameKey, const Node*, ConstEvalStructuredNameKeyHash>;

// Rendered-name template type binding map. Owner: call-env compatibility
// mirror for template substitution sites without parameter TextId/key
// metadata. Limitation: not authoritative when text/key binding metadata is
// present. Removal condition: all template binding lookups carry metadata.
using TypeBindings = std::unordered_map<std::string, TypeSpec>;
using TypeBindingTextMap = std::unordered_map<TextId, TypeSpec>;

struct TypeBindingStructuredKey {
  int namespace_context_id = -1;
  TextId template_text_id = kInvalidText;
  int param_index = -1;
  TextId param_text_id = kInvalidText;

  bool valid() const { return template_text_id != kInvalidText && param_index >= 0; }
  bool operator==(const TypeBindingStructuredKey& other) const {
    return namespace_context_id == other.namespace_context_id &&
           template_text_id == other.template_text_id &&
           param_index == other.param_index &&
           param_text_id == other.param_text_id;
  }
};

struct TypeBindingStructuredKeyHash {
  std::size_t operator()(const TypeBindingStructuredKey& key) const {
    std::size_t h = std::hash<int>{}(key.namespace_context_id);
    h ^= std::hash<TextId>{}(key.template_text_id) + 0x9e3779b9u + (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(key.param_index) + 0x9e3779b9u + (h << 6) + (h >> 2);
    h ^= std::hash<TextId>{}(key.param_text_id) + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
  }
};

using TypeBindingStructuredMap =
    std::unordered_map<TypeBindingStructuredKey, TypeSpec, TypeBindingStructuredKeyHash>;
// Rendered parameter spelling -> metadata bridges. Owner: call-env/type
// substitution compatibility for legacy TypeSpecs that still carry only a
// display tag. Limitation: these maps select TextId/key authority but are not
// binding authority themselves. Removal condition: all TypeSpecs carry
// parameter TextId or structured owner metadata directly.
using TypeBindingNameTextMap = std::unordered_map<std::string, TextId>;
using TypeBindingNameStructuredMap =
    std::unordered_map<std::string, TypeBindingStructuredKey>;

enum class ConstEvalValueLookupStatus {
  NoMetadata,
  Miss,
  Found,
};

struct ConstEvalValueLookupResult {
  ConstEvalValueLookupStatus status = ConstEvalValueLookupStatus::NoMetadata;
  long long value = 0;
  bool enum_binding_metadata_miss = false;
  bool local_binding_metadata_miss = false;
  bool named_binding_metadata_miss = false;
  bool nttp_binding_metadata_miss = false;
};

struct ConstEvalEnv {
  // Flat rendered maps are no-metadata compatibility fallbacks. TextId and
  // structured maps below are the semantic authority when their metadata is
  // available for the queried node.
  const ConstMap* enum_consts = nullptr;
  const ConstMap* named_consts = nullptr;
  const ConstMap* local_consts = nullptr;

  const ConstTextMap* enum_consts_by_text = nullptr;
  const ConstTextMap* named_consts_by_text = nullptr;
  const ConstTextMap* local_consts_by_text = nullptr;
  const ConstStructuredMap* enum_consts_by_key = nullptr;
  const ConstStructuredMap* named_consts_by_key = nullptr;
  const ConstStructuredMap* local_consts_by_key = nullptr;

  // Scoped rendered maps are compatibility mirrors for validate.cpp scope
  // stacks. Owner: legacy scoped enum/local handoff. Limitation: TextId/key
  // scope stacks are authoritative when present. Removal condition: scoped
  // handoff always provides metadata maps. Searched innermost (back) to
  // outermost (front).
  const std::vector<ConstMap>* enum_scopes = nullptr;
  const std::vector<ConstMap>* local_const_scopes = nullptr;
  const std::vector<ConstTextMap>* enum_scopes_by_text = nullptr;
  const std::vector<ConstTextMap>* local_const_scopes_by_text = nullptr;
  const std::vector<ConstStructuredMap>* enum_scopes_by_key = nullptr;
  const std::vector<ConstStructuredMap>* local_const_scopes_by_key = nullptr;

  // Template type substitution. `type_bindings` is the rendered compatibility
  // mirror; text/key maps are authority for metadata-rich paths. The
  // name->metadata maps are bridge indexes, not semantic authority.
  const TypeBindings* type_bindings = nullptr;
  const TypeBindingTextMap* type_bindings_by_text = nullptr;
  const TypeBindingStructuredMap* type_bindings_by_key = nullptr;
  const TypeBindingNameTextMap* type_binding_text_ids_by_name = nullptr;
  const TypeBindingNameStructuredMap* type_binding_keys_by_name = nullptr;

  // NTTP substitution. Owner: call-env compatibility for legacy forwarded
  // parameter spellings. Limitation: the rendered map is a no-metadata mirror;
  // text/key maps are authoritative for forwarded NTTP metadata. Removal
  // condition: all NTTP forwarding carries TextId/key metadata.
  const std::unordered_map<std::string, long long>* nttp_bindings = nullptr;
  const ConstTextMap* nttp_bindings_by_text = nullptr;
  const ConstStructuredMap* nttp_bindings_by_key = nullptr;

  // Optional late-known record layouts from HIR lowering. Owner: HIR layout
  // handoff for sizeof/alignof on tagged records that were not immediately
  // computable in sema. Limitation: `struct_defs` is only a rendered-tag
  // compatibility map; `struct_def_owner_index` is authority when TypeSpec
  // metadata can identify the record. `link_name_texts` only canonicalizes
  // cross-table TextIds and still ends at owner-key lookup. Removal condition:
  // all record TypeSpecs and layout handoff use one structured owner-key table.
  const std::unordered_map<std::string, HirStructDef>* struct_defs = nullptr;
  const std::unordered_map<HirRecordOwnerKey, std::string, HirRecordOwnerKeyHash>*
      struct_def_owner_index = nullptr;
  const TextTable* link_name_texts = nullptr;

  using TemplateStructLookupFn =
      const Node* (*)(const TypeSpec& owner, const void* ctx);
  TemplateStructLookupFn lookup_template_struct_primary = nullptr;
  const void* template_struct_lookup_ctx = nullptr;

  std::optional<long long> lookup(const std::string& name) const {
    // Rendered lookup is intentionally retained only for callers that provide
    // no Node/TextId/key metadata. Node-based lookup below must gate entry here.
    // 1. Scoped enum constants (innermost first).
    if (enum_scopes) {
      for (auto it = enum_scopes->rbegin(); it != enum_scopes->rend(); ++it) {
        auto sit = it->find(name);
        if (sit != it->end()) return sit->second;
      }
    }
    // 2. Flat enum constants.
    if (enum_consts) {
      auto it = enum_consts->find(name);
      if (it != enum_consts->end()) return it->second;
    }
    // 3. Scoped local constants (innermost first).
    if (local_const_scopes) {
      for (auto it = local_const_scopes->rbegin(); it != local_const_scopes->rend(); ++it) {
        auto sit = it->find(name);
        if (sit != it->end()) return sit->second;
      }
    }
    // 4. Flat local constants.
    if (local_consts) {
      auto it = local_consts->find(name);
      if (it != local_consts->end()) return it->second;
    }
    // 5. Named constants (global const/constexpr bindings).
    if (named_consts) {
      auto it = named_consts->find(name);
      if (it != named_consts->end()) return it->second;
    }
    // 6. Non-type template parameter bindings.
    if (nttp_bindings) {
      auto it = nttp_bindings->find(name);
      if (it != nttp_bindings->end()) return it->second;
    }
    return std::nullopt;
  }

  std::optional<long long> lookup(const Node* n) const {
    if (!n || !n->name || !n->name[0]) return std::nullopt;

    const ConstEvalValueLookupResult structured = lookup_by_key(n);
    if (structured.status == ConstEvalValueLookupStatus::Found) {
      return structured.value;
    }
    bool has_authoritative_metadata =
        structured.status == ConstEvalValueLookupStatus::Miss;

    const ConstEvalValueLookupResult text = lookup_by_text(n);
    if (text.status == ConstEvalValueLookupStatus::Found) {
      return text.value;
    }
    has_authoritative_metadata =
        has_authoritative_metadata ||
        text.status == ConstEvalValueLookupStatus::Miss;
    if (has_authoritative_metadata) {
      return lookup_rendered_compatibility(
          n->name,
          structured.enum_binding_metadata_miss ||
              text.enum_binding_metadata_miss,
          structured.local_binding_metadata_miss ||
              text.local_binding_metadata_miss,
          structured.named_binding_metadata_miss ||
              text.named_binding_metadata_miss,
          structured.nttp_binding_metadata_miss ||
              text.nttp_binding_metadata_miss);
    }

    return lookup(n->name);
  }

 private:
  static std::optional<ConstEvalStructuredNameKey> local_key(const Node* n) {
    if (!n || n->unqualified_text_id == kInvalidText) return std::nullopt;
    if (n->is_global_qualified || n->n_qualifier_segments > 0) return std::nullopt;
    ConstEvalStructuredNameKey key;
    key.base_text_id = n->unqualified_text_id;
    return key;
  }

  static std::optional<ConstEvalStructuredNameKey> symbol_key(const Node* n) {
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

  static ConstEvalValueLookupResult lookup_text_map_status(
      const ConstTextMap* map,
      TextId text_id) {
    if (!map || text_id == kInvalidText) return {};
    auto it = map->find(text_id);
    if (it == map->end()) return {ConstEvalValueLookupStatus::Miss, 0};
    return {ConstEvalValueLookupStatus::Found, it->second};
  }

  static ConstEvalValueLookupResult lookup_key_map_status(
      const ConstStructuredMap* map,
      const std::optional<ConstEvalStructuredNameKey>& key) {
    if (!map || !key.has_value() || !key->valid()) return {};
    auto it = map->find(*key);
    if (it == map->end()) return {ConstEvalValueLookupStatus::Miss, 0};
    return {ConstEvalValueLookupStatus::Found, it->second};
  }

  std::optional<long long> lookup_rendered_nttp(const std::string& name) const {
    // Compatibility bridge for unqualified NTTP references after other value
    // domains had metadata but the NTTP binding itself did not. Covered NTTP
    // metadata misses must not reach this bridge.
    if (!nttp_bindings) return std::nullopt;
    auto it = nttp_bindings->find(name);
    return it != nttp_bindings->end() ? std::optional<long long>(it->second)
                                      : std::nullopt;
  }

  std::optional<long long> lookup_rendered_compatibility(
      const std::string& name,
      bool skip_enum,
      bool skip_local,
      bool skip_named,
      bool skip_nttp) const {
    // Metadata-rich misses close only the covered domain. Other rendered maps
    // remain no-metadata compatibility fallbacks for legacy handoffs.
    if (!skip_enum) {
      if (enum_scopes) {
        for (auto it = enum_scopes->rbegin(); it != enum_scopes->rend(); ++it) {
          auto sit = it->find(name);
          if (sit != it->end()) return sit->second;
        }
      }
      if (enum_consts) {
        auto it = enum_consts->find(name);
        if (it != enum_consts->end()) return it->second;
      }
    }
    if (!skip_local) {
      if (local_const_scopes) {
        for (auto it = local_const_scopes->rbegin();
             it != local_const_scopes->rend(); ++it) {
          auto sit = it->find(name);
          if (sit != it->end()) return sit->second;
        }
      }
      if (local_consts) {
        auto it = local_consts->find(name);
        if (it != local_consts->end()) return it->second;
      }
    }
    if (!skip_named && named_consts) {
      auto it = named_consts->find(name);
      if (it != named_consts->end()) return it->second;
    }
    if (!skip_nttp) return lookup_rendered_nttp(name);
    return std::nullopt;
  }

  ConstEvalValueLookupResult lookup_by_text(const Node* n) const {
    if (n && (n->is_global_qualified || n->n_qualifier_segments > 0)) {
      return {};
    }
    const TextId text_id = n ? n->unqualified_text_id : kInvalidText;
    if (text_id == kInvalidText) return {};
    bool saw_metadata = false;
    ConstEvalValueLookupResult miss;
    if (enum_scopes_by_text) {
      for (auto it = enum_scopes_by_text->rbegin(); it != enum_scopes_by_text->rend(); ++it) {
        ConstEvalValueLookupResult result = lookup_text_map_status(&*it, text_id);
        if (result.status == ConstEvalValueLookupStatus::Found) return result;
        if (result.status == ConstEvalValueLookupStatus::Miss) {
          saw_metadata = true;
          miss.enum_binding_metadata_miss = true;
        }
      }
    }
    if (auto result = lookup_text_map_status(enum_consts_by_text, text_id);
        result.status == ConstEvalValueLookupStatus::Found) {
      return result;
    } else {
      if (result.status == ConstEvalValueLookupStatus::Miss) {
        saw_metadata = true;
        miss.enum_binding_metadata_miss = true;
      }
    }
    if (local_const_scopes_by_text) {
      for (auto it = local_const_scopes_by_text->rbegin();
           it != local_const_scopes_by_text->rend(); ++it) {
        ConstEvalValueLookupResult result = lookup_text_map_status(&*it, text_id);
        if (result.status == ConstEvalValueLookupStatus::Found) return result;
        if (result.status == ConstEvalValueLookupStatus::Miss) {
          saw_metadata = true;
          miss.local_binding_metadata_miss = true;
        }
      }
    }
    if (auto result = lookup_text_map_status(local_consts_by_text, text_id);
        result.status == ConstEvalValueLookupStatus::Found) {
      return result;
    } else {
      if (result.status == ConstEvalValueLookupStatus::Miss) {
        saw_metadata = true;
        miss.local_binding_metadata_miss = true;
      }
    }
    if (auto result = lookup_text_map_status(named_consts_by_text, text_id);
        result.status == ConstEvalValueLookupStatus::Found) {
      return result;
    } else {
      if (result.status == ConstEvalValueLookupStatus::Miss) {
        saw_metadata = true;
        miss.named_binding_metadata_miss = true;
      }
    }
    if (auto result = lookup_text_map_status(nttp_bindings_by_text, text_id);
        result.status == ConstEvalValueLookupStatus::Found) {
      return result;
    } else {
      if (result.status == ConstEvalValueLookupStatus::Miss) {
        saw_metadata = true;
        miss.nttp_binding_metadata_miss = true;
      }
    }
    if (saw_metadata) {
      miss.status = ConstEvalValueLookupStatus::Miss;
      return miss;
    }
    return {};
  }

  ConstEvalValueLookupResult lookup_by_key(const Node* n) const {
    const auto local = local_key(n);
    const auto symbol = symbol_key(n);
    bool saw_metadata = false;
    ConstEvalValueLookupResult miss;
    if (enum_scopes_by_key) {
      for (auto it = enum_scopes_by_key->rbegin(); it != enum_scopes_by_key->rend(); ++it) {
        ConstEvalValueLookupResult result = lookup_key_map_status(&*it, local);
        if (result.status == ConstEvalValueLookupStatus::Found) return result;
        if (result.status == ConstEvalValueLookupStatus::Miss) {
          saw_metadata = true;
          miss.enum_binding_metadata_miss = true;
        }
      }
    }
    if (auto result = lookup_key_map_status(enum_consts_by_key, symbol);
        result.status == ConstEvalValueLookupStatus::Found) {
      return result;
    } else {
      if (result.status == ConstEvalValueLookupStatus::Miss) {
        saw_metadata = true;
        miss.enum_binding_metadata_miss = true;
      }
    }
    if (local_const_scopes_by_key) {
      for (auto it = local_const_scopes_by_key->rbegin();
           it != local_const_scopes_by_key->rend(); ++it) {
        ConstEvalValueLookupResult result = lookup_key_map_status(&*it, local);
        if (result.status == ConstEvalValueLookupStatus::Found) return result;
        if (result.status == ConstEvalValueLookupStatus::Miss) {
          saw_metadata = true;
          miss.local_binding_metadata_miss = true;
        }
      }
    }
    if (auto result = lookup_key_map_status(local_consts_by_key, local);
        result.status == ConstEvalValueLookupStatus::Found) {
      return result;
    } else {
      if (result.status == ConstEvalValueLookupStatus::Miss) {
        saw_metadata = true;
        miss.local_binding_metadata_miss = true;
      }
    }
    if (auto result = lookup_key_map_status(named_consts_by_key, symbol);
        result.status == ConstEvalValueLookupStatus::Found) {
      return result;
    } else {
      if (result.status == ConstEvalValueLookupStatus::Miss) {
        saw_metadata = true;
        miss.named_binding_metadata_miss = true;
      }
    }
    if (auto result = lookup_key_map_status(nttp_bindings_by_key, local);
        result.status == ConstEvalValueLookupStatus::Found) {
      return result;
    } else {
      if (result.status == ConstEvalValueLookupStatus::Miss) {
        saw_metadata = true;
        miss.nttp_binding_metadata_miss = true;
      }
    }
    if (saw_metadata) {
      miss.status = ConstEvalValueLookupStatus::Miss;
      return miss;
    }
    return {};
  }
};

// ── Unified constant-expression evaluation API ───────────────────────────────

ConstEvalResult evaluate_constant_expr(const Node* n, const ConstEvalEnv& env);

// ── Consteval function-body interpreter ──────────────────────────────────────

// Evaluate a consteval function call at compile time.
// `func_def` must be an NK_FUNCTION node with is_consteval=true.
// `args` are the constant values for each parameter.
// `consteval_fns` maps rendered function names to NK_FUNCTION AST nodes for
// no-metadata recursive/chained consteval calls. The optional TextId/key maps
// are authority for metadata-rich call sites; the rendered map is removable
// once all callers provide those maps.
ConstEvalResult evaluate_consteval_call(
    const Node* func_def,
    const std::vector<ConstValue>& args,
    const ConstEvalEnv& env,
    const std::unordered_map<std::string, const Node*>& consteval_fns,
    int depth = 0,
    const ConstEvalFunctionTextMap* consteval_fns_by_text = nullptr,
    const ConstEvalFunctionStructuredMap* consteval_fns_by_key = nullptr);

// Apply explicit template arguments from a consteval call-site onto an
// evaluation environment so template/NTTP-dependent consteval bodies can be
// interpreted outside HIR lowering as well.
ConstEvalEnv bind_consteval_call_env(
    const Node* callee_expr,
    const Node* func_def,
    const ConstEvalEnv& outer_env,
    TypeBindings* out_type_bindings,
    std::unordered_map<std::string, long long>* out_nttp_bindings,
    TypeBindingTextMap* out_type_bindings_by_text = nullptr,
    TypeBindingStructuredMap* out_type_bindings_by_key = nullptr,
    TypeBindingNameTextMap* out_type_binding_text_ids_by_name = nullptr,
    TypeBindingNameStructuredMap* out_type_binding_keys_by_name = nullptr,
    ConstTextMap* out_nttp_bindings_by_text = nullptr,
    ConstStructuredMap* out_nttp_bindings_by_key = nullptr);

// ── String literal helpers ───────────────────────────────────────────────────

std::vector<long long> decode_string_literal_values(const char* sval, bool wide);
std::string bytes_from_string_literal(const StringLiteral& sl);

}  // namespace c4c::hir
