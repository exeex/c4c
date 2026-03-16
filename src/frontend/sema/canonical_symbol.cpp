#include "canonical_symbol.hpp"

#include <utility>

namespace tinyc2ll::frontend_cxx::sema {
namespace {

CanonicalType make_leaf_type(TypeBase base,
                             std::string spelling,
                             bool is_const,
                             bool is_volatile,
                             bool is_vector,
                             long long vector_lanes,
                             long long vector_bytes) {
  CanonicalType type{};
  type.user_spelling = std::move(spelling);
  type.source_base = base;
  type.is_const = is_const;
  type.is_volatile = is_volatile;
  type.is_vector = is_vector;
  type.vector_lanes = vector_lanes;
  type.vector_bytes = vector_bytes;

  switch (base) {
    case TB_VOID: type.kind = CanonicalTypeKind::Void; break;
    case TB_BOOL: type.kind = CanonicalTypeKind::Bool; break;
    case TB_CHAR: type.kind = CanonicalTypeKind::Char; break;
    case TB_SCHAR: type.kind = CanonicalTypeKind::SignedChar; break;
    case TB_UCHAR: type.kind = CanonicalTypeKind::UnsignedChar; break;
    case TB_SHORT: type.kind = CanonicalTypeKind::Short; break;
    case TB_USHORT: type.kind = CanonicalTypeKind::UnsignedShort; break;
    case TB_INT: type.kind = CanonicalTypeKind::Int; break;
    case TB_UINT: type.kind = CanonicalTypeKind::UnsignedInt; break;
    case TB_LONG: type.kind = CanonicalTypeKind::Long; break;
    case TB_ULONG: type.kind = CanonicalTypeKind::UnsignedLong; break;
    case TB_LONGLONG: type.kind = CanonicalTypeKind::LongLong; break;
    case TB_ULONGLONG: type.kind = CanonicalTypeKind::UnsignedLongLong; break;
    case TB_FLOAT: type.kind = CanonicalTypeKind::Float; break;
    case TB_DOUBLE: type.kind = CanonicalTypeKind::Double; break;
    case TB_LONGDOUBLE: type.kind = CanonicalTypeKind::LongDouble; break;
    case TB_INT128: type.kind = CanonicalTypeKind::Int128; break;
    case TB_UINT128: type.kind = CanonicalTypeKind::UInt128; break;
    case TB_VA_LIST: type.kind = CanonicalTypeKind::VaList; break;
    case TB_STRUCT: type.kind = CanonicalTypeKind::Struct; break;
    case TB_UNION: type.kind = CanonicalTypeKind::Union; break;
    case TB_ENUM: type.kind = CanonicalTypeKind::Enum; break;
    case TB_TYPEDEF: type.kind = CanonicalTypeKind::TypedefName; break;
    case TB_COMPLEX_FLOAT:
    case TB_COMPLEX_DOUBLE:
    case TB_COMPLEX_LONGDOUBLE:
    case TB_COMPLEX_CHAR:
    case TB_COMPLEX_SCHAR:
    case TB_COMPLEX_UCHAR:
    case TB_COMPLEX_SHORT:
    case TB_COMPLEX_USHORT:
    case TB_COMPLEX_INT:
    case TB_COMPLEX_UINT:
    case TB_COMPLEX_LONG:
    case TB_COMPLEX_ULONG:
    case TB_COMPLEX_LONGLONG:
    case TB_COMPLEX_ULONGLONG:
      type.kind = CanonicalTypeKind::Complex;
      break;
    case TB_FUNC_PTR:
      type.kind = CanonicalTypeKind::VendorExtended;
      break;
  }

  return type;
}

CanonicalType wrap_pointer(CanonicalType pointee) {
  CanonicalType type{};
  type.kind = CanonicalTypeKind::Pointer;
  type.element_type = std::make_shared<CanonicalType>(std::move(pointee));
  return type;
}

CanonicalType wrap_array(CanonicalType element, long long dim) {
  CanonicalType type{};
  type.kind = CanonicalTypeKind::Array;
  type.array_size = dim;
  type.element_type = std::make_shared<CanonicalType>(std::move(element));
  return type;
}

CanonicalType wrap_function(CanonicalType return_type,
                            std::vector<CanonicalType> params,
                            bool is_variadic,
                            bool unspecified_params) {
  CanonicalType type{};
  type.kind = CanonicalTypeKind::Function;
  auto sig = std::make_shared<CanonicalFunctionSig>();
  sig->return_type = std::make_shared<CanonicalType>(std::move(return_type));
  sig->params = std::move(params);
  sig->is_variadic = is_variadic;
  sig->unspecified_params = unspecified_params;
  type.function_sig = std::move(sig);
  return type;
}

LanguageLinkage default_linkage_for(SourceProfile profile) {
  return profile == SourceProfile::C ? LanguageLinkage::C : LanguageLinkage::Cxx;
}

CanonicalScope translation_unit_scope() {
  CanonicalScope scope{};
  scope.segments.push_back({CanonicalScopeKind::TranslationUnit, {}});
  return scope;
}

bool is_unspecified_param_list(const Node* fn_like_node) {
  if (!fn_like_node) return false;
  return fn_like_node->n_params == 0 && !fn_like_node->variadic;
}

bool is_unspecified_fn_ptr_param_list(const Node* decl_like_node) {
  if (!decl_like_node) return false;
  return decl_like_node->n_fn_ptr_params == 0 && !decl_like_node->fn_ptr_variadic;
}

CanonicalType canonicalize_base_type(const TypeSpec& ts) {
  return make_leaf_type(ts.base, ts.tag ? ts.tag : "", ts.is_const, ts.is_volatile,
                        ts.is_vector, ts.vector_lanes, ts.vector_bytes);
}

CanonicalType apply_array_layers(CanonicalType type, const TypeSpec& ts) {
  for (int i = ts.array_rank - 1; i >= 0; --i) {
    type = wrap_array(std::move(type), ts.array_dims[i]);
  }
  return type;
}

CanonicalType apply_pointer_layers(CanonicalType type, int pointer_depth) {
  for (int i = 0; i < pointer_depth; ++i) {
    type = wrap_pointer(std::move(type));
  }
  return type;
}

std::vector<CanonicalType> canonicalize_param_types(Node* const* params, int count) {
  std::vector<CanonicalType> out;
  for (int i = 0; i < count; ++i) {
    const Node* param = params ? params[i] : nullptr;
    if (!param) continue;
    out.push_back(canonicalize_declarator_type(param));
  }
  return out;
}

CanonicalType canonicalize_fn_ptr_type(const Node* decl_like_node) {
  TypeSpec return_ts = decl_like_node->type;
  if (return_ts.ptr_level > 0) --return_ts.ptr_level;
  return_ts.is_fn_ptr = false;
  CanonicalType return_type = canonicalize_type(return_ts);
  std::vector<CanonicalType> params =
      canonicalize_param_types(decl_like_node->fn_ptr_params, decl_like_node->n_fn_ptr_params);
  CanonicalType fn_type =
      wrap_function(std::move(return_type), std::move(params), decl_like_node->fn_ptr_variadic,
                    is_unspecified_fn_ptr_param_list(decl_like_node));
  return wrap_pointer(std::move(fn_type));
}

CanonicalType canonicalize_function_node_type(const Node* fn_node) {
  CanonicalType return_type;
  // If the function returns a function pointer, the fn_ptr info is on the
  // function node itself (fn_ptr_params, fn_ptr_variadic).
  if (fn_node->type.is_fn_ptr) {
    return_type = canonicalize_fn_ptr_type(fn_node);
  } else {
    return_type = canonicalize_type(fn_node->type);
  }
  std::vector<CanonicalType> params =
      canonicalize_param_types(fn_node->params, fn_node->n_params);
  return wrap_function(std::move(return_type), std::move(params), fn_node->variadic,
                       is_unspecified_param_list(fn_node));
}

void record_param_types(const Node* fn_node, ResolvedTypeTable* table) {
  if (!fn_node || !table) return;
  for (int i = 0; i < fn_node->n_params; ++i) {
    const Node* param = fn_node->params ? fn_node->params[i] : nullptr;
    if (!param) continue;
    auto ct = std::make_shared<CanonicalType>(canonicalize_declarator_type(param));
    table->record(param, std::move(ct));
  }
}

void collect_top_level_symbol(const Node* item,
                              SourceProfile profile,
                              std::vector<CanonicalSymbol>* out,
                              ResolvedTypeTable* resolved) {
  if (!item || !out) return;

  if (item->kind == NK_FUNCTION || item->kind == NK_GLOBAL_VAR ||
      item->kind == NK_STRUCT_DEF || item->kind == NK_ENUM_DEF) {
    CanonicalSymbol symbol{};
    symbol.scope = translation_unit_scope();
    symbol.linkage = default_linkage_for(profile);
    symbol.source_profile = profile;
    symbol.line = item->line;

    if (item->kind == NK_FUNCTION) {
      symbol.kind = CanonicalSymbolKind::Function;
      symbol.source_name = item->name ? item->name : "<anon_fn>";
      symbol.type = std::make_shared<CanonicalType>(canonicalize_declarator_type(item));
      // Record parameter types.
      if (resolved) record_param_types(item, resolved);
    } else if (item->kind == NK_GLOBAL_VAR) {
      symbol.kind = CanonicalSymbolKind::Object;
      symbol.source_name = item->name ? item->name : "<anon_global>";
      symbol.type = std::make_shared<CanonicalType>(canonicalize_declarator_type(item));
    } else if (item->kind == NK_STRUCT_DEF) {
      symbol.kind = CanonicalSymbolKind::Type;
      symbol.source_name = item->name ? item->name : "<anon_struct>";
      CanonicalType ct{};
      ct.kind = item->type.base == TB_UNION ? CanonicalTypeKind::Union
                                            : CanonicalTypeKind::Struct;
      ct.user_spelling = item->name ? item->name : "";
      symbol.type = std::make_shared<CanonicalType>(std::move(ct));
    } else {
      // NK_ENUM_DEF
      symbol.kind = CanonicalSymbolKind::Type;
      symbol.source_name = item->name ? item->name : "<anon_enum>";
      CanonicalType ct{};
      ct.kind = CanonicalTypeKind::Enum;
      ct.user_spelling = item->name ? item->name : "";
      symbol.type = std::make_shared<CanonicalType>(std::move(ct));
    }

    // Record the node's resolved type.
    if (resolved) resolved->record(item, symbol.type);

    out->push_back(std::move(symbol));
  }
}

std::string canonical_type_kind_str(CanonicalTypeKind kind) {
  switch (kind) {
    case CanonicalTypeKind::Void: return "void";
    case CanonicalTypeKind::Bool: return "_Bool";
    case CanonicalTypeKind::Char: return "char";
    case CanonicalTypeKind::SignedChar: return "signed char";
    case CanonicalTypeKind::UnsignedChar: return "unsigned char";
    case CanonicalTypeKind::Short: return "short";
    case CanonicalTypeKind::UnsignedShort: return "unsigned short";
    case CanonicalTypeKind::Int: return "int";
    case CanonicalTypeKind::UnsignedInt: return "unsigned int";
    case CanonicalTypeKind::Long: return "long";
    case CanonicalTypeKind::UnsignedLong: return "unsigned long";
    case CanonicalTypeKind::LongLong: return "long long";
    case CanonicalTypeKind::UnsignedLongLong: return "unsigned long long";
    case CanonicalTypeKind::Float: return "float";
    case CanonicalTypeKind::Double: return "double";
    case CanonicalTypeKind::LongDouble: return "long double";
    case CanonicalTypeKind::Int128: return "__int128";
    case CanonicalTypeKind::UInt128: return "unsigned __int128";
    case CanonicalTypeKind::VaList: return "__builtin_va_list";
    case CanonicalTypeKind::Struct: return "struct";
    case CanonicalTypeKind::Union: return "union";
    case CanonicalTypeKind::Enum: return "enum";
    case CanonicalTypeKind::TypedefName: return "typedef";
    case CanonicalTypeKind::Complex: return "_Complex";
    case CanonicalTypeKind::VendorExtended: return "<vendor>";
    case CanonicalTypeKind::Pointer: return "pointer";
    case CanonicalTypeKind::Array: return "array";
    case CanonicalTypeKind::Function: return "function";
  }
  return "?";
}

}  // namespace

CanonicalType canonicalize_type(const TypeSpec& ts) {
  CanonicalType type = canonicalize_base_type(ts);
  type = apply_array_layers(std::move(type), ts);
  type = apply_pointer_layers(std::move(type), ts.ptr_level);
  return type;
}

CanonicalType canonicalize_declarator_type(const Node* decl_like_node) {
  if (!decl_like_node) return CanonicalType{};

  if (decl_like_node->kind == NK_FUNCTION) {
    return canonicalize_function_node_type(decl_like_node);
  }

  if (decl_like_node->type.is_fn_ptr) {
    return canonicalize_fn_ptr_type(decl_like_node);
  }

  return canonicalize_type(decl_like_node->type);
}

SemaCanonicalResult build_canonical_symbols(const Node* root, SourceProfile profile) {
  SemaCanonicalResult result{};
  if (!root) return result;

  if (root->kind != NK_PROGRAM) {
    collect_top_level_symbol(root, profile, &result.symbols, &result.resolved_types);
    return result;
  }

  for (int i = 0; i < root->n_children; ++i) {
    const Node* item = root->children ? root->children[i] : nullptr;
    collect_top_level_symbol(item, profile, &result.symbols, &result.resolved_types);
  }
  return result;
}

std::string format_canonical_type(const CanonicalType& type) {
  std::string result;

  if (type.is_const) result += "const ";
  if (type.is_volatile) result += "volatile ";

  if (type.kind == CanonicalTypeKind::Pointer) {
    result += "ptr(";
    if (type.element_type)
      result += format_canonical_type(*type.element_type);
    result += ")";
  } else if (type.kind == CanonicalTypeKind::Array) {
    result += "array[";
    result += std::to_string(type.array_size);
    result += "](";
    if (type.element_type)
      result += format_canonical_type(*type.element_type);
    result += ")";
  } else if (type.kind == CanonicalTypeKind::Function) {
    result += "fn(";
    if (type.function_sig) {
      if (type.function_sig->return_type)
        result += format_canonical_type(*type.function_sig->return_type);
      result += ")(";
      for (size_t i = 0; i < type.function_sig->params.size(); ++i) {
        if (i > 0) result += ", ";
        result += format_canonical_type(type.function_sig->params[i]);
      }
      if (type.function_sig->is_variadic) {
        if (!type.function_sig->params.empty()) result += ", ";
        result += "...";
      }
      if (type.function_sig->unspecified_params) result += "/*unspecified*/";
      result += ")";
    }
  } else {
    result += canonical_type_kind_str(type.kind);
    if (!type.user_spelling.empty()) {
      result += " '";
      result += type.user_spelling;
      result += "'";
    }
  }

  if (type.is_vector) {
    result += " __vector(";
    result += std::to_string(type.vector_bytes);
    result += ")";
  }

  return result;
}

std::string format_canonical_result(const SemaCanonicalResult& result) {
  std::string out;
  out += "=== Canonical Symbols ===\n";
  for (const auto& sym : result.symbols) {
    switch (sym.kind) {
      case CanonicalSymbolKind::Function: out += "  fn "; break;
      case CanonicalSymbolKind::Object: out += "  obj "; break;
      case CanonicalSymbolKind::Type: out += "  type "; break;
    }
    out += sym.source_name;
    if (sym.type) {
      out += " : ";
      out += format_canonical_type(*sym.type);
    }
    out += "\n";
  }
  out += "=== Resolved Type Table (" +
         std::to_string(result.resolved_types.types.size()) + " entries) ===\n";
  return out;
}

// ── Phase 3: callable/prototype helpers ──────────────────────────────────────

bool is_callable_type(const CanonicalType& ct) {
  if (ct.kind == CanonicalTypeKind::Function) return true;
  if (ct.kind == CanonicalTypeKind::Pointer && ct.element_type &&
      ct.element_type->kind == CanonicalTypeKind::Function)
    return true;
  return false;
}

const CanonicalFunctionSig* get_function_sig(const CanonicalType& ct) {
  if (ct.kind == CanonicalTypeKind::Function) return ct.function_sig.get();
  if (ct.kind == CanonicalTypeKind::Pointer && ct.element_type &&
      ct.element_type->kind == CanonicalTypeKind::Function)
    return ct.element_type->function_sig.get();
  return nullptr;
}

/// Map CanonicalTypeKind back to TypeBase when source_base is absent.
static TypeBase typebase_from_kind(CanonicalTypeKind kind) {
  switch (kind) {
    case CanonicalTypeKind::Void: return TB_VOID;
    case CanonicalTypeKind::Bool: return TB_BOOL;
    case CanonicalTypeKind::Char: return TB_CHAR;
    case CanonicalTypeKind::SignedChar: return TB_SCHAR;
    case CanonicalTypeKind::UnsignedChar: return TB_UCHAR;
    case CanonicalTypeKind::Short: return TB_SHORT;
    case CanonicalTypeKind::UnsignedShort: return TB_USHORT;
    case CanonicalTypeKind::Int: return TB_INT;
    case CanonicalTypeKind::UnsignedInt: return TB_UINT;
    case CanonicalTypeKind::Long: return TB_LONG;
    case CanonicalTypeKind::UnsignedLong: return TB_ULONG;
    case CanonicalTypeKind::LongLong: return TB_LONGLONG;
    case CanonicalTypeKind::UnsignedLongLong: return TB_ULONGLONG;
    case CanonicalTypeKind::Float: return TB_FLOAT;
    case CanonicalTypeKind::Double: return TB_DOUBLE;
    case CanonicalTypeKind::LongDouble: return TB_LONGDOUBLE;
    case CanonicalTypeKind::Int128: return TB_INT128;
    case CanonicalTypeKind::UInt128: return TB_UINT128;
    case CanonicalTypeKind::VaList: return TB_VA_LIST;
    case CanonicalTypeKind::Struct: return TB_STRUCT;
    case CanonicalTypeKind::Union: return TB_UNION;
    case CanonicalTypeKind::Enum: return TB_ENUM;
    case CanonicalTypeKind::TypedefName: return TB_TYPEDEF;
    case CanonicalTypeKind::Complex: return TB_COMPLEX_DOUBLE;
    case CanonicalTypeKind::VendorExtended: return TB_FUNC_PTR;
    case CanonicalTypeKind::Pointer: return TB_INT;   // shouldn't reach
    case CanonicalTypeKind::Array: return TB_INT;      // shouldn't reach
    case CanonicalTypeKind::Function: return TB_FUNC_PTR;
  }
  return TB_INT;
}

TypeSpec typespec_from_canonical(const CanonicalType& ct) {
  TypeSpec ts{};
  ts.tag = nullptr;
  ts.ptr_level = 0;
  ts.align_bytes = 0;
  ts.array_size = -1;
  ts.array_rank = 0;
  ts.is_ptr_to_array = false;
  ts.inner_rank = -1;
  ts.is_vector = false;
  ts.vector_lanes = 0;
  ts.vector_bytes = 0;
  ts.array_size_expr = nullptr;
  ts.is_const = false;
  ts.is_volatile = false;
  ts.is_fn_ptr = false;
  ts.is_packed = false;
  for (int i = 0; i < 8; ++i) ts.array_dims[i] = 0;

  if (ct.kind == CanonicalTypeKind::Pointer) {
    if (!ct.element_type) {
      ts.base = TB_VOID;
      ts.ptr_level = 1;
      return ts;
    }
    // Pointer → Function → encode as fn_ptr TypeSpec
    if (ct.element_type->kind == CanonicalTypeKind::Function) {
      const auto* sig = ct.element_type->function_sig.get();
      if (sig && sig->return_type) {
        ts = typespec_from_canonical(*sig->return_type);
      } else {
        ts.base = TB_VOID;
      }
      ts.ptr_level += 1;
      ts.is_fn_ptr = true;
      return ts;
    }
    // Pointer → Array → is_ptr_to_array
    if (ct.element_type->kind == CanonicalTypeKind::Array) {
      ts = typespec_from_canonical(*ct.element_type);
      ts.ptr_level += 1;
      ts.is_ptr_to_array = true;
      return ts;
    }
    // Regular pointer
    ts = typespec_from_canonical(*ct.element_type);
    ts.ptr_level += 1;
    return ts;
  }

  if (ct.kind == CanonicalTypeKind::Array) {
    if (!ct.element_type) {
      ts.base = TB_INT;
      return ts;
    }
    ts = typespec_from_canonical(*ct.element_type);
    // Prepend this array dimension
    if (ts.array_rank < 8) {
      for (int i = ts.array_rank; i > 0; --i) {
        ts.array_dims[i] = ts.array_dims[i - 1];
      }
      ts.array_dims[0] = ct.array_size;
      ts.array_rank += 1;
      ts.array_size = ts.array_dims[0];
    }
    return ts;
  }

  // Leaf type
  ts.base = ct.source_base ? *ct.source_base : typebase_from_kind(ct.kind);
  ts.is_const = ct.is_const;
  ts.is_volatile = ct.is_volatile;
  ts.is_vector = ct.is_vector;
  ts.vector_lanes = ct.vector_lanes;
  ts.vector_bytes = ct.vector_bytes;

  // For nominal types, point tag to the canonical type's user_spelling storage.
  // SAFETY: the canonical type must outlive the returned TypeSpec.
  if (!ct.user_spelling.empty() &&
      (ct.kind == CanonicalTypeKind::Struct || ct.kind == CanonicalTypeKind::Union ||
       ct.kind == CanonicalTypeKind::Enum || ct.kind == CanonicalTypeKind::TypedefName)) {
    ts.tag = ct.user_spelling.c_str();
  }

  return ts;
}

}  // namespace tinyc2ll::frontend_cxx::sema
