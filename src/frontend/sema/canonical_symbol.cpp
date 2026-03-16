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

CanonicalType canonicalize_function_node_type(const Node* fn_node) {
  CanonicalType return_type = canonicalize_type(fn_node->type);
  std::vector<CanonicalType> params =
      canonicalize_param_types(fn_node->params, fn_node->n_params);
  return wrap_function(std::move(return_type), std::move(params), fn_node->variadic,
                       is_unspecified_param_list(fn_node));
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

void collect_top_level_symbol(const Node* item,
                              SourceProfile profile,
                              std::vector<CanonicalSymbol>* out) {
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
    } else if (item->kind == NK_GLOBAL_VAR) {
      symbol.kind = CanonicalSymbolKind::Object;
      symbol.source_name = item->name ? item->name : "<anon_global>";
      symbol.type = std::make_shared<CanonicalType>(canonicalize_declarator_type(item));
    } else {
      symbol.kind = CanonicalSymbolKind::Type;
      symbol.source_name = item->name ? item->name : "<anon_type>";
      symbol.type = std::make_shared<CanonicalType>(canonicalize_type(item->type));
    }

    out->push_back(std::move(symbol));
  }
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
    collect_top_level_symbol(root, profile, &result.symbols);
    return result;
  }

  for (int i = 0; i < root->n_children; ++i) {
    const Node* item = root->children ? root->children[i] : nullptr;
    collect_top_level_symbol(item, profile, &result.symbols);
  }
  return result;
}

}  // namespace tinyc2ll::frontend_cxx::sema
