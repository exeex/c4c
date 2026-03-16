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

}  // namespace tinyc2ll::frontend_cxx::sema
