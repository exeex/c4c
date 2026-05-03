#include "canonical_symbol.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

namespace c4c::sema {
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

CanonicalType wrap_lvalue_reference(CanonicalType referred) {
  CanonicalType type{};
  type.kind = CanonicalTypeKind::LValueReference;
  type.element_type = std::make_shared<CanonicalType>(std::move(referred));
  return type;
}

CanonicalType wrap_rvalue_reference(CanonicalType referred) {
  CanonicalType type{};
  type.kind = CanonicalTypeKind::RValueReference;
  type.element_type = std::make_shared<CanonicalType>(std::move(referred));
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

LanguageLinkage linkage_for_node(const Node* node, SourceProfile profile) {
  if (node && node->linkage_spec && std::string_view(node->linkage_spec) == "C") {
    return LanguageLinkage::C;
  }
  return default_linkage_for(profile);
}

template <typename T>
auto typespec_legacy_display_tag_if_present(const T& ts, int) -> decltype(ts.tag) {
  return ts.tag;
}

const char* typespec_legacy_display_tag_if_present(const TypeSpec&, long) {
  return nullptr;
}

template <typename T>
auto set_typespec_final_spelling_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

void set_typespec_final_spelling_tag_if_present(TypeSpec&, const char*, long) {}

const char* record_definition_display_name(const Node* record_def) {
  if (!record_def) return nullptr;
  if (record_def->unqualified_name && record_def->unqualified_name[0]) {
    return record_def->unqualified_name;
  }
  if (record_def->name && record_def->name[0]) return record_def->name;
  return nullptr;
}

std::string canonical_leaf_display_spelling(const TypeSpec& ts) {
  if (const char* record_name = record_definition_display_name(ts.record_def)) {
    return record_name;
  }
  if (const char* compatibility_tag =
          typespec_legacy_display_tag_if_present(ts, 0)) {
    return compatibility_tag;
  }
  return {};
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
  return make_leaf_type(ts.base, canonical_leaf_display_spelling(ts), ts.is_const,
                        ts.is_volatile, ts.is_vector, ts.vector_lanes,
                        ts.vector_bytes);
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

CanonicalType apply_reference_layer(CanonicalType type, const TypeSpec& ts) {
  if (ts.is_lvalue_ref) {
    type = wrap_lvalue_reference(std::move(type));
  } else if (ts.is_rvalue_ref) {
    type = wrap_rvalue_reference(std::move(type));
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
  // Strip declarator array dims — they belong to the variable, not the fn_ptr return type.
  return_ts.array_rank = 0;
  return_ts.array_size = -1;
  for (int i = 0; i < 8; ++i) return_ts.array_dims[i] = -1;
  return_ts.is_ptr_to_array = false;
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

/// Recursively walk an AST subtree (function body) and record canonical types
/// for every NK_DECL node found.  This fills ResolvedTypeTable for locals so
/// that downstream lowering can use canonical signatures instead of falling
/// back to parser-field reconstruction.
void record_local_decl_types(const Node* node, ResolvedTypeTable* resolved) {
  if (!node || !resolved) return;

  if (node->kind == NK_DECL) {
    auto ct = std::make_shared<CanonicalType>(canonicalize_declarator_type(node));
    resolved->record(node, std::move(ct));
    // Don't return — NK_DECL can have an init subtree with nested decls
    // (e.g. statement expressions), so keep walking.
  }

  // Recurse into children (NK_BLOCK stmts, NK_IF branches, etc.)
  if (node->children) {
    for (int i = 0; i < node->n_children; ++i) {
      record_local_decl_types(node->children[i], resolved);
    }
  }
  // Recurse into body (NK_WHILE, NK_FOR, NK_IF, NK_SWITCH, NK_BLOCK, etc.)
  if (node->body) record_local_decl_types(node->body, resolved);
  // Recurse into then/else/cond branches (NK_IF, NK_TERNARY)
  if (node->then_) record_local_decl_types(node->then_, resolved);
  if (node->else_) record_local_decl_types(node->else_, resolved);
  if (node->cond) record_local_decl_types(node->cond, resolved);
  // Recurse into init (NK_FOR init, NK_DECL init)
  if (node->init) record_local_decl_types(node->init, resolved);
  // Recurse into left/right for expression trees that may contain stmt-exprs
  if (node->left) record_local_decl_types(node->left, resolved);
  if (node->right) record_local_decl_types(node->right, resolved);
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
    symbol.linkage = linkage_for_node(item, profile);
    symbol.source_profile = profile;
    symbol.line = item->line;
    for (int i = 0; i < item->n_template_params; ++i) {
      const char* pname = item->template_param_names ? item->template_param_names[i] : nullptr;
      if (!pname || !pname[0]) continue;
      symbol.template_params.push_back({pname});
    }

    if (item->kind == NK_FUNCTION) {
      symbol.kind = CanonicalSymbolKind::Function;
      symbol.source_name = item->name ? item->name : "<anon_fn>";
      symbol.type = std::make_shared<CanonicalType>(canonicalize_declarator_type(item));
      // Record parameter types.
      if (resolved) record_param_types(item, resolved);
      // Record local declaration types within the function body.
      if (resolved && item->body) record_local_decl_types(item->body, resolved);
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

CanonicalType substitute_template_args_impl(
    const CanonicalType& type,
    const std::unordered_map<std::string, CanonicalTemplateArg>& bindings) {
  if (type.kind == CanonicalTypeKind::TypedefName && !type.user_spelling.empty()) {
    auto it = bindings.find(type.user_spelling);
    if (it != bindings.end() && it->second.type) {
      CanonicalType substituted = *it->second.type;
      substituted.is_const = substituted.is_const || type.is_const;
      substituted.is_volatile = substituted.is_volatile || type.is_volatile;
      return substituted;
    }
  }

  CanonicalType out = type;
  if (type.element_type) {
    out.element_type = std::make_shared<CanonicalType>(
        substitute_template_args_impl(*type.element_type, bindings));
  }
  if (type.function_sig) {
    auto sig = std::make_shared<CanonicalFunctionSig>();
    sig->is_variadic = type.function_sig->is_variadic;
    sig->unspecified_params = type.function_sig->unspecified_params;
    if (type.function_sig->return_type) {
      sig->return_type = std::make_shared<CanonicalType>(
          substitute_template_args_impl(*type.function_sig->return_type, bindings));
    }
    for (const auto& param : type.function_sig->params) {
      sig->params.push_back(substitute_template_args_impl(param, bindings));
    }
    out.function_sig = std::move(sig);
  }
  return out;
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
    case CanonicalTypeKind::LValueReference: return "lvalue reference";
    case CanonicalTypeKind::RValueReference: return "rvalue reference";
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
  type = apply_reference_layer(std::move(type), ts);
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

std::string format_template_arg(const CanonicalTemplateArg& arg) {
  if (arg.type) return format_canonical_type(*arg.type);
  if (arg.integral_value) return std::to_string(*arg.integral_value);
  return "?";
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
    out += " : ";
    if (!sym.template_params.empty()) {
      out += "consteval (";
      for (size_t i = 0; i < sym.template_params.size(); ++i) {
        if (i > 0) out += ", ";
        out += "typename ";
        out += sym.template_params[i].name;
      }
      out += ") -> ";
    }
    if (!sym.template_args.empty()) {
      out += "apply<";
      for (size_t i = 0; i < sym.template_args.size(); ++i) {
        if (i > 0) out += ", ";
        out += format_template_arg(sym.template_args[i]);
      }
      out += "> ";
    }
    if (sym.type) {
      out += format_canonical_type(*sym.type);
    } else {
      out += "<null>";
    }
    if (sym.linkage == LanguageLinkage::C) {
      out += " [extern \"C\"]";
    }
    out += "\n";
  }
  out += "=== Resolved Type Table (" +
         std::to_string(result.resolved_types.types.size()) + " entries) ===\n";
  return out;
}

CanonicalType substitute_template_args(
    const CanonicalType& type,
    const std::vector<CanonicalTemplateParam>& params,
    const std::vector<CanonicalTemplateArg>& args) {
  std::unordered_map<std::string, CanonicalTemplateArg> bindings;
  const size_t count = std::min(params.size(), args.size());
  for (size_t i = 0; i < count; ++i) {
    bindings.emplace(params[i].name, args[i]);
  }
  return substitute_template_args_impl(type, bindings);
}

CanonicalSymbol instantiate_symbol(
    const CanonicalSymbol& sym,
    const std::vector<CanonicalTemplateArg>& args) {
  CanonicalSymbol instantiated = sym;
  if (!sym.template_params.empty() && sym.type) {
    instantiated.type = std::make_shared<CanonicalType>(
        substitute_template_args(*sym.type, sym.template_params, args));
  }
  const size_t consumed = std::min(sym.template_params.size(), args.size());
  instantiated.template_args.insert(instantiated.template_args.end(),
                                    args.begin(), args.begin() + consumed);
  if (consumed > 0 && consumed <= instantiated.template_params.size()) {
    instantiated.template_params.erase(instantiated.template_params.begin(),
                                       instantiated.template_params.begin() + consumed);
  }
  return instantiated;
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
    case CanonicalTypeKind::LValueReference: return TB_INT;   // shouldn't reach
    case CanonicalTypeKind::RValueReference: return TB_INT;  // shouldn't reach
    case CanonicalTypeKind::Array: return TB_INT;      // shouldn't reach
    case CanonicalTypeKind::Function: return TB_FUNC_PTR;
  }
  return TB_INT;
}

TypeSpec typespec_from_canonical(const CanonicalType& ct) {
  TypeSpec ts{};
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
  ts.is_lvalue_ref = false;
  ts.is_packed = false;
  for (int i = 0; i < 8; ++i) ts.array_dims[i] = 0;

  if (ct.kind == CanonicalTypeKind::LValueReference) {
    if (!ct.element_type) {
      ts.base = TB_INT;
      ts.is_lvalue_ref = true;
      return ts;
    }
    ts = typespec_from_canonical(*ct.element_type);
    ts.is_lvalue_ref = true;
    return ts;
  }

  if (ct.kind == CanonicalTypeKind::RValueReference) {
    if (!ct.element_type) {
      ts.base = TB_INT;
      ts.is_rvalue_ref = true;
      return ts;
    }
    ts = typespec_from_canonical(*ct.element_type);
    ts.is_rvalue_ref = true;
    return ts;
  }

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
    set_typespec_final_spelling_tag_if_present(ts, ct.user_spelling.c_str(), 0);
  }

  return ts;
}

// ── Phase 5: canonical declaration identity ──────────────────────────────────

bool types_equal(const CanonicalType& a, const CanonicalType& b) {
  if (a.kind != b.kind) return false;
  if (a.is_const != b.is_const) return false;
  if (a.is_volatile != b.is_volatile) return false;
  if (a.is_vector != b.is_vector) return false;
  if (a.is_vector && (a.vector_lanes != b.vector_lanes || a.vector_bytes != b.vector_bytes))
    return false;

  switch (a.kind) {
    case CanonicalTypeKind::Pointer:
    case CanonicalTypeKind::LValueReference:
    case CanonicalTypeKind::RValueReference:
      if (!a.element_type || !b.element_type) return a.element_type == b.element_type;
      return types_equal(*a.element_type, *b.element_type);

    case CanonicalTypeKind::Array:
      if (a.array_size != b.array_size) return false;
      if (!a.element_type || !b.element_type) return a.element_type == b.element_type;
      return types_equal(*a.element_type, *b.element_type);

    case CanonicalTypeKind::Function: {
      if (!a.function_sig || !b.function_sig) return a.function_sig == b.function_sig;
      const auto& sa = *a.function_sig;
      const auto& sb = *b.function_sig;
      if (sa.is_variadic != sb.is_variadic) return false;
      if (sa.unspecified_params != sb.unspecified_params) return false;
      if (sa.params.size() != sb.params.size()) return false;
      if (!sa.return_type || !sb.return_type) {
        if (sa.return_type != sb.return_type) return false;
      } else if (!types_equal(*sa.return_type, *sb.return_type)) {
        return false;
      }
      for (size_t i = 0; i < sa.params.size(); ++i) {
        if (!types_equal(sa.params[i], sb.params[i])) return false;
      }
      return true;
    }

    case CanonicalTypeKind::Struct:
    case CanonicalTypeKind::Union:
    case CanonicalTypeKind::Enum:
    case CanonicalTypeKind::TypedefName:
      return a.user_spelling == b.user_spelling;

    default:
      // Leaf primitive types: kind + qualifiers already compared above.
      return true;
  }
}

bool prototypes_compatible(const CanonicalType& a, const CanonicalType& b) {
  // Both must be function types to compare prototypes.
  const auto* sa = get_function_sig(a);
  const auto* sb = get_function_sig(b);
  if (!sa || !sb) return false;

  // Unspecified parameter lists are compatible with anything.
  if (sa->unspecified_params || sb->unspecified_params) {
    // Return types must still match.
    if (sa->return_type && sb->return_type) {
      return types_equal(*sa->return_type, *sb->return_type);
    }
    return true;
  }

  // Otherwise, full structural equality on the function type.
  // Peel pointer wrapper if present so we compare the Function nodes.
  const CanonicalType* fa = &a;
  const CanonicalType* fb = &b;
  if ((fa->kind == CanonicalTypeKind::Pointer ||
       fa->kind == CanonicalTypeKind::LValueReference ||
       fa->kind == CanonicalTypeKind::RValueReference) && fa->element_type)
    fa = fa->element_type.get();
  if ((fb->kind == CanonicalTypeKind::Pointer ||
       fb->kind == CanonicalTypeKind::LValueReference ||
       fb->kind == CanonicalTypeKind::RValueReference) && fb->element_type)
    fb = fb->element_type.get();
  return types_equal(*fa, *fb);
}

// ── Phase 6: Itanium ABI mangling ────────────────────────────────────────────

/// Encode a canonical type as an Itanium ABI type string.
/// Reference: https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling-type
static void mangle_type_impl(const CanonicalType& ct, std::string& out) {
  // Qualifiers: const → K, volatile → V (applied outermost-first,
  // but in Itanium ABI the order is: cv-qualifiers wrap the type,
  // so const volatile int → KVi, read as K(V(i))).
  // We emit qualifiers before the base type encoding.
  if (ct.is_const && ct.is_volatile) {
    out += "VK";  // volatile(const(type))
  } else if (ct.is_const) {
    out += "K";
  } else if (ct.is_volatile) {
    out += "V";
  }

  switch (ct.kind) {
    case CanonicalTypeKind::Void:          out += 'v'; return;
    case CanonicalTypeKind::Bool:          out += 'b'; return;
    case CanonicalTypeKind::Char:          out += 'c'; return;
    case CanonicalTypeKind::SignedChar:    out += 'a'; return;
    case CanonicalTypeKind::UnsignedChar:  out += 'h'; return;
    case CanonicalTypeKind::Short:         out += 's'; return;
    case CanonicalTypeKind::UnsignedShort: out += 't'; return;
    case CanonicalTypeKind::Int:           out += 'i'; return;
    case CanonicalTypeKind::UnsignedInt:   out += 'j'; return;
    case CanonicalTypeKind::Long:          out += 'l'; return;
    case CanonicalTypeKind::UnsignedLong:  out += 'm'; return;
    case CanonicalTypeKind::LongLong:      out += 'x'; return;
    case CanonicalTypeKind::UnsignedLongLong: out += 'y'; return;
    case CanonicalTypeKind::Float:         out += 'f'; return;
    case CanonicalTypeKind::Double:        out += 'd'; return;
    case CanonicalTypeKind::LongDouble:    out += 'e'; return;
    case CanonicalTypeKind::Int128:        out += 'n'; return;
    case CanonicalTypeKind::UInt128:       out += 'o'; return;

    case CanonicalTypeKind::VaList:
      // __builtin_va_list is typically a vendor type or struct; encode as
      // vendor extended type: u <len> <name>
      out += "u17__builtin_va_list";
      return;

    case CanonicalTypeKind::Complex:
      // Itanium ABI: C <base-type>
      out += 'C';
      if (ct.source_base) {
        switch (*ct.source_base) {
          case TB_COMPLEX_FLOAT:      out += 'f'; break;
          case TB_COMPLEX_DOUBLE:     out += 'd'; break;
          case TB_COMPLEX_LONGDOUBLE: out += 'e'; break;
          case TB_COMPLEX_CHAR:       out += 'c'; break;
          case TB_COMPLEX_SCHAR:      out += 'a'; break;
          case TB_COMPLEX_UCHAR:      out += 'h'; break;
          case TB_COMPLEX_SHORT:      out += 's'; break;
          case TB_COMPLEX_USHORT:     out += 't'; break;
          case TB_COMPLEX_INT:        out += 'i'; break;
          case TB_COMPLEX_UINT:       out += 'j'; break;
          case TB_COMPLEX_LONG:       out += 'l'; break;
          case TB_COMPLEX_ULONG:      out += 'm'; break;
          case TB_COMPLEX_LONGLONG:   out += 'x'; break;
          case TB_COMPLEX_ULONGLONG:  out += 'y'; break;
          default:                    out += 'd'; break;  // fallback to complex double
        }
      } else {
        out += 'd';  // default to complex double
      }
      return;

    case CanonicalTypeKind::Pointer:
      out += 'P';
      if (ct.element_type) {
        mangle_type_impl(*ct.element_type, out);
      } else {
        out += 'v';  // ptr to void
      }
      return;

    case CanonicalTypeKind::LValueReference:
      out += 'R';
      if (ct.element_type) {
        mangle_type_impl(*ct.element_type, out);
      } else {
        out += 'v';
      }
      return;

    case CanonicalTypeKind::RValueReference:
      out += 'O';  // Itanium ABI: O = rvalue reference
      if (ct.element_type) {
        mangle_type_impl(*ct.element_type, out);
      } else {
        out += 'v';
      }
      return;

    case CanonicalTypeKind::Array:
      // A <dimension> _ <element-type>
      out += 'A';
      if (ct.array_size >= 0) out += std::to_string(ct.array_size);
      out += '_';
      if (ct.element_type) {
        mangle_type_impl(*ct.element_type, out);
      } else {
        out += 'v';
      }
      return;

    case CanonicalTypeKind::Function:
      // F <return-type> <param-types> [v if no params] E
      out += 'F';
      if (ct.function_sig) {
        if (ct.function_sig->return_type) {
          mangle_type_impl(*ct.function_sig->return_type, out);
        } else {
          out += 'v';
        }
        if (ct.function_sig->params.empty() && !ct.function_sig->is_variadic) {
          // No parameters → encode as void
          // (but only if not variadic, which would be pathological)
        } else {
          for (const auto& p : ct.function_sig->params) {
            mangle_type_impl(p, out);
          }
        }
        if (ct.function_sig->is_variadic) out += 'z';
        out += 'E';
      } else {
        out += "vE";  // no sig → void() fallback
      }
      return;

    case CanonicalTypeKind::Struct:
    case CanonicalTypeKind::Union:
    case CanonicalTypeKind::Enum:
    case CanonicalTypeKind::TypedefName:
      // <source-name> ::= <positive length number> <identifier>
      out += std::to_string(ct.user_spelling.size());
      out += ct.user_spelling;
      return;

    case CanonicalTypeKind::VendorExtended:
      // u <len> <name>
      if (!ct.user_spelling.empty()) {
        out += 'u';
        out += std::to_string(ct.user_spelling.size());
        out += ct.user_spelling;
      } else {
        out += "u7unknown";  // fallback
      }
      return;
  }
}

/// Encode a single unqualified name: <source-name> ::= <length> <identifier>
static void mangle_unqualified_name(const std::string& name, std::string& out) {
  out += std::to_string(name.size());
  out += name;
}

/// Encode nested name if scope has namespace/record segments.
/// <nested-name> ::= N [<prefix>] <unqualified-name> E
static void mangle_name(const CanonicalSymbol& sym, std::string& out) {
  bool has_nesting = false;
  for (const auto& seg : sym.scope.segments) {
    if (seg.kind == CanonicalScopeKind::Namespace ||
        seg.kind == CanonicalScopeKind::Record) {
      has_nesting = true;
      break;
    }
  }

  if (has_nesting) {
    out += 'N';
    for (const auto& seg : sym.scope.segments) {
      if (seg.kind == CanonicalScopeKind::TranslationUnit) continue;
      if (!seg.name.empty()) mangle_unqualified_name(seg.name, out);
    }
    mangle_unqualified_name(sym.source_name, out);
    out += 'E';
  } else {
    mangle_unqualified_name(sym.source_name, out);
  }
}

/// Mangle a function symbol: _Z <name> <bare-function-type>
/// <bare-function-type> is the parameter types only (no return type for
/// non-template functions per Itanium ABI).
static void mangle_function(const CanonicalSymbol& sym, std::string& out) {
  out += "_Z";
  mangle_name(sym, out);

  // Encode parameter types (bare-function-type).
  const auto* sig = sym.type ? get_function_sig(*sym.type) : nullptr;
  if (sig) {
    if (sig->params.empty() && !sig->is_variadic) {
      out += 'v';  // void parameter list → "v"
    } else {
      for (const auto& p : sig->params) {
        mangle_type_impl(p, out);
      }
      if (sig->is_variadic) out += 'z';
    }
  } else {
    out += 'v';  // unknown sig → treat as void()
  }
}

/// Mangle an object (variable) symbol: _Z <name>
static void mangle_object(const CanonicalSymbol& sym, std::string& out) {
  out += "_Z";
  mangle_name(sym, out);
}

std::string mangle_symbol(const CanonicalSymbol& sym) {
  // extern "C" linkage: no mangling, return source name as-is.
  if (sym.linkage == LanguageLinkage::C) {
    return sym.source_name;
  }

  // C++ linkage: Itanium ABI mangling.
  std::string mangled;
  if (sym.kind == CanonicalSymbolKind::Function) {
    mangle_function(sym, mangled);
  } else if (sym.kind == CanonicalSymbolKind::Object) {
    mangle_object(sym, mangled);
  } else {
    // Type symbols (struct/enum defs) are not typically mangled as linker
    // symbols; return the source name as a fallback.
    return sym.source_name;
  }
  return mangled;
}

/// Encode a canonical type using Itanium ABI encoding.
/// This is the public entry point for type mangling.
std::string mangle_type(const CanonicalType& ct) {
  std::string out;
  mangle_type_impl(ct, out);
  return out;
}

bool CanonicalIdentity::operator==(const CanonicalIdentity& o) const {
  if (kind != o.kind) return false;
  if (name != o.name) return false;
  if (linkage != o.linkage) return false;
  // For C linkage, name+kind is sufficient identity.
  if (linkage == LanguageLinkage::C) return true;
  // For C++ linkage, function overloads need type discrimination.
  if (kind == CanonicalSymbolKind::Function) {
    if (!discriminator_type && !o.discriminator_type) return true;
    if (!discriminator_type || !o.discriminator_type) return false;
    return types_equal(*discriminator_type, *o.discriminator_type);
  }
  return true;
}

std::size_t CanonicalIdentityHash::operator()(const CanonicalIdentity& id) const {
  std::size_t h = std::hash<std::string>{}(id.name);
  h ^= std::hash<uint8_t>{}(static_cast<uint8_t>(id.kind)) << 1;
  h ^= std::hash<uint8_t>{}(static_cast<uint8_t>(id.linkage)) << 2;
  return h;
}

CanonicalIdentity identity_of(const CanonicalSymbol& sym) {
  CanonicalIdentity id;
  id.kind = sym.kind;
  id.name = sym.source_name;
  id.linkage = sym.linkage;
  // For C++ function overloads, use the canonical type as discriminator.
  if (sym.linkage == LanguageLinkage::Cxx &&
      sym.kind == CanonicalSymbolKind::Function && sym.type) {
    id.discriminator_type = sym.type;
  }
  return id;
}

void CanonicalSymbolTable::insert_or_merge(CanonicalSymbol sym) {
  auto id = identity_of(sym);
  auto it = by_identity.find(id);
  if (it == by_identity.end()) {
    by_identity.emplace(std::move(id), std::move(sym));
    return;
  }

  // Merge: prefer the symbol with a more complete type.
  // A definition (with line > 0 and a Function type that has specified params)
  // takes precedence over a forward declaration (unspecified params).
  auto& existing = it->second;
  bool new_is_more_complete = false;

  if (sym.type && existing.type) {
    const auto* new_sig = get_function_sig(*sym.type);
    const auto* old_sig = get_function_sig(*existing.type);
    if (new_sig && old_sig) {
      // Prefer specified params over unspecified.
      if (old_sig->unspecified_params && !new_sig->unspecified_params)
        new_is_more_complete = true;
    }
  } else if (sym.type && !existing.type) {
    new_is_more_complete = true;
  }

  if (new_is_more_complete) {
    existing = std::move(sym);
  }
}

const CanonicalSymbol* CanonicalSymbolTable::lookup(const CanonicalIdentity& id) const {
  auto it = by_identity.find(id);
  return it != by_identity.end() ? &it->second : nullptr;
}

CanonicalSymbolTable build_symbol_table(const Node* root, SourceProfile profile) {
  auto result = build_canonical_symbols(root, profile);
  CanonicalSymbolTable table;
  for (auto& sym : result.symbols) {
    table.insert_or_merge(std::move(sym));
  }
  return table;
}

}  // namespace c4c::sema
