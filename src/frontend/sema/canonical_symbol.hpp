#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.hpp"
#include "source_profile.hpp"

namespace tinyc2ll::frontend_cxx::sema {

enum class LanguageLinkage : uint8_t {
  C,
  Cxx,
};

enum class CanonicalScopeKind : uint8_t {
  TranslationUnit,
  Namespace,
  Record,
  Function,
  Block,
};

struct CanonicalScopeSegment {
  CanonicalScopeKind kind = CanonicalScopeKind::TranslationUnit;
  std::string name;
};

struct CanonicalScope {
  std::vector<CanonicalScopeSegment> segments;
};

enum class CanonicalTypeKind : uint8_t {
  Void,
  Bool,
  Char,
  SignedChar,
  UnsignedChar,
  Short,
  UnsignedShort,
  Int,
  UnsignedInt,
  Long,
  UnsignedLong,
  LongLong,
  UnsignedLongLong,
  Float,
  Double,
  LongDouble,
  Int128,
  UInt128,
  VaList,
  Struct,
  Union,
  Enum,
  TypedefName,
  Complex,
  VendorExtended,
  Pointer,
  Array,
  Function,
};

struct CanonicalType;

struct CanonicalTemplateArg {
  std::shared_ptr<CanonicalType> type;
  std::optional<long long> integral_value;
};

struct CanonicalFunctionSig {
  std::shared_ptr<CanonicalType> return_type;
  std::vector<CanonicalType> params;
  bool is_variadic = false;
  bool unspecified_params = false;
};

struct CanonicalType {
  CanonicalTypeKind kind = CanonicalTypeKind::Void;
  std::string user_spelling;
  std::optional<TypeBase> source_base;

  bool is_const = false;
  bool is_volatile = false;
  bool is_vector = false;
  long long vector_lanes = 0;
  long long vector_bytes = 0;

  long long array_size = -1;
  std::shared_ptr<CanonicalType> element_type;
  std::shared_ptr<CanonicalFunctionSig> function_sig;
};

enum class CanonicalSymbolKind : uint8_t {
  Function,
  Object,
  Type,
};

struct CanonicalSymbol {
  CanonicalSymbolKind kind = CanonicalSymbolKind::Object;
  std::string source_name;
  CanonicalScope scope;
  LanguageLinkage linkage = LanguageLinkage::C;
  SourceProfile source_profile = SourceProfile::C;
  int line = 0;

  std::shared_ptr<CanonicalType> type;
  std::vector<CanonicalTemplateArg> template_args;
};

/// Per-Node resolved type table.  Populated during build_canonical_symbols for
/// every declaration node (functions, globals, params, struct/enum defs).
/// Downstream stages can query this instead of re-deriving types from TypeSpec.
struct ResolvedTypeTable {
  std::unordered_map<const Node*, std::shared_ptr<CanonicalType>> types;

  /// Look up the canonical type for a given AST node.  Returns nullptr if not
  /// recorded.
  std::shared_ptr<CanonicalType> lookup(const Node* node) const {
    auto it = types.find(node);
    return it != types.end() ? it->second : nullptr;
  }

  /// Record a canonical type for a node.
  void record(const Node* node, std::shared_ptr<CanonicalType> type) {
    if (node && type) types[node] = std::move(type);
  }
};

struct SemaCanonicalResult {
  std::vector<CanonicalSymbol> symbols;
  ResolvedTypeTable resolved_types;
};

CanonicalType canonicalize_type(const TypeSpec& ts);
CanonicalType canonicalize_declarator_type(const Node* decl_like_node);
SemaCanonicalResult build_canonical_symbols(const Node* root,
                                            SourceProfile profile = SourceProfile::C);

/// Format a canonical type as a human-readable string for debugging.
std::string format_canonical_type(const CanonicalType& type);

/// Format the full SemaCanonicalResult (symbols + resolved types summary).
std::string format_canonical_result(const SemaCanonicalResult& result);

// ── Phase 3: callable/prototype helpers ──────────────────────────────────────

/// Returns true if the canonical type represents a callable function pointer
/// (i.e. Pointer → Function).
bool is_callable_type(const CanonicalType& ct);

/// Returns the function signature from a canonical type.
/// Works for both Function types and Pointer(Function) types.
/// Returns nullptr if the type is not callable.
const CanonicalFunctionSig* get_function_sig(const CanonicalType& ct);

/// Convert a canonical type back to a TypeSpec.
/// The returned TypeSpec's tag field points into the canonical type's
/// user_spelling storage, so the canonical type must outlive the TypeSpec.
TypeSpec typespec_from_canonical(const CanonicalType& ct);

// ── Phase 5: canonical declaration identity ──────────────────────────────────

/// Structural equality comparison for canonical types.
/// Two types are equal when they have the same recursive structure,
/// qualifiers, and nominal names.
bool types_equal(const CanonicalType& a, const CanonicalType& b);

/// Check whether two function prototypes are compatible.
/// Unspecified parameter lists are compatible with any parameter list.
/// Otherwise, return type, parameter count, parameter types, and
/// variadic flag must match.
bool prototypes_compatible(const CanonicalType& a, const CanonicalType& b);

// ── Phase 6: Itanium ABI mangling ────────────────────────────────────────────

/// Produce a mangled identifier for a canonical symbol using Itanium ABI rules.
/// For extern "C" linkage, returns the source name unchanged.
/// For C++ linkage, produces an Itanium-style mangled name (_Z prefix, encoded
/// parameter types for functions, nested names for scoped symbols).
std::string mangle_symbol(const CanonicalSymbol& sym);

/// Encode a canonical type using Itanium ABI type encoding.
/// Useful for type-level mangling (e.g., function pointer types, template args).
std::string mangle_type(const CanonicalType& ct);

/// A unique declaration identity combining scope, name, kind, and linkage.
/// In C, two declarations at file scope with the same name and compatible
/// types refer to the same entity.  In C++, overload discrimination will
/// additionally consider canonical function type.
struct CanonicalIdentity {
  CanonicalSymbolKind kind = CanonicalSymbolKind::Object;
  std::string name;
  LanguageLinkage linkage = LanguageLinkage::C;
  /// For C++ overload discrimination; unused for C linkage.
  std::shared_ptr<CanonicalType> discriminator_type;

  bool operator==(const CanonicalIdentity& o) const;
  bool operator!=(const CanonicalIdentity& o) const { return !(*this == o); }
};

struct CanonicalIdentityHash {
  std::size_t operator()(const CanonicalIdentity& id) const;
};

/// Build a CanonicalIdentity from a CanonicalSymbol.
CanonicalIdentity identity_of(const CanonicalSymbol& sym);

/// Extended sema result with identity-based symbol deduplication.
/// Maps each unique identity to a single canonical symbol (the most complete
/// definition wins over forward declarations).
struct CanonicalSymbolTable {
  std::unordered_map<CanonicalIdentity, CanonicalSymbol, CanonicalIdentityHash> by_identity;

  /// Insert or merge a symbol.  If a symbol with the same identity already
  /// exists, the one with a function body (definition) takes precedence over
  /// a forward declaration.  Type compatibility is verified.
  void insert_or_merge(CanonicalSymbol sym);

  /// Look up a symbol by identity.  Returns nullptr if not found.
  const CanonicalSymbol* lookup(const CanonicalIdentity& id) const;
};

/// Build canonical symbols with identity-based deduplication.
/// This is the Phase 5 entry point; it calls build_canonical_symbols
/// internally and then deduplicates into a CanonicalSymbolTable.
CanonicalSymbolTable build_symbol_table(const Node* root,
                                        SourceProfile profile = SourceProfile::C);

}  // namespace tinyc2ll::frontend_cxx::sema
