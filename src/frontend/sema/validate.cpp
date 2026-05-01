#include "validate.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "consteval.hpp"
#include "type_utils.hpp"

namespace c4c::sema {

using hir::ConstEvalEnv;
using hir::ConstEvalFunctionStructuredMap;
using hir::ConstEvalFunctionTextMap;
using hir::ConstEvalStructuredNameKey;
using hir::ConstMap;
using hir::ConstStructuredMap;
using hir::ConstTextMap;
using hir::evaluate_constant_expr;

bool SemaStructuredNameKey::operator==(const SemaStructuredNameKey& other) const {
  return namespace_context_id == other.namespace_context_id &&
         is_global_qualified == other.is_global_qualified &&
         qualifier_text_ids == other.qualifier_text_ids &&
         base_text_id == other.base_text_id;
}

std::size_t SemaStructuredNameKeyHash::operator()(const SemaStructuredNameKey& key) const {
  std::size_t h = std::hash<int>{}(key.namespace_context_id);
  h ^= std::hash<bool>{}(key.is_global_qualified) + 0x9e3779b9u + (h << 6) + (h >> 2);
  h ^= std::hash<TextId>{}(key.base_text_id) + 0x9e3779b9u + (h << 6) + (h >> 2);
  for (TextId segment : key.qualifier_text_ids) {
    h ^= std::hash<TextId>{}(segment) + 0x9e3779b9u + (h << 6) + (h >> 2);
  }
  return h;
}

std::optional<SemaStructuredNameKey> sema_local_name_key(const Node* node) {
  if (!node || node->unqualified_text_id == kInvalidText) return std::nullopt;
  if (node->is_global_qualified || node->n_qualifier_segments > 0) return std::nullopt;
  SemaStructuredNameKey key;
  key.base_text_id = node->unqualified_text_id;
  return key;
}

static bool sema_plain_var_ref_spells(const Node* node, const char* spelling) {
  return node && node->kind == NK_VAR && spelling && spelling[0] && node->name &&
         std::strcmp(node->name, spelling) == 0 && node->unqualified_name &&
         std::strcmp(node->unqualified_name, spelling) == 0 &&
         node->unqualified_text_id != kInvalidText && !node->is_global_qualified &&
         node->n_qualifier_segments == 0;
}

static TextId first_plain_var_ref_text_id(const Node* node, const char* spelling) {
  if (!node) return kInvalidText;
  if (sema_plain_var_ref_spells(node, spelling)) return node->unqualified_text_id;

  const Node* scalar_children[] = {
      node->left, node->right, node->cond, node->then_, node->else_,
      node->body, node->init, node->update,
  };
  for (const Node* child : scalar_children) {
    if (TextId text_id = first_plain_var_ref_text_id(child, spelling);
        text_id != kInvalidText) {
      return text_id;
    }
  }
  for (int i = 0; node->children && i < node->n_children; ++i) {
    if (TextId text_id = first_plain_var_ref_text_id(node->children[i], spelling);
        text_id != kInvalidText) {
      return text_id;
    }
  }
  for (int i = 0; node->params && i < node->n_params; ++i) {
    if (TextId text_id = first_plain_var_ref_text_id(node->params[i], spelling);
        text_id != kInvalidText) {
      return text_id;
    }
  }
  for (int i = 0; node->template_arg_exprs && i < node->n_template_args; ++i) {
    if (TextId text_id = first_plain_var_ref_text_id(node->template_arg_exprs[i], spelling);
        text_id != kInvalidText) {
      return text_id;
    }
  }
  for (int i = 0; node->ctor_init_args && node->ctor_init_nargs && i < node->n_ctor_inits; ++i) {
    for (int j = 0; node->ctor_init_args[i] && j < node->ctor_init_nargs[i]; ++j) {
      if (TextId text_id = first_plain_var_ref_text_id(node->ctor_init_args[i][j], spelling);
          text_id != kInvalidText) {
        return text_id;
      }
    }
  }
  return kInvalidText;
}

static std::optional<SemaStructuredNameKey> sema_injected_local_name_key(
    const Node* fn, const char* spelling) {
  const TextId text_id = first_plain_var_ref_text_id(fn, spelling);
  if (text_id == kInvalidText) return std::nullopt;
  SemaStructuredNameKey key;
  key.base_text_id = text_id;
  return key;
}

std::optional<SemaStructuredNameKey> sema_structured_name_key(const Node* node) {
  if (!node || node->unqualified_text_id == kInvalidText) return std::nullopt;
  if (node->n_qualifier_segments < 0) return std::nullopt;
  if (node->n_qualifier_segments > 0 && !node->qualifier_text_ids) return std::nullopt;

  SemaStructuredNameKey key;
  key.namespace_context_id = node->namespace_context_id;
  key.is_global_qualified = node->is_global_qualified;
  key.base_text_id = node->unqualified_text_id;
  key.qualifier_text_ids.reserve(static_cast<std::size_t>(node->n_qualifier_segments));
  for (int i = 0; i < node->n_qualifier_segments; ++i) {
    const TextId segment = node->qualifier_text_ids[i];
    if (segment == kInvalidText) return std::nullopt;
    key.qualifier_text_ids.push_back(segment);
  }
  return key;
}

std::optional<SemaStructuredNameKey> sema_symbol_name_key(const Node* node) {
  if (!node || node->namespace_context_id < 0 ||
      node->unqualified_text_id == kInvalidText) {
    return std::nullopt;
  }
  SemaStructuredNameKey key;
  key.namespace_context_id = node->namespace_context_id;
  key.base_text_id = node->unqualified_text_id;
  return key;
}

static std::optional<SemaStructuredNameKey> sema_qualified_symbol_name_key(
    const Node* node) {
  if (!node || node->namespace_context_id < 0 ||
      node->unqualified_text_id == kInvalidText) {
    return std::nullopt;
  }
  if (node->n_qualifier_segments <= 0) return sema_symbol_name_key(node);
  if (!node->qualifier_text_ids) return std::nullopt;

  SemaStructuredNameKey key;
  key.namespace_context_id = node->namespace_context_id;
  key.base_text_id = node->unqualified_text_id;
  key.qualifier_text_ids.reserve(static_cast<std::size_t>(node->n_qualifier_segments));
  for (int i = 0; i < node->n_qualifier_segments; ++i) {
    const TextId segment = node->qualifier_text_ids[i];
    if (segment == kInvalidText) return std::nullopt;
    key.qualifier_text_ids.push_back(segment);
  }
  return key;
}

static std::optional<SemaStructuredNameKey> sema_template_param_local_name_key(
    const Node* node, int param_index) {
  if (!node || param_index < 0 || param_index >= node->n_template_params ||
      !node->template_param_name_text_ids) {
    return std::nullopt;
  }
  const TextId text_id = node->template_param_name_text_ids[param_index];
  if (text_id == kInvalidText) return std::nullopt;
  SemaStructuredNameKey key;
  key.base_text_id = text_id;
  return key;
}

static bool sema_template_param_is_type_param(const Node* node, int param_index) {
  if (!node || param_index < 0 || param_index >= node->n_template_params ||
      !node->template_param_names || !node->template_param_names[param_index]) {
    return false;
  }
  return !(node->template_param_is_nttp && node->template_param_is_nttp[param_index]);
}

static TextId sema_template_param_name_text_id(const Node* node, int param_index) {
  if (!node || param_index < 0 || param_index >= node->n_template_params ||
      !node->template_param_name_text_ids) {
    return kInvalidText;
  }
  return node->template_param_name_text_ids[param_index];
}

SemaDualLookupMatch compare_sema_lookup_presence(bool legacy_found, bool structured_found) {
  if (!legacy_found && !structured_found) return SemaDualLookupMatch::BothMissing;
  if (legacy_found && structured_found) return SemaDualLookupMatch::Match;
  return legacy_found ? SemaDualLookupMatch::LegacyOnly : SemaDualLookupMatch::StructuredOnly;
}

static ConstEvalStructuredNameKey to_consteval_key(const SemaStructuredNameKey& key) {
  ConstEvalStructuredNameKey out;
  out.namespace_context_id = key.namespace_context_id;
  out.is_global_qualified = key.is_global_qualified;
  out.qualifier_text_ids = key.qualifier_text_ids;
  out.base_text_id = key.base_text_id;
  return out;
}

namespace {

struct FunctionSig {
  TypeSpec ret{};
  std::vector<TypeSpec> params;
  bool variadic = false;
  bool has_param_pack = false;
  // True for K&R-style f() declarations: accepts any number of arguments.
  bool unspecified_params = false;
};

struct ExprInfo {
  TypeSpec type{};
  bool valid = false;
  bool is_lvalue = false;
  bool is_const_lvalue = false;
  // True when this expression is a function name (from funcs_ lookup).
  // In C, a function name and &function-name have the same pointer type, so
  // NK_ADDR should not add an extra ptr_level when this flag is set.
  bool is_fn_name = false;
};

bool template_owner_can_defer_static_assert(const Node* owner) {
  return owner && owner->n_template_params > 0;
}

TypeSpec make_int_ts() {
  TypeSpec ts{};
  ts.base = TB_INT;
  ts.array_size = -1;
  ts.array_rank = 0;
  return ts;
}

TypeSpec decay_array(TypeSpec ts) {
  if (ts.is_lvalue_ref) ts.is_lvalue_ref = false;
  if (ts.is_rvalue_ref) ts.is_rvalue_ref = false;
  if (is_vector_ty(ts)) return ts;
  // is_ptr_to_array means the type is already a pointer wrapping an array (e.g. char (*)[4]);
  // do not decay such types a second time.
  if (ts.array_rank > 0 && !ts.is_ptr_to_array) {
    if (ts.array_rank > 1) {
      // Multi-dimensional array decays to pointer-to-inner-array.
      // e.g. char[3][28] → char(*)[?] (one rank consumed, one remains).
      ts.array_rank -= 1;
      ts.array_size = -1;  // inner size not tracked in flat TypeSpec
      ts.ptr_level += 1;
      ts.is_ptr_to_array = true;
    } else {
      ts.array_rank = 0;
      ts.array_size = -1;
      ts.ptr_level += 1;
    }
  }
  return ts;
}

bool is_arithmetic_base(TypeBase b) {
  switch (b) {
    case TB_CHAR:
    case TB_UCHAR:
    case TB_SCHAR:
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
    case TB_FLOAT:
    case TB_DOUBLE:
    case TB_LONGDOUBLE:
    case TB_BOOL:
    case TB_ENUM:
      return true;
    default:
      return false;
  }
}

bool is_float_or_complex_base(TypeBase b) {
  switch (b) {
    case TB_FLOAT:
    case TB_DOUBLE:
    case TB_LONGDOUBLE:
      return true;
    default:
      break;
  }
  return b >= TB_COMPLEX_FLOAT && b <= TB_COMPLEX_ULONGLONG;
}

bool is_pointer_like_type(const TypeSpec& ts_raw) {
  const TypeSpec ts = decay_array(ts_raw);
  return ts.ptr_level > 0 || ts.is_fn_ptr || ts.base == TB_FUNC_PTR;
}

TypeSpec referred_type(TypeSpec ts) {
  ts.is_lvalue_ref = false;
  ts.is_rvalue_ref = false;
  return ts;
}

bool is_invalid_pointer_float_implicit_conversion(
    const TypeSpec& dst_raw, const TypeSpec& src_raw, bool /*src_is_null_ptr_const*/) {
  const TypeSpec dst = decay_array(dst_raw);
  const TypeSpec src = decay_array(src_raw);
  const bool dst_ptr = is_pointer_like_type(dst);
  const bool src_ptr = is_pointer_like_type(src);
  const bool dst_float = is_float_or_complex_base(dst.base) && !dst_ptr;
  const bool src_float = is_float_or_complex_base(src.base) && !src_ptr;
  return (src_ptr && dst_float) || (src_float && dst_ptr);
}

bool is_integer_like_base(TypeBase b) {
  switch (b) {
    case TB_CHAR:
    case TB_UCHAR:
    case TB_SCHAR:
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
    case TB_BOOL:
    case TB_ENUM:
      return true;
    default:
      return false;
  }
}

bool is_switch_integer_type(const TypeSpec& ts_raw) {
  const TypeSpec ts = decay_array(ts_raw);
  if (ts.ptr_level > 0 || ts.array_rank > 0) return false;
  return is_integer_like_base(ts.base);
}

TypeSpec canonicalize_param_type(TypeSpec ts) {
  // C function parameter adjustment: array/function parameters adjust to pointers.
  // Preserve ref qualifiers across decay_array (which strips them for its own purposes).
  const bool save_lref = ts.is_lvalue_ref;
  const bool save_rref = ts.is_rvalue_ref;
  ts = decay_array(ts);
  ts.is_lvalue_ref = save_lref;
  ts.is_rvalue_ref = save_rref;
  // C11 6.7.6.3p8: a parameter of function type adjusts to pointer-to-function type.
  // Our TypeSpec uses is_fn_ptr=true,ptr_level=0 for bare function type "int()"
  // and is_fn_ptr=true,ptr_level=1 for function pointer "int(*)()".
  if (ts.is_fn_ptr && ts.ptr_level == 0) ts.ptr_level = 1;
  // Top-level qualifiers on parameters are not part of function type compatibility.
  ts.is_const = false;
  ts.is_volatile = false;
  return ts;
}

bool same_types_for_function_compat(TypeSpec a, TypeSpec b, bool for_param = false) {
  if (for_param) {
    a = canonicalize_param_type(a);
    b = canonicalize_param_type(b);
  }

  if (a.base != b.base) return false;
  if (a.is_vector != b.is_vector) return false;
  if (a.is_vector &&
      (a.vector_lanes != b.vector_lanes || a.vector_bytes != b.vector_bytes))
    return false;
  if (a.ptr_level != b.ptr_level) return false;
  if (a.is_lvalue_ref != b.is_lvalue_ref) return false;
  if (a.is_rvalue_ref != b.is_rvalue_ref) return false;
  if (a.array_rank != b.array_rank) return false;
  if (a.array_size != b.array_size) return false;
  if (a.is_fn_ptr != b.is_fn_ptr) return false;
  if (a.base == TB_STRUCT || a.base == TB_UNION || a.base == TB_ENUM) {
    if (a.tag == nullptr || b.tag == nullptr) return false;
    if (std::string(a.tag) != std::string(b.tag)) return false;
  }
  return true;
}

bool function_sig_compatible(const FunctionSig& a, const FunctionSig& b) {
  if (!same_types_for_function_compat(a.ret, b.ret, false)) return false;
  // K&R unspecified-params declarations are compatible with any prototype that
  // has the same return type (C11 6.2.7p3).
  if (a.unspecified_params || b.unspecified_params) return true;
  if (a.variadic != b.variadic) return false;
  if (a.has_param_pack != b.has_param_pack) return false;
  if (a.params.size() != b.params.size()) return false;
  for (size_t i = 0; i < a.params.size(); ++i) {
    if (!same_types_for_function_compat(a.params[i], b.params[i], true)) return false;
  }
  return true;
}

// Check if two function signatures are ref-overloads: same param count/base types
// but differ in lvalue-ref vs rvalue-ref qualifier on at least one parameter.
bool is_ref_overload(const FunctionSig& a, const FunctionSig& b) {
  if (a.variadic != b.variadic) return false;
  if (a.has_param_pack != b.has_param_pack) return false;
  if (a.params.size() != b.params.size()) return false;
  bool has_ref_diff = false;
  for (size_t i = 0; i < a.params.size(); ++i) {
    TypeSpec ap = a.params[i], bp = b.params[i];
    bool ref_differs = (ap.is_lvalue_ref != bp.is_lvalue_ref) ||
                       (ap.is_rvalue_ref != bp.is_rvalue_ref);
    if (ref_differs) {
      has_ref_diff = true;
      // Strip ref and const for base-type comparison.
      ap.is_lvalue_ref = bp.is_lvalue_ref = false;
      ap.is_rvalue_ref = bp.is_rvalue_ref = false;
      ap.is_const = bp.is_const = false;
      if (!same_types_for_function_compat(ap, bp, true)) return false;
    } else if (!same_types_for_function_compat(ap, bp, true)) {
      return false;
    }
  }
  return has_ref_diff;
}

bool supports_cpp_overload_set(const char* name) {
  if (!name || !name[0]) return false;
  if (std::strncmp(name, "::", 2) == 0) name += 2;
  // C++ permits overloading ordinary operator functions, not just allocation
  // forms. Treat our parser's normalized operator_* spellings as overloadable
  // so sema doesn't reject later declarations as C-style redeclarations.
  if (std::strncmp(name, "operator_", 9) == 0) return true;
  return std::strcmp(name, "operator_new") == 0 ||
         std::strcmp(name, "operator_new_array") == 0 ||
         std::strcmp(name, "operator_delete") == 0 ||
         std::strcmp(name, "operator_delete_array") == 0;
}

bool same_tagged_type(const TypeSpec& a, const TypeSpec& b) {
  if (a.base != b.base) return false;
  if (a.base != TB_STRUCT && a.base != TB_UNION && a.base != TB_ENUM) return true;
  if (a.tag == nullptr || b.tag == nullptr) return false;
  return std::string(a.tag) == std::string(b.tag);
}

std::string unqualified_tag_name(const TypeSpec& ts) {
  if (!ts.tag || !ts.tag[0]) return {};
  const std::string tag(ts.tag);
  const size_t sep = tag.rfind("::");
  return sep == std::string::npos ? tag : tag.substr(sep + 2);
}

bool same_tagged_type_ignoring_qualification(const TypeSpec& a, const TypeSpec& b) {
  if (a.base != b.base) return false;
  if (a.base != TB_STRUCT && a.base != TB_UNION && a.base != TB_ENUM) return false;
  const std::string a_name = unqualified_tag_name(a);
  const std::string b_name = unqualified_tag_name(b);
  return !a_name.empty() && a_name == b_name;
}

// C11 6.3.2.3p3: null pointer constant is an integer constant expression with value 0,
// or such an expression cast to void*.
bool is_null_pointer_constant_expr(const Node* n) {
  if (!n) return false;
  if (n->kind == NK_INT_LIT) return n->ival == 0;
  if (n->kind == NK_CAST) {
    const bool cast_to_void_ptr =
        n->type.base == TB_VOID && n->type.ptr_level > 0 && n->type.array_rank == 0;
    if (cast_to_void_ptr) return is_null_pointer_constant_expr(n->left);
  }
  return false;
}

bool is_deref_of_this_expr(const Node* n) {
  return n && n->kind == NK_DEREF && n->left && n->left->kind == NK_VAR &&
         n->left->name && std::strcmp(n->left->name, "this") == 0;
}

std::optional<std::string> qualified_method_owner_struct(const Node* fn) {
  if (!fn || fn->kind != NK_FUNCTION || !fn->name) return std::nullopt;
  const std::string name(fn->name);
  const size_t sep = name.rfind("::");
  if (sep == std::string::npos || sep == 0 || sep + 2 >= name.size())
    return std::nullopt;
  return name.substr(0, sep);
}


// Implicit conversion check for assignment-like contexts:
// - function-call argument to parameter (C11 6.5.2.2p7)
// - assignment/initializer constraints (C11 6.5.16.1)
bool implicit_convertible(const TypeSpec& dst_raw, const TypeSpec& src_raw,
                          bool src_is_null_ptr_const) {
  // Function pointer types are structurally complex and not fully tracked;
  // accept any assignment involving a function pointer type.
  if (dst_raw.is_fn_ptr || src_raw.is_fn_ptr ||
      dst_raw.base == TB_FUNC_PTR || src_raw.base == TB_FUNC_PTR) {
    return true;
  }
  // Unresolved typedef names (e.g., size_t used before full resolution) are
  // accepted conservatively; we cannot check them without a typedef table.
  if (dst_raw.base == TB_TYPEDEF || src_raw.base == TB_TYPEDEF) return true;

  const TypeSpec dst = decay_array(dst_raw);
  const TypeSpec src = decay_array(src_raw);

  // pointer vs non-pointer mismatch
  const bool d_ptr = dst.ptr_level > 0;
  const bool s_ptr = src.ptr_level > 0;
  if (d_ptr != s_ptr) {
    // Allow null pointer constant to convert to any pointer parameter.
    if (d_ptr && !s_ptr && src_is_null_ptr_const) return true;
    return false;
  }

  // Pointer compatibility: require same depth and pointee family.
  if (d_ptr && s_ptr) {
    // void* converts to/from any object pointer type (C11 6.3.2.3p1).
    // Either side being void* (ptr_level==1, base==TB_VOID) is sufficient.
    if ((dst.ptr_level == 1 && dst.base == TB_VOID) ||
        (src.ptr_level == 1 && src.base == TB_VOID)) return true;
    if (dst.ptr_level != src.ptr_level) return false;
    if (dst.base == TB_STRUCT || dst.base == TB_UNION || dst.base == TB_ENUM ||
        src.base == TB_STRUCT || src.base == TB_UNION || src.base == TB_ENUM) {
      // Allow enum* <-> integer type* (enum underlying type is int-compatible).
      if ((dst.base == TB_ENUM && is_arithmetic_base(src.base) &&
           src.base != TB_STRUCT && src.base != TB_UNION) ||
          (src.base == TB_ENUM && is_arithmetic_base(dst.base) &&
           dst.base != TB_STRUCT && dst.base != TB_UNION)) {
        return true;
      }
      return same_tagged_type(dst, src);
    }
    // Allow signed/unsigned pointer pairs of the same underlying size
    // (GCC warns but permits; required for many C test-suite programs).
    auto signed_unsigned_pair = [](TypeBase a, TypeBase b) {
      return (a == TB_CHAR    && b == TB_UCHAR)   || (a == TB_UCHAR   && b == TB_CHAR)   ||
             (a == TB_SHORT   && b == TB_USHORT)  || (a == TB_USHORT  && b == TB_SHORT)  ||
             (a == TB_INT     && b == TB_UINT)    || (a == TB_UINT    && b == TB_INT)    ||
             (a == TB_LONG    && b == TB_ULONG)   || (a == TB_ULONG   && b == TB_LONG)   ||
             (a == TB_LONGLONG && b == TB_ULONGLONG) || (a == TB_ULONGLONG && b == TB_LONGLONG);
    };
    if (signed_unsigned_pair(dst.base, src.base)) return true;
    if (dst.base == src.base) return true;
    // GCC warns but does not reject pointer assignments between arithmetic
    // base types of the same pointer depth (e.g., float* = int*).
    if (is_arithmetic_base(dst.base) && is_arithmetic_base(src.base)) return true;
    return false;
  }

  // Non-pointer: structs/unions must match exactly.
  if (dst.base == TB_STRUCT || dst.base == TB_UNION ||
      src.base == TB_STRUCT || src.base == TB_UNION) {
    return same_tagged_type(dst, src);
  }

  if (dst.is_vector || src.is_vector) {
    return dst.is_vector && src.is_vector &&
           dst.base == src.base &&
           dst.vector_lanes == src.vector_lanes &&
           dst.vector_bytes == src.vector_bytes;
  }

  // Arithmetic scalar conversions are allowed.
  if (is_arithmetic_base(dst.base) && is_arithmetic_base(src.base)) return true;

  // _Complex types convert freely among themselves and to/from arithmetic types.
  const bool d_cx = (dst.base >= TB_COMPLEX_FLOAT && dst.base <= TB_COMPLEX_ULONGLONG);
  const bool s_cx = (src.base >= TB_COMPLEX_FLOAT && src.base <= TB_COMPLEX_ULONGLONG);
  if (d_cx || s_cx) return true;

  return dst.base == src.base;
}

class Validator {
 public:
  ValidateResult run(const Node* root) {
    if (!root || root->kind != NK_PROGRAM) {
      emit(0, "internal: sema root is not program");
      return finish();
    }

    collect_toplevel(root);
    for (int i = 0; i < root->n_children; ++i) {
      validate_toplevel_node(root->children[i]);
    }
    return finish();
  }

 private:
  struct ScopedSym {
    TypeSpec type{};
    bool initialized = true;
  };

  std::vector<Diagnostic> diags_;
  std::unordered_map<std::string, TypeSpec> globals_;
  std::unordered_set<std::string> structured_global_names_;
  std::unordered_set<std::string> structured_global_self_names_;
  std::unordered_map<SemaStructuredNameKey, const TypeSpec*, SemaStructuredNameKeyHash>
      structured_globals_;
  std::unordered_map<std::string, SemaStructuredNameKey> structured_global_keys_by_name_;
  std::unordered_map<std::string, TypeSpec> enum_consts_;
  std::unordered_set<std::string> structured_enum_const_names_;
  std::unordered_map<SemaStructuredNameKey, const TypeSpec*, SemaStructuredNameKeyHash>
      structured_enum_consts_;
  std::unordered_map<std::string, SemaStructuredNameKey> structured_enum_const_keys_by_name_;
  ConstMap enum_const_vals_global_;   // integer values of global enum constants
  ConstTextMap enum_const_vals_global_by_text_;
  ConstStructuredMap enum_const_vals_global_by_key_;
  std::vector<ConstMap> enum_const_vals_scopes_;  // block/function-scoped enum constants
  std::vector<ConstTextMap> enum_const_vals_scopes_by_text_;
  std::vector<ConstStructuredMap> enum_const_vals_scopes_by_key_;
  ConstMap global_const_vals_;
  ConstTextMap global_const_vals_by_text_;
  ConstStructuredMap global_const_vals_by_key_;
  std::vector<ConstMap> local_const_vals_scopes_;  // block-scoped const/constexpr local values
  std::vector<ConstTextMap> local_const_vals_scopes_by_text_;
  std::vector<ConstStructuredMap> local_const_vals_scopes_by_key_;
  std::unordered_map<std::string, FunctionSig> funcs_;
  std::unordered_map<SemaStructuredNameKey, const FunctionSig*, SemaStructuredNameKeyHash>
      structured_funcs_;
  std::unordered_set<std::string> structured_function_names_;
  std::unordered_map<std::string, const Node*> consteval_funcs_;
  ConstEvalFunctionTextMap consteval_funcs_by_text_;
  ConstEvalFunctionStructuredMap consteval_funcs_by_key_;
  // Ref-overloaded function signatures: name → multiple overloads differing in ref-qualifier.
  std::unordered_map<std::string, std::vector<FunctionSig>> ref_overload_sigs_;
  std::unordered_map<std::string, std::vector<FunctionSig>> cpp_overload_sigs_;
  std::unordered_set<std::string> structured_ref_overload_names_;
  std::unordered_set<std::string> structured_cpp_overload_names_;
  std::unordered_map<SemaStructuredNameKey, const std::vector<FunctionSig>*,
                     SemaStructuredNameKeyHash>
      structured_ref_overload_sigs_;
  std::unordered_map<SemaStructuredNameKey, const std::vector<FunctionSig>*,
                     SemaStructuredNameKeyHash>
      structured_cpp_overload_sigs_;
  std::unordered_set<std::string> complete_structs_;
  std::unordered_set<std::string> complete_unions_;
  std::unordered_map<std::string, SemaStructuredNameKey> structured_record_keys_by_tag_;
  std::unordered_set<std::string> ambiguous_structured_record_tags_;
  std::unordered_set<SemaStructuredNameKey, SemaStructuredNameKeyHash> complete_structs_by_key_;
  std::unordered_set<SemaStructuredNameKey, SemaStructuredNameKeyHash> complete_unions_by_key_;
  std::unordered_map<const Node*, const Node*> method_owner_records_;
  std::unordered_map<SemaStructuredNameKey, const Node*, SemaStructuredNameKeyHash>
      struct_defs_by_key_;
  std::unordered_map<std::string, std::vector<const Node*>> struct_defs_by_unqualified_name_;
  std::unordered_map<SemaStructuredNameKey, std::unordered_set<TextId>, SemaStructuredNameKeyHash>
      struct_field_text_ids_by_key_;
  std::unordered_map<SemaStructuredNameKey, std::unordered_map<std::string, TextId>,
                     SemaStructuredNameKeyHash>
      struct_field_text_ids_by_name_by_key_;
  std::unordered_map<SemaStructuredNameKey, std::unordered_map<TextId, TypeSpec>,
                     SemaStructuredNameKeyHash>
      struct_static_member_types_by_key_;
  std::unordered_map<SemaStructuredNameKey, std::vector<SemaStructuredNameKey>,
                     SemaStructuredNameKeyHash>
      struct_base_keys_by_key_;
  std::unordered_set<std::string> template_type_params_;
  std::unordered_set<TextId> template_type_param_text_ids_;
  std::unordered_map<std::string, TextId> template_type_param_text_id_by_name_;
  std::string current_method_struct_tag_;
  std::optional<SemaStructuredNameKey> current_method_struct_key_;
  std::vector<std::unordered_map<std::string, ScopedSym>> scopes_;
  std::vector<std::unordered_set<std::string>> structured_scope_names_;
  std::vector<std::unordered_map<SemaStructuredNameKey, ScopedSym*, SemaStructuredNameKeyHash>>
      structured_scopes_;
  bool suppress_uninit_read_ = false;

  int loop_depth_ = 0;
  // Set when the current function contains any goto statement.  Functions with
  // gotos/loops have non-trivial control flow, so uninit-read detection is
  // suppressed to avoid false positives on dead code paths.
  bool fn_has_goto_ = false;
  bool fn_has_loop_ = false;
  bool in_function_ = false;
  TypeSpec current_fn_ret_{};
  const Node* current_fn_node_ = nullptr;  // for template param lookup

  struct SwitchCtx {
    std::unordered_set<long long> case_vals;
    bool has_default = false;
  };
  std::vector<SwitchCtx> switch_stack_;

  static bool tracks_uninit_read(const TypeSpec& ts) {
    // Keep this deliberately narrow to avoid frontend false positives:
    // only plain scalar locals participate in this check.
    if (ts.array_rank > 0) return false;
    if (ts.ptr_level > 0) return false;
    if (ts.base == TB_STRUCT || ts.base == TB_UNION) return false;
    // Complex types are typically initialized piecemeal via __real/__imag;
    // exclude them to avoid false positives.
    if (ts.base >= TB_COMPLEX_FLOAT && ts.base <= TB_COMPLEX_ULONGLONG)
      return false;
    return true;
  }

  void emit(int line, std::string msg) {
    diags_.push_back(Diagnostic{nullptr, line, 1, std::move(msg)});
  }

  void emit(const Node* node, std::string msg) {
    if (!node) {
      emit(0, std::move(msg));
      return;
    }
    diags_.push_back(Diagnostic{
        node->file, node->line, node->column > 0 ? node->column : 1, std::move(msg)});
  }

  void record_cpp_overload(const std::string& name, FunctionSig sig) {
    auto& ovset = cpp_overload_sigs_[name];
    if (ovset.empty()) {
      auto it = funcs_.find(name);
      if (it != funcs_.end()) ovset.push_back(it->second);
    }
    for (FunctionSig& existing : ovset) {
      if (!function_sig_compatible(existing, sig)) continue;
      if (existing.unspecified_params && !sig.unspecified_params) {
        existing = std::move(sig);
      }
      return;
    }
    ovset.push_back(std::move(sig));
  }

  void bind_global(const Node* n) {
    if (!n || !n->name || !n->name[0]) return;
    auto [it, inserted] = globals_.insert_or_assign(n->name, n->type);
    (void)inserted;
    if (auto key = sema_symbol_name_key(n); key.has_value() && key->valid()) {
      structured_global_names_.insert(n->name);
      if (n->unqualified_name && n->unqualified_name[0] &&
          unqualified_name(n->name) == n->unqualified_name) {
        structured_global_self_names_.insert(n->name);
      }
      structured_globals_[*key] = &it->second;
      structured_global_keys_by_name_[n->name] = *key;
    }
    if (auto key = sema_qualified_symbol_name_key(n); key.has_value() &&
        key->valid()) {
      structured_global_names_.insert(n->name);
      structured_globals_[*key] = &it->second;
      structured_global_keys_by_name_[n->name] = *key;
    }
  }

  void mirror_function_decl(const Node* n) {
    if (!n || !n->name || !n->name[0]) return;
    auto key = sema_function_lookup_key(n);
    if (!key.has_value() || !key->valid()) return;

    auto fn = funcs_.find(n->name);
    if (fn != funcs_.end()) {
      structured_function_names_.insert(n->name);
      structured_funcs_[*key] = &fn->second;
    }

    auto ref_ov = ref_overload_sigs_.find(n->name);
    if (ref_ov != ref_overload_sigs_.end()) {
      structured_ref_overload_names_.insert(n->name);
      structured_ref_overload_sigs_[*key] = &ref_ov->second;
    }

    auto cpp_ov = cpp_overload_sigs_.find(n->name);
    if (cpp_ov != cpp_overload_sigs_.end()) {
      structured_cpp_overload_names_.insert(n->name);
      structured_cpp_overload_sigs_[*key] = &cpp_ov->second;
    }
  }

  static std::optional<SemaStructuredNameKey> sema_function_lookup_key(
      const Node* n) {
    if (!n) return std::nullopt;
    if (n->is_global_qualified || n->n_qualifier_segments > 0) {
      if (auto key = sema_qualified_symbol_name_key(n);
          key.has_value() && key->valid()) {
        return key;
      }
    }
    auto key = sema_symbol_name_key(n);
    if (key.has_value() && key->valid()) return key;
    return std::nullopt;
  }

  void record_consteval_function(const Node* n) {
    if (!n || !n->name || !n->name[0]) return;
    consteval_funcs_[n->name] = n;
    if (n->unqualified_text_id != kInvalidText) {
      consteval_funcs_by_text_[n->unqualified_text_id] = n;
    }
    if (auto key = sema_symbol_name_key(n); key.has_value() && key->valid()) {
      consteval_funcs_by_key_[to_consteval_key(*key)] = n;
    }
  }

  const TypeSpec* lookup_global_by_key(const SemaStructuredNameKey& key) const {
    auto it = structured_globals_.find(key);
    return it != structured_globals_.end() ? it->second : nullptr;
  }

  const FunctionSig* lookup_function_by_key(const SemaStructuredNameKey& key) const {
    auto it = structured_funcs_.find(key);
    return it != structured_funcs_.end() ? it->second : nullptr;
  }

  const std::vector<FunctionSig>* lookup_ref_overloads_by_key(
      const SemaStructuredNameKey& key) const {
    auto it = structured_ref_overload_sigs_.find(key);
    return it != structured_ref_overload_sigs_.end() ? it->second : nullptr;
  }

  const std::vector<FunctionSig>* lookup_cpp_overloads_by_key(
      const SemaStructuredNameKey& key) const {
    auto it = structured_cpp_overload_sigs_.find(key);
    return it != structured_cpp_overload_sigs_.end() ? it->second : nullptr;
  }

  const FunctionSig* lookup_function_by_name(const std::string& name,
                                             const Node* reference = nullptr) const {
    if (reference) {
      if (auto key = sema_function_lookup_key(reference); key.has_value()) {
        const FunctionSig* structured = lookup_function_by_key(*key);
        if (structured) return structured;
        if (structured_function_names_.count(name) > 0) return nullptr;
      }
    }
    auto it = funcs_.find(name);
    return it != funcs_.end() ? &it->second : nullptr;
  }

  const std::vector<FunctionSig>* lookup_ref_overloads_by_name(
      const std::string& name, const Node* reference = nullptr) const {
    if (reference) {
      if (auto key = sema_function_lookup_key(reference); key.has_value()) {
        const std::vector<FunctionSig>* structured =
            lookup_ref_overloads_by_key(*key);
        if (structured) return structured;
        if (structured_ref_overload_names_.count(name) > 0) return nullptr;
      }
    }
    auto it = ref_overload_sigs_.find(name);
    return it != ref_overload_sigs_.end() ? &it->second : nullptr;
  }

  const std::vector<FunctionSig>* lookup_cpp_overloads_by_name(
      const std::string& name, const Node* reference = nullptr) const {
    if (reference) {
      if (auto key = sema_function_lookup_key(reference); key.has_value()) {
        const std::vector<FunctionSig>* structured =
            lookup_cpp_overloads_by_key(*key);
        if (structured) return structured;
        if (structured_cpp_overload_names_.count(name) > 0) return nullptr;
      }
    }
    auto it = cpp_overload_sigs_.find(name);
    return it != cpp_overload_sigs_.end() ? &it->second : nullptr;
  }

  const Node* lookup_consteval_function_by_key(const SemaStructuredNameKey& key) const {
    auto it = consteval_funcs_by_key_.find(to_consteval_key(key));
    return it != consteval_funcs_by_key_.end() ? it->second : nullptr;
  }

  const Node* lookup_consteval_function_by_text(const Node* reference) const {
    if (!reference || reference->unqualified_text_id == kInvalidText ||
        reference->is_global_qualified || reference->n_qualifier_segments > 0) {
      return nullptr;
    }
    auto it = consteval_funcs_by_text_.find(reference->unqualified_text_id);
    return it != consteval_funcs_by_text_.end() ? it->second : nullptr;
  }

  const Node* lookup_consteval_function_by_name(
      const std::string& name, const Node* reference = nullptr) const {
    if (reference) {
      bool has_authoritative_metadata = false;
      if (auto key = sema_symbol_name_key(reference); key.has_value()) {
        has_authoritative_metadata = true;
        const Node* structured = lookup_consteval_function_by_key(*key);
        if (structured) return structured;
      }
      if (reference->unqualified_text_id != kInvalidText &&
          !reference->is_global_qualified && reference->n_qualifier_segments == 0) {
        has_authoritative_metadata = true;
        const Node* text = lookup_consteval_function_by_text(reference);
        if (text) return text;
      }
      if (has_authoritative_metadata) return nullptr;
    }
    auto it = consteval_funcs_.find(name);
    const Node* legacy = it != consteval_funcs_.end() ? it->second : nullptr;
    return legacy;
  }

  ValidateResult finish() {
    ValidateResult r;
    r.ok = diags_.empty();
    r.diagnostics = std::move(diags_);
    return r;
  }

  void enter_scope() {
    scopes_.emplace_back();
    structured_scope_names_.emplace_back();
    structured_scopes_.emplace_back();
    enum_const_vals_scopes_.emplace_back();
    enum_const_vals_scopes_by_text_.emplace_back();
    enum_const_vals_scopes_by_key_.emplace_back();
    local_const_vals_scopes_.emplace_back();
    local_const_vals_scopes_by_text_.emplace_back();
    local_const_vals_scopes_by_key_.emplace_back();
  }

  void leave_scope() {
    if (!scopes_.empty()) scopes_.pop_back();
    if (!structured_scope_names_.empty()) structured_scope_names_.pop_back();
    if (!structured_scopes_.empty()) structured_scopes_.pop_back();
    if (!enum_const_vals_scopes_.empty()) enum_const_vals_scopes_.pop_back();
    if (!enum_const_vals_scopes_by_text_.empty()) enum_const_vals_scopes_by_text_.pop_back();
    if (!enum_const_vals_scopes_by_key_.empty()) enum_const_vals_scopes_by_key_.pop_back();
    if (!local_const_vals_scopes_.empty()) local_const_vals_scopes_.pop_back();
    if (!local_const_vals_scopes_by_text_.empty()) local_const_vals_scopes_by_text_.pop_back();
    if (!local_const_vals_scopes_by_key_.empty()) local_const_vals_scopes_by_key_.pop_back();
  }

  void bind_local(const std::string& name, const TypeSpec& ts, bool initialized, int line,
                  std::optional<SemaStructuredNameKey> structured_key = std::nullopt) {
    if (scopes_.empty()) enter_scope();
    auto& scope = scopes_.back();
    if (scope.find(name) != scope.end()) {
      emit(line, "redefinition of symbol '" + name + "' in same scope");
      return;
    }
    auto [it, inserted] = scope.emplace(name, ScopedSym{ts, initialized});
    if (inserted && structured_key.has_value() && structured_key->valid()) {
      if (structured_scope_names_.size() < scopes_.size()) {
        structured_scope_names_.resize(scopes_.size());
      }
      structured_scope_names_.back().insert(name);
      if (structured_scopes_.size() < scopes_.size()) structured_scopes_.resize(scopes_.size());
      structured_scopes_.back().emplace(*structured_key, &it->second);
    }
  }

  TypeSpec deduce_local_decl_type(const Node* decl, const ExprInfo* init_info = nullptr) {
    if (!decl) return {};
    TypeSpec deduced = decl->type;
    if (deduced.base != TB_AUTO || !decl->init) return deduced;

    ExprInfo local_init_info = init_info ? *init_info : infer_expr(decl->init);
    if (!local_init_info.valid) return deduced;

    deduced = local_init_info.type;
    deduced.is_const = deduced.is_const || decl->type.is_const;
    deduced.is_volatile = deduced.is_volatile || decl->type.is_volatile;
    deduced.is_lvalue_ref = false;
    deduced.is_rvalue_ref = false;
    if (decl->type.is_lvalue_ref) {
      deduced.is_lvalue_ref = true;
    } else if (decl->type.is_rvalue_ref) {
      if (local_init_info.is_lvalue) deduced.is_lvalue_ref = true;
      else deduced.is_rvalue_ref = true;
    }
    return deduced;
  }

  struct LocalRenderedLookup {
    const ScopedSym* symbol = nullptr;
    bool has_structured_metadata = false;
  };

  LocalRenderedLookup lookup_local_symbol_by_name(const std::string& name) const {
    for (std::size_t i = scopes_.size(); i > 0; --i) {
      const auto& scope = scopes_[i - 1];
      auto f = scope.find(name);
      if (f != scope.end()) {
        const bool structured =
            i <= structured_scope_names_.size() &&
            structured_scope_names_[i - 1].count(name) > 0;
        return LocalRenderedLookup{&f->second, structured};
      }
    }
    return {};
  }

  const ScopedSym* lookup_local_symbol_by_key(const SemaStructuredNameKey& key) const {
    for (auto it = structured_scopes_.rbegin(); it != structured_scopes_.rend(); ++it) {
      auto f = it->find(key);
      if (f != it->end()) return f->second;
    }
    return nullptr;
  }

  std::optional<ScopedSym> lookup_symbol(const std::string& name,
                                         const Node* reference = nullptr) const {
    const LocalRenderedLookup rendered_local = lookup_local_symbol_by_name(name);
    const auto local_key = reference ? sema_local_name_key(reference) : std::nullopt;
    if (local_key.has_value()) {
      const ScopedSym* structured = lookup_local_symbol_by_key(*local_key);
      (void)compare_sema_lookup_ptrs(rendered_local.symbol, structured);
      if (structured) return *structured;
    }
    if (rendered_local.symbol &&
        !(local_key.has_value() && rendered_local.has_structured_metadata)) {
      return *rendered_local.symbol;
    }

    const auto structured_key =
        reference ? sema_symbol_name_key(reference) : std::nullopt;
    const auto qualified_structured_key =
        reference ? sema_qualified_symbol_name_key(reference) : std::nullopt;
    const auto structured_global_metadata_matches_reference =
        [&](const std::string& rendered_name) {
          if (!qualified_structured_key.has_value()) return false;
          auto it = structured_global_keys_by_name_.find(rendered_name);
          return it != structured_global_keys_by_name_.end() &&
                 it->second == *qualified_structured_key;
        };
    const auto structured_global_metadata_qualifier_matches_reference =
        [&](const std::string& rendered_name) {
          if (!qualified_structured_key.has_value()) return false;
          auto it = structured_global_keys_by_name_.find(rendered_name);
          if (it == structured_global_keys_by_name_.end()) return false;
          if (it->second.qualifier_text_ids.empty()) return true;
          return it->second.qualifier_text_ids ==
                 qualified_structured_key->qualifier_text_ids;
        };
    const auto structured_enum_metadata_matches_reference =
        [&](const std::string& rendered_name) {
          if (!qualified_structured_key.has_value()) return false;
          auto it = structured_enum_const_keys_by_name_.find(rendered_name);
          return it != structured_enum_const_keys_by_name_.end() &&
                 it->second == *qualified_structured_key;
        };
    const bool has_unqualified_or_global_structured_symbol_key =
        structured_key.has_value() && reference && reference->n_qualifier_segments == 0;
    auto g = globals_.find(name);
    const TypeSpec* rendered_global_compatibility = g != globals_.end() ? &g->second : nullptr;
    const bool rendered_global_has_structured_metadata =
        structured_global_names_.count(name) > 0;
    const bool rendered_global_blocks_unqualified_structured_key =
        rendered_global_has_structured_metadata &&
        (name.find("::") == std::string::npos || name.rfind("::", 0) == 0);
    const bool rendered_global_conflicts_with_reference =
        reference && reference->n_qualifier_segments > 0 &&
        reference->unqualified_name && reference->unqualified_name[0] &&
        std::string(reference->unqualified_name).find("::") == std::string::npos &&
        structured_global_names_.count(name) > 0 &&
        (unqualified_name(name) != reference->unqualified_name ||
         !structured_global_metadata_qualifier_matches_reference(name));
    const bool rendered_global_matches_reference =
        !reference || !reference->unqualified_name || !reference->unqualified_name[0] ||
        unqualified_name(name) == reference->unqualified_name;
    if (qualified_structured_key.has_value()) {
      const TypeSpec* structured_global = lookup_global_by_key(*qualified_structured_key);
      (void)compare_sema_lookup_ptrs(rendered_global_compatibility, structured_global);
      if (structured_global) return ScopedSym{*structured_global, true};
    }
    if (structured_key.has_value() &&
        (!qualified_structured_key.has_value() ||
         *structured_key != *qualified_structured_key) &&
        (!reference || reference->n_qualifier_segments == 0 ||
         name.find("::") == std::string::npos)) {
      const TypeSpec* structured_global = lookup_global_by_key(*structured_key);
      (void)compare_sema_lookup_ptrs(rendered_global_compatibility, structured_global);
      if (structured_global) return ScopedSym{*structured_global, true};
    }
    if (rendered_global_compatibility &&
        (!structured_key.has_value() ||
         (!rendered_global_has_structured_metadata &&
          rendered_global_matches_reference) ||
         (rendered_global_has_structured_metadata &&
          rendered_global_matches_reference &&
          structured_global_self_names_.count(name) > 0)) &&
        !(has_unqualified_or_global_structured_symbol_key &&
          rendered_global_blocks_unqualified_structured_key) &&
        !(structured_key.has_value() && rendered_global_has_structured_metadata &&
          rendered_global_conflicts_with_reference)) {
      return ScopedSym{*rendered_global_compatibility, true};
    }

    const FunctionSig* fn = lookup_function_by_name(name, reference);
    if (fn) {
      TypeSpec fts = fn->ret;
      fts.ptr_level += 1;
      return ScopedSym{fts, true};
    }
    auto ec = enum_consts_.find(name);
    const TypeSpec* rendered_enum_compatibility = ec != enum_consts_.end() ? &ec->second : nullptr;
    const bool rendered_enum_has_structured_metadata =
        structured_enum_const_names_.count(name) > 0;
    const bool rendered_enum_blocks_unqualified_structured_key =
        rendered_enum_has_structured_metadata &&
        (name.find("::") == std::string::npos || name.rfind("::", 0) == 0);
    const bool rendered_enum_matches_reference =
        !reference || !reference->unqualified_name || !reference->unqualified_name[0] ||
        unqualified_name(name) == reference->unqualified_name;
    if (qualified_structured_key.has_value()) {
      const TypeSpec* structured_enum = nullptr;
      auto se = structured_enum_consts_.find(*qualified_structured_key);
      if (se != structured_enum_consts_.end()) structured_enum = se->second;
      (void)compare_sema_lookup_ptrs(rendered_enum_compatibility, structured_enum);
      if (structured_enum) return ScopedSym{*structured_enum, true};
    }
    if (structured_key.has_value() &&
        (!qualified_structured_key.has_value() ||
         *structured_key != *qualified_structured_key) &&
        (!reference || reference->n_qualifier_segments == 0)) {
      const TypeSpec* structured_enum = nullptr;
      auto se = structured_enum_consts_.find(*structured_key);
      if (se != structured_enum_consts_.end()) structured_enum = se->second;
      (void)compare_sema_lookup_ptrs(rendered_enum_compatibility, structured_enum);
      if (structured_enum) return ScopedSym{*structured_enum, true};
    }
    if (rendered_enum_compatibility &&
        (!structured_key.has_value() ||
         (!rendered_enum_has_structured_metadata &&
          rendered_enum_matches_reference) ||
         (rendered_enum_has_structured_metadata &&
          rendered_enum_matches_reference &&
          structured_enum_metadata_matches_reference(name))) &&
        !(has_unqualified_or_global_structured_symbol_key &&
          rendered_enum_blocks_unqualified_structured_key)) {
      return ScopedSym{*rendered_enum_compatibility, true};
    }
    return std::nullopt;
  }

  void populate_const_eval_env(ConstEvalEnv& env) const {
    env.enum_consts = &enum_const_vals_global_;
    env.named_consts = &global_const_vals_;
    env.enum_consts_by_text = &enum_const_vals_global_by_text_;
    env.named_consts_by_text = &global_const_vals_by_text_;
    env.enum_consts_by_key = &enum_const_vals_global_by_key_;
    env.named_consts_by_key = &global_const_vals_by_key_;
    env.enum_scopes = &enum_const_vals_scopes_;
    env.local_const_scopes = &local_const_vals_scopes_;
    env.enum_scopes_by_text = &enum_const_vals_scopes_by_text_;
    env.local_const_scopes_by_text = &local_const_vals_scopes_by_text_;
    env.enum_scopes_by_key = &enum_const_vals_scopes_by_key_;
    env.local_const_scopes_by_key = &local_const_vals_scopes_by_key_;
  }

  static bool is_const_int_binding_type(const TypeSpec& ts) {
    return is_switch_integer_type(ts) &&
           ts.ptr_level == 0 &&
           ts.array_rank == 0 &&
           !ts.is_lvalue_ref &&
           !ts.is_rvalue_ref;
  }

  void record_global_const_binding(const Node* n, long long value) {
    if (!n || !n->name || !n->name[0]) return;
    global_const_vals_[n->name] = value;
    if (n->unqualified_text_id != kInvalidText) {
      global_const_vals_by_text_[n->unqualified_text_id] = value;
    }
    if (auto key = sema_symbol_name_key(n); key.has_value() && key->valid()) {
      global_const_vals_by_key_[to_consteval_key(*key)] = value;
    }
  }

  void record_local_const_binding(const Node* n, long long value) {
    if (!n || !n->name || !n->name[0]) return;
    if (local_const_vals_scopes_.empty()) local_const_vals_scopes_.emplace_back();
    local_const_vals_scopes_.back()[n->name] = value;
    if (n->unqualified_text_id != kInvalidText) {
      if (local_const_vals_scopes_by_text_.empty()) local_const_vals_scopes_by_text_.emplace_back();
      local_const_vals_scopes_by_text_.back()[n->unqualified_text_id] = value;
    }
    if (auto key = sema_local_name_key(n); key.has_value() && key->valid()) {
      if (local_const_vals_scopes_by_key_.empty()) local_const_vals_scopes_by_key_.emplace_back();
      local_const_vals_scopes_by_key_.back()[to_consteval_key(*key)] = value;
    }
  }

  static std::optional<SemaStructuredNameKey> enum_variant_global_key(
      const Node* n, int index) {
    if (!n || n->kind != NK_ENUM_DEF || index < 0 || index >= n->n_enum_variants ||
        !n->enum_name_text_ids || n->namespace_context_id < 0) {
      return std::nullopt;
    }
    const TextId text_id = n->enum_name_text_ids[index];
    if (text_id == kInvalidText) return std::nullopt;
    SemaStructuredNameKey key;
    key.namespace_context_id = n->namespace_context_id;
    key.base_text_id = text_id;
    return key;
  }

  static std::optional<SemaStructuredNameKey> enum_variant_global_qualified_key(
      const Node* n, int index) {
    auto key = enum_variant_global_key(n, index);
    if (!key.has_value()) return std::nullopt;
    if (n->n_qualifier_segments <= 0) return key;
    if (!n->qualifier_text_ids) return std::nullopt;
    key->qualifier_text_ids.reserve(static_cast<std::size_t>(n->n_qualifier_segments));
    for (int i = 0; i < n->n_qualifier_segments; ++i) {
      const TextId segment = n->qualifier_text_ids[i];
      if (segment == kInvalidText) return std::nullopt;
      key->qualifier_text_ids.push_back(segment);
    }
    return key;
  }

  static std::optional<SemaStructuredNameKey> enum_variant_local_key(
      const Node* n, int index) {
    if (!n || n->kind != NK_ENUM_DEF || index < 0 || index >= n->n_enum_variants ||
        !n->enum_name_text_ids) {
      return std::nullopt;
    }
    const TextId text_id = n->enum_name_text_ids[index];
    if (text_id == kInvalidText) return std::nullopt;
    SemaStructuredNameKey key;
    key.base_text_id = text_id;
    return key;
  }

  void bind_enum_constants_global(const Node* n) {
    if (!n || n->kind != NK_ENUM_DEF || n->n_enum_variants <= 0 || !n->enum_names) return;
    TypeSpec its = make_int_ts();
    for (int i = 0; i < n->n_enum_variants; ++i) {
      if (!n->enum_names[i] || !n->enum_names[i][0]) continue;
      auto [it, inserted] = enum_consts_.insert_or_assign(n->enum_names[i], its);
      (void)inserted;
      if (auto key = enum_variant_global_key(n, i); key.has_value() && key->valid()) {
        structured_enum_const_names_.insert(n->enum_names[i]);
        structured_enum_consts_[*key] = &it->second;
        structured_enum_const_keys_by_name_[n->enum_names[i]] = *key;
      }
      if (auto key = enum_variant_global_qualified_key(n, i);
          key.has_value() && key->valid()) {
        structured_enum_const_names_.insert(n->enum_names[i]);
        structured_enum_consts_[*key] = &it->second;
        structured_enum_const_keys_by_name_[n->enum_names[i]] = *key;
      }
      if (n->enum_vals) {
        enum_const_vals_global_[n->enum_names[i]] = n->enum_vals[i];
        if (n->enum_name_text_ids && n->enum_name_text_ids[i] != kInvalidText) {
          enum_const_vals_global_by_text_[n->enum_name_text_ids[i]] = n->enum_vals[i];
        }
        if (auto key = enum_variant_global_key(n, i); key.has_value() && key->valid()) {
          enum_const_vals_global_by_key_[to_consteval_key(*key)] = n->enum_vals[i];
        }
      }
    }
  }

  void bind_enum_constants_local(const Node* n) {
    if (!n || n->kind != NK_ENUM_DEF || n->n_enum_variants <= 0 || !n->enum_names) return;
    TypeSpec its = make_int_ts();
    for (int i = 0; i < n->n_enum_variants; ++i) {
      if (!n->enum_names[i] || !n->enum_names[i][0]) continue;
      const auto key = enum_variant_local_key(n, i);
      bind_local(n->enum_names[i], its, true, n->line, key);
      if (n->enum_vals) {
        if (enum_const_vals_scopes_.empty()) enum_const_vals_scopes_.emplace_back();
        enum_const_vals_scopes_.back()[n->enum_names[i]] = n->enum_vals[i];
        if (n->enum_name_text_ids && n->enum_name_text_ids[i] != kInvalidText) {
          if (enum_const_vals_scopes_by_text_.empty())
            enum_const_vals_scopes_by_text_.emplace_back();
          enum_const_vals_scopes_by_text_.back()[n->enum_name_text_ids[i]] = n->enum_vals[i];
        }
        if (key.has_value() && key->valid()) {
          if (enum_const_vals_scopes_by_key_.empty())
            enum_const_vals_scopes_by_key_.emplace_back();
          enum_const_vals_scopes_by_key_.back()[to_consteval_key(*key)] = n->enum_vals[i];
        }
      }
    }
  }

  void mark_initialized_if_local_var(const Node* n) {
    if (!n) return;
    // Drill through member access and indexing to find the base variable.
    if (n->kind == NK_MEMBER || n->kind == NK_INDEX || n->kind == NK_DEREF ||
        n->kind == NK_ADDR) {
      mark_initialized_if_local_var(n->left);
      return;
    }
    if (n->kind != NK_VAR || !n->name || !n->name[0]) return;
    const auto key = sema_local_name_key(n);
    for (std::size_t i = scopes_.size(); i > 0; --i) {
      auto& scope = scopes_[i - 1];
      auto f = scope.find(n->name);
      ScopedSym* structured = nullptr;
      if (f != scope.end()) {
        if (key.has_value() && i <= structured_scopes_.size()) {
          auto sf = structured_scopes_[i - 1].find(*key);
          if (sf != structured_scopes_[i - 1].end()) structured = sf->second;
          (void)compare_sema_lookup_ptrs(&f->second, structured);
        }
        f->second.initialized = true;
        return;
      }
      if (key.has_value() && i <= structured_scopes_.size()) {
        auto sf = structured_scopes_[i - 1].find(*key);
        if (sf != structured_scopes_[i - 1].end()) {
          structured = sf->second;
          (void)compare_sema_lookup_ptrs(static_cast<const ScopedSym*>(nullptr), structured);
          structured->initialized = true;
          return;
        }
      }
    }
  }

  bool is_complete_object_type(const TypeSpec& ts) const {
    if (ts.ptr_level > 0 || ts.array_rank > 0 || ts.is_lvalue_ref || ts.is_rvalue_ref) return true;
    if (ts.tpl_struct_origin) return true;  // pending template struct — resolved at HIR level
    if ((ts.base != TB_STRUCT && ts.base != TB_UNION) || !ts.tag || !ts.tag[0]) return true;
    const std::string tag(ts.tag);
    const bool legacy_complete =
        ts.base == TB_STRUCT ? complete_structs_.count(tag) > 0
                             : complete_unions_.count(tag) > 0;
    if (auto record_key = structured_record_key_for_tag(tag); record_key.has_value()) {
      const bool structured_complete =
          ts.base == TB_STRUCT ? complete_structs_by_key_.count(*record_key) > 0
                               : complete_unions_by_key_.count(*record_key) > 0;
      (void)compare_sema_lookup_presence(legacy_complete, structured_complete);
    }
    return legacy_complete;
  }

  static std::string unqualified_name(const std::string& name) {
    const size_t sep = name.rfind("::");
    return sep == std::string::npos ? name : name.substr(sep + 2);
  }

  std::optional<SemaStructuredNameKey> structured_record_key_for_tag(
      const std::string& tag) const {
    if (ambiguous_structured_record_tags_.count(tag) > 0) return std::nullopt;
    auto it = structured_record_keys_by_tag_.find(tag);
    if (it == structured_record_keys_by_tag_.end()) return std::nullopt;
    return it->second;
  }

  void note_structured_record_key_for_tag(const std::string& tag,
                                          const SemaStructuredNameKey& key) {
    if (ambiguous_structured_record_tags_.count(tag) > 0) return;
    auto [it, inserted] = structured_record_keys_by_tag_.emplace(tag, key);
    if (!inserted && it->second != key) {
      structured_record_keys_by_tag_.erase(it);
      ambiguous_structured_record_tags_.insert(tag);
    }
  }

  void note_struct_def(const Node* n) {
    if (!n || n->kind != NK_STRUCT_DEF || !n->name || !n->name[0]) return;
    // Zero-sized structs/unions are a GCC extension; treat them as complete.
    const std::string tag(n->name);
    const auto record_key = sema_symbol_name_key(n);
    if (record_key.has_value() && record_key->valid()) {
      note_structured_record_key_for_tag(tag, *record_key);
    }
    struct_defs_by_unqualified_name_[unqualified_name(tag)].push_back(n);
    if (record_key.has_value() && record_key->valid()) struct_defs_by_key_[*record_key] = n;
    if (n->is_union) {
      complete_unions_.insert(tag);
      if (record_key.has_value() && record_key->valid()) complete_unions_by_key_.insert(*record_key);
    } else {
      complete_structs_.insert(tag);
      if (record_key.has_value() && record_key->valid()) complete_structs_by_key_.insert(*record_key);
    }
    for (int i = 0; i < n->n_fields; ++i) {
      if (!n->fields[i] || !n->fields[i]->name || !n->fields[i]->name[0]) continue;
      if (record_key.has_value() && record_key->valid() &&
          n->fields[i]->unqualified_text_id != kInvalidText) {
        struct_static_member_types_by_key_[*record_key][n->fields[i]->unqualified_text_id] =
            n->fields[i]->type;
      }
      if (!n->fields[i]->is_static) {
        if (record_key.has_value() && record_key->valid() &&
            n->fields[i]->unqualified_text_id != kInvalidText) {
          struct_field_text_ids_by_key_[*record_key].insert(n->fields[i]->unqualified_text_id);
          auto& field_text_ids_by_name = struct_field_text_ids_by_name_by_key_[*record_key];
          auto [it, inserted] = field_text_ids_by_name.emplace(
              n->fields[i]->name, n->fields[i]->unqualified_text_id);
          if (!inserted && it->second != n->fields[i]->unqualified_text_id) {
            it->second = kInvalidText;
          }
        }
      }
    }
    for (int i = 0; i < n->n_bases; ++i) {
      const TypeSpec& base = n->base_types[i];
      if (record_key.has_value() && record_key->valid() && base.tag && base.tag[0]) {
        if (auto base_key = structured_record_key_for_tag(base.tag); base_key.has_value()) {
          struct_base_keys_by_key_[*record_key].push_back(*base_key);
        }
      }
    }
  }

  std::optional<TypeSpec> lookup_struct_static_member_type_by_key(
      const SemaStructuredNameKey& record_key, TextId member_text_id,
      bool* has_metadata = nullptr) const {
    if (member_text_id == kInvalidText) return std::nullopt;
    auto sit = struct_static_member_types_by_key_.find(record_key);
    if (sit != struct_static_member_types_by_key_.end()) {
      if (has_metadata) *has_metadata = true;
      auto mit = sit->second.find(member_text_id);
      if (mit != sit->second.end()) return mit->second;
    }
    auto bit = struct_base_keys_by_key_.find(record_key);
    if (bit != struct_base_keys_by_key_.end()) {
      for (const auto& base_key : bit->second) {
        bool base_has_metadata = false;
        auto from_base = lookup_struct_static_member_type_by_key(
            base_key, member_text_id, &base_has_metadata);
        if (base_has_metadata && has_metadata) *has_metadata = true;
        if (from_base.has_value()) return from_base;
      }
    }
    return std::nullopt;
  }

  static std::optional<SemaStructuredNameKey> static_member_owner_key_from_reference(
      const Node* reference) {
    if (!reference || reference->namespace_context_id < 0 ||
        reference->unqualified_text_id == kInvalidText ||
        reference->n_qualifier_segments <= 0 || !reference->qualifier_text_ids) {
      return std::nullopt;
    }
    const TextId owner_text_id =
        reference->qualifier_text_ids[reference->n_qualifier_segments - 1];
    if (owner_text_id == kInvalidText) return std::nullopt;
    SemaStructuredNameKey key;
    key.namespace_context_id = reference->namespace_context_id;
    key.base_text_id = owner_text_id;
    return key;
  }

  bool static_member_lookup_has_structured_metadata(
      const std::string& tag, const Node* reference) const {
    if (!reference || reference->unqualified_text_id == kInvalidText) return false;
    auto reference_key = static_member_owner_key_from_reference(reference);
    if (reference_key.has_value() && reference_key->valid()) {
      bool has_metadata = false;
      (void)lookup_struct_static_member_type_by_key(
          *reference_key, reference->unqualified_text_id, &has_metadata);
      if (reference->n_template_args > 0) return false;
      if (has_metadata) return true;
      auto rendered_key = structured_record_key_for_tag(tag);
      return rendered_key.has_value() && *rendered_key != *reference_key;
    }
    return false;
  }

  std::optional<TypeSpec> lookup_struct_static_member_type(
      const std::string& tag, const Node* reference) const {
    if (reference && reference->unqualified_text_id != kInvalidText) {
      auto reference_key = static_member_owner_key_from_reference(reference);
      if (reference_key.has_value() && reference_key->valid()) {
        bool has_metadata = false;
        auto structured = lookup_struct_static_member_type_by_key(
            *reference_key, reference->unqualified_text_id, &has_metadata);
        if (structured.has_value() ||
            (has_metadata && reference->n_template_args <= 0)) {
          return structured;
        }
      }
      if (auto record_key = structured_record_key_for_tag(tag); record_key.has_value()) {
        auto structured = lookup_struct_static_member_type_by_key(
            *record_key, reference->unqualified_text_id);
        if (structured.has_value()) return structured;
      }
    }
    return std::nullopt;
  }

  std::optional<bool> has_struct_instance_field_by_key(
      const SemaStructuredNameKey& record_key, TextId member_text_id) const {
    if (member_text_id == kInvalidText) return std::nullopt;
    bool has_structured_metadata = false;
    auto fit = struct_field_text_ids_by_key_.find(record_key);
    if (fit != struct_field_text_ids_by_key_.end()) {
      has_structured_metadata = true;
      if (fit->second.count(member_text_id)) return true;
    }
    auto bit = struct_base_keys_by_key_.find(record_key);
    if (bit != struct_base_keys_by_key_.end()) {
      for (const auto& base_key : bit->second) {
        auto from_base = has_struct_instance_field_by_key(base_key, member_text_id);
        if (from_base.value_or(false)) return true;
        if (from_base.has_value()) has_structured_metadata = true;
      }
    }
    if (has_structured_metadata) return false;
    return std::nullopt;
  }

  std::optional<TextId> resolve_struct_instance_field_text_id_by_name(
      const SemaStructuredNameKey& record_key, const char* member) const {
    if (!member || !member[0]) return std::nullopt;
    auto fit = struct_field_text_ids_by_name_by_key_.find(record_key);
    if (fit != struct_field_text_ids_by_name_by_key_.end()) {
      auto mit = fit->second.find(member);
      if (mit != fit->second.end()) {
        return mit->second == kInvalidText ? std::nullopt : std::optional<TextId>(mit->second);
      }
    }

    std::optional<TextId> found;
    auto bit = struct_base_keys_by_key_.find(record_key);
    if (bit == struct_base_keys_by_key_.end()) return found;
    for (const auto& base_key : bit->second) {
      auto from_base = resolve_struct_instance_field_text_id_by_name(base_key, member);
      if (!from_base.has_value()) continue;
      if (found.has_value() && *found != *from_base) return std::nullopt;
      found = *from_base;
    }
    return found;
  }

  bool has_struct_instance_field(TextId member_text_id, const char* member) const {
    if (!current_method_struct_key_.has_value() || !current_method_struct_key_->valid()) {
      return false;
    }
    if (member_text_id == kInvalidText) {
      auto resolved_text_id = resolve_struct_instance_field_text_id_by_name(
          *current_method_struct_key_, member);
      if (!resolved_text_id.has_value()) return false;
      member_text_id = *resolved_text_id;
    }
    const auto structured =
        has_struct_instance_field_by_key(*current_method_struct_key_, member_text_id);
    return structured.value_or(false);
  }

  std::optional<SemaStructuredNameKey> method_owner_key_from_qualifier(
      const Node* fn, const std::string& owner) const {
    if (!fn || fn->namespace_context_id < 0 || fn->n_qualifier_segments <= 0 ||
        !fn->qualifier_text_ids) {
      return std::nullopt;
    }
    const TextId owner_text_id = fn->qualifier_text_ids[fn->n_qualifier_segments - 1];
    if (owner_text_id == kInvalidText) return std::nullopt;
    SemaStructuredNameKey key;
    key.namespace_context_id = fn->namespace_context_id;
    key.base_text_id = owner_text_id;
    const bool structured_owner_key_exists =
        struct_defs_by_key_.find(key) != struct_defs_by_key_.end();
    if (structured_owner_key_exists) return key;
    if (owner.find("::") != std::string::npos || fn->n_qualifier_segments != 1 ||
        !fn->qualifier_segments || !fn->qualifier_segments[0]) {
      return std::nullopt;
    }
    const std::string qualifier_owner(fn->qualifier_segments[0]);
    if (qualifier_owner != unqualified_name(owner)) return std::nullopt;
    return key;
  }

  const Node* resolve_owner_in_namespace_context(
      const std::string& owner, int namespace_context_id,
      const std::optional<SemaStructuredNameKey>& structured_owner_key) const {
    if (owner.empty() || namespace_context_id < 0) return nullptr;
    if (structured_owner_key.has_value() && structured_owner_key->valid()) {
      auto it = struct_defs_by_key_.find(*structured_owner_key);
      return it != struct_defs_by_key_.end() ? it->second : nullptr;
    }
    return nullptr;
  }

  const Node* enclosing_method_owner_record(const Node* fn) const {
    if (auto owner = qualified_method_owner_struct(fn); owner.has_value()) {
      if (const Node* contextual = resolve_owner_in_namespace_context(
              *owner, fn ? fn->namespace_context_id : -1,
              method_owner_key_from_qualifier(fn, *owner))) {
        return contextual;
      }
      return nullptr;
    }
    auto it = method_owner_records_.find(fn);
    if (it == method_owner_records_.end() || !it->second || !it->second->name ||
        !it->second->name[0]) {
      return nullptr;
    }
    return it->second;
  }

  std::optional<std::string> enclosing_method_owner_struct_compatibility(const Node* fn) const {
    if (const Node* record = enclosing_method_owner_record(fn)) return std::string(record->name);
    if (auto owner = qualified_method_owner_struct(fn); owner.has_value()) {
      if (auto key = method_owner_key_from_qualifier(fn, *owner);
          key.has_value() && key->valid()) {
        return std::nullopt;
      }
      if (complete_structs_.count(*owner) || complete_unions_.count(*owner)) return owner;
      // Do not guess across namespaces once direct contextual and canonical-tag
      // lookup has failed. Unqualified owners must resolve through the
      // declaration namespace; qualified owners must already name the record.
      if (owner->find("::") != std::string::npos) return owner;
      return std::nullopt;
    }
    return std::nullopt;
  }

  bool can_defer_owner_qualified_cast_typedef(const TypeSpec& ts) const {
    if (current_method_struct_tag_.empty() || !ts.tag || !ts.tag[0]) return false;
    if (ts.is_global_qualified) return false;
    return std::strstr(ts.tag, "::") != nullptr;
  }

  void bind_template_nttps(const Node* n, int line) {
    if (!n || n->n_template_params <= 0 || !n->template_param_names) return;
    for (int i = 0; i < n->n_template_params; ++i) {
      if (!(n->template_param_is_nttp && n->template_param_is_nttp[i]) ||
          !n->template_param_names[i]) {
        continue;
      }
      TypeSpec nttp_ts{};
      nttp_ts.base = TB_INT;
      nttp_ts.array_size = -1;
      nttp_ts.inner_rank = -1;
      bind_local(n->template_param_names[i], nttp_ts, true, line,
                 sema_template_param_local_name_key(n, i));
    }
  }

  void record_template_type_param(const char* name, TextId text_id) {
    if (!name || !name[0]) return;
    template_type_params_.insert(name);
    if (text_id == kInvalidText) return;
    template_type_param_text_ids_.insert(text_id);
    auto [it, inserted] = template_type_param_text_id_by_name_.emplace(name, text_id);
    if (!inserted && it->second != text_id) {
      it->second = kInvalidText;
    }
  }

  bool template_type_param_mirror_matches_name(const char* name) const {
    if (!name || !name[0]) return false;
    auto it = template_type_param_text_id_by_name_.find(name);
    if (it == template_type_param_text_id_by_name_.end() || it->second == kInvalidText) {
      return false;
    }
    return template_type_param_text_ids_.count(it->second) > 0;
  }

  bool is_known_template_type_param_name(const char* name) const {
    if (!name || !name[0]) return false;
    const bool legacy = template_type_params_.count(name) > 0;
    const bool structured = template_type_param_mirror_matches_name(name);
    if (template_type_param_text_id_by_name_.count(name) > 0) {
      (void)compare_sema_lookup_presence(legacy, structured);
      return structured;
    }
    return legacy;
  }

  bool is_current_template_type_param_name(const char* name) const {
    if (!current_fn_node_ || !name || !name[0]) return false;
    bool legacy = false;
    bool structured = false;
    bool saw_structured_candidate = false;
    for (int i = 0; i < current_fn_node_->n_template_params; ++i) {
      if (!sema_template_param_is_type_param(current_fn_node_, i)) continue;
      const char* param_name = current_fn_node_->template_param_names[i];
      const bool name_matches = std::strcmp(param_name, name) == 0;
      legacy = legacy || name_matches;

      const TextId text_id = sema_template_param_name_text_id(current_fn_node_, i);
      if (!name_matches || text_id == kInvalidText) continue;
      saw_structured_candidate = true;
      if (template_type_param_text_ids_.count(text_id) > 0) {
        structured = true;
      }
    }
    if (saw_structured_candidate) {
      (void)compare_sema_lookup_presence(legacy, structured);
      return structured;
    }
    return legacy;
  }

  void record_template_type_params_recursive(const Node* n) {
    if (!n) return;
    if (n->n_template_params > 0 &&
        n->template_param_names) {
      for (int i = 0; i < n->n_template_params; ++i) {
        if (sema_template_param_is_type_param(n, i)) {
          record_template_type_param(n->template_param_names[i],
                                     sema_template_param_name_text_id(n, i));
        }
      }
    }
    for (int i = 0; i < n->n_children; ++i) {
      record_template_type_params_recursive(n->children[i]);
    }
    for (int i = 0; i < n->n_fields; ++i) {
      record_template_type_params_recursive(n->fields[i]);
    }
    for (int i = 0; i < n->n_params; ++i) {
      record_template_type_params_recursive(n->params[i]);
    }
    record_template_type_params_recursive(n->body);
    record_template_type_params_recursive(n->left);
    record_template_type_params_recursive(n->right);
    record_template_type_params_recursive(n->cond);
    record_template_type_params_recursive(n->then_);
    record_template_type_params_recursive(n->else_);
    record_template_type_params_recursive(n->init);
    record_template_type_params_recursive(n->update);
  }

  bool looks_like_template_placeholder_name(const char* name) const {
    if (!name || !name[0]) return false;
    if (complete_structs_.count(name) > 0 || complete_unions_.count(name) > 0 ||
        globals_.count(name) > 0 || enum_consts_.count(name) > 0 ||
        funcs_.count(name) > 0) {
      return false;
    }
    bool saw_upper = false;
    for (const unsigned char* p =
             reinterpret_cast<const unsigned char*>(name);
         *p; ++p) {
      if (std::isupper(*p)) {
        saw_upper = true;
        continue;
      }
      if (std::isdigit(*p) || *p == '_') continue;
      return false;
    }
    return saw_upper;
  }

  void collect_toplevel_node(const Node* n) {
    if (!n) return;
    record_template_type_params_recursive(n);
    if (n->kind == NK_BLOCK) {
      // Multi-declarator global block: recurse into children.
      for (int i = 0; i < n->n_children; ++i) collect_toplevel_node(n->children[i]);
      return;
    }
    if (n->kind == NK_GLOBAL_VAR) {
      bind_global(n);
      if (!is_complete_object_type(n->type)) {
        emit(n, "object has incomplete struct/union type");
      }
    } else if (n->kind == NK_FUNCTION) {
      if (!n->name || !n->name[0]) return;
      if (n->is_consteval && n->body) record_consteval_function(n);
      if (qualified_method_owner_struct(n).has_value()) return;
      FunctionSig sig;
      sig.ret = n->type;
      sig.variadic = n->variadic;
      // K&R-style f() with no param list means "unspecified arguments".
      if (n->n_params == 0 && !n->variadic) sig.unspecified_params = true;
      for (int p = 0; p < n->n_params; ++p) {
        const Node* param = n->params[p];
        if (!param) continue;
        // In C, `f(void)` means no parameters; skip the void sentinel param.
        const TypeSpec& pt = param->type;
        if (pt.base == TB_VOID && pt.ptr_level == 0 && pt.array_rank == 0) continue;
        sig.params.push_back(pt);
        if (param->is_parameter_pack) sig.has_param_pack = true;
      }
      auto cpp_ov = cpp_overload_sigs_.find(n->name);
      if (cpp_ov != cpp_overload_sigs_.end()) {
        bool matched = false;
        for (FunctionSig& existing : cpp_ov->second) {
          if (!function_sig_compatible(existing, sig)) continue;
          matched = true;
          if (existing.unspecified_params && !sig.unspecified_params) {
            existing = std::move(sig);
          }
          break;
        }
        if (!matched && !n->is_explicit_specialization) {
          if (supports_cpp_overload_set(n->name)) {
            cpp_ov->second.push_back(std::move(sig));
          } else {
            emit(n, std::string("conflicting types for function '") + n->name + "'");
          }
        }
      } else {
        auto it = funcs_.find(n->name);
        if (it != funcs_.end()) {
        if (!function_sig_compatible(it->second, sig) && !n->is_explicit_specialization) {
          // Check if this is a ref-overload (T& vs T&&) before emitting error.
          if (is_ref_overload(it->second, sig)) {
            auto& ovset = ref_overload_sigs_[n->name];
            if (ovset.empty()) ovset.push_back(it->second);
            ovset.push_back(std::move(sig));
          } else if (supports_cpp_overload_set(n->name)) {
            record_cpp_overload(n->name, std::move(sig));
          } else {
            emit(n, std::string("conflicting types for function '") + n->name + "'");
          }
        } else if (it->second.unspecified_params && !sig.unspecified_params) {
          // Upgrade K&R unspecified-params declaration to the full prototype.
          it->second = std::move(sig);
        }
        } else {
          funcs_[n->name] = std::move(sig);
        }
      }
      mirror_function_decl(n);
    } else if (n->kind == NK_STRUCT_DEF) {
      note_struct_def(n);
      for (int i = 0; i < n->n_children; ++i) {
        const Node* child = n->children[i];
        if (child && child->kind == NK_FUNCTION) method_owner_records_[child] = n;
      }
    } else if (n->kind == NK_ENUM_DEF) {
      bind_enum_constants_global(n);
    }
  }

  void collect_toplevel(const Node* root) {
    for (int i = 0; i < root->n_children; ++i) {
      collect_toplevel_node(root->children[i]);
    }
  }

  void validate_toplevel_node(const Node* n) {
    if (!n) return;
    if (n->kind == NK_BLOCK) {
      for (int i = 0; i < n->n_children; ++i) validate_toplevel_node(n->children[i]);
      return;
    }
    if (n->kind == NK_FUNCTION) {
      validate_function(n);
    } else if (n->kind == NK_GLOBAL_VAR) {
      validate_global(n);
    } else if (n->kind == NK_STATIC_ASSERT) {
      validate_static_assert(n, nullptr);
    } else if (n->kind == NK_STRUCT_DEF) {
      note_struct_def(n);
      for (int i = 0; i < n->n_children; ++i) {
        const Node* child = n->children[i];
        if (child && child->kind == NK_STATIC_ASSERT) {
          validate_static_assert(child, n);
        }
      }
    } else if (n->kind == NK_ENUM_DEF) {
      bind_enum_constants_global(n);
    }
  }

  void validate_static_assert(const Node* n, const Node* template_owner) {
    if (!n || n->kind != NK_STATIC_ASSERT || !n->left) return;

    ConstEvalEnv env;
    populate_const_eval_env(env);

    if (auto r = evaluate_constant_expr(n->left, env); r.ok()) {
      if (r.as_int() == 0) emit(n, "_Static_assert condition is false");
      return;
    }

    if (n->left->kind == NK_CALL && n->left->left &&
        n->left->left->kind == NK_VAR && n->left->left->name) {
      const Node* consteval_fn =
          lookup_consteval_function_by_name(n->left->left->name, n->left->left);
      if (consteval_fn) {
        std::vector<hir::ConstValue> args;
        bool args_ok = true;
        for (int i = 0; i < n->left->n_children; ++i) {
          auto arg = evaluate_constant_expr(n->left->children[i], env);
          if (!arg.ok()) {
            args_ok = false;
            break;
          }
          args.push_back(*arg.value);
        }
        if (args_ok) {
          hir::TypeBindings tpl_bindings;
          hir::TypeBindingTextMap tpl_bindings_by_text;
          hir::TypeBindingStructuredMap tpl_bindings_by_key;
          hir::TypeBindingNameTextMap tpl_binding_text_ids_by_name;
          hir::TypeBindingNameStructuredMap tpl_binding_keys_by_name;
          std::unordered_map<std::string, long long> nttp_bindings;
          hir::ConstTextMap nttp_bindings_by_text;
          hir::ConstStructuredMap nttp_bindings_by_key;
          ConstEvalEnv call_env = hir::bind_consteval_call_env(
              n->left->left, consteval_fn, env, &tpl_bindings, &nttp_bindings,
              &tpl_bindings_by_text, &tpl_bindings_by_key,
              &tpl_binding_text_ids_by_name, &tpl_binding_keys_by_name,
              &nttp_bindings_by_text, &nttp_bindings_by_key);
          auto r = hir::evaluate_consteval_call(
              consteval_fn, args, call_env, consteval_funcs_, 0,
              &consteval_funcs_by_text_, &consteval_funcs_by_key_);
          if (r.ok()) {
            if (r.as_int() == 0) emit(n, "_Static_assert condition is false");
            return;
          }
        }
      }
    }

    if (template_owner_can_defer_static_assert(template_owner) ||
        template_owner_can_defer_static_assert(current_fn_node_) ||
        (n->n_template_params > 0)) {
      return;
    }
  }

  void validate_global(const Node* n) {
    if (!n) return;
    // Skip reference-init checks for template-parameterized declarations;
    // these are template function declarations that the parser may not fully
    // understand (e.g. reference-returning functions like char(&f(T(&x)[N]))[N]).
    if (n->n_template_params > 0) return;
    if (n->type.is_lvalue_ref && !n->init) {
      emit(n, "lvalue reference must be initialized");
    }
    if (n->type.is_rvalue_ref && !n->init) {
      emit(n, "rvalue reference must be initialized");
    }
    if (n->init) {
      ExprInfo rhs = infer_expr(n->init);
      if (n->type.is_lvalue_ref && !rhs.is_lvalue) {
        emit(n, "lvalue reference must bind to an lvalue");
      }
      if (n->type.is_rvalue_ref && rhs.is_lvalue) {
        emit(n, "rvalue reference cannot bind to an lvalue");
      }
      if (rhs.valid &&
          is_invalid_pointer_float_implicit_conversion(
              referred_type(n->type), rhs.type, is_null_pointer_constant_expr(n->init))) {
        emit(n, "incompatible initializer type");
      }
      if (n->name && n->name[0] &&
          (n->type.is_const || n->is_constexpr) &&
          is_const_int_binding_type(n->type)) {
        ConstEvalEnv env;
        populate_const_eval_env(env);
        if (auto r = evaluate_constant_expr(n->init, env); r.ok()) {
          record_global_const_binding(n, r.as_int());
        }
      }
    }
  }

  static bool node_contains_goto(const Node* n) {
    if (!n) return false;
    if (n->kind == NK_GOTO) return true;
    if (n->left && node_contains_goto(n->left)) return true;
    if (n->right && node_contains_goto(n->right)) return true;
    if (n->cond && node_contains_goto(n->cond)) return true;
    if (n->then_ && node_contains_goto(n->then_)) return true;
    if (n->else_ && node_contains_goto(n->else_)) return true;
    if (n->body && node_contains_goto(n->body)) return true;
    if (n->init && node_contains_goto(n->init)) return true;
    if (n->update && node_contains_goto(n->update)) return true;
    for (int i = 0; i < n->n_children; ++i)
      if (node_contains_goto(n->children[i])) return true;
    return false;
  }

  static bool node_contains_loop(const Node* n) {
    if (!n) return false;
    if (n->kind == NK_FOR || n->kind == NK_WHILE || n->kind == NK_DO_WHILE) return true;
    if (n->left && node_contains_loop(n->left)) return true;
    if (n->right && node_contains_loop(n->right)) return true;
    if (n->cond && node_contains_loop(n->cond)) return true;
    if (n->then_ && node_contains_loop(n->then_)) return true;
    if (n->else_ && node_contains_loop(n->else_)) return true;
    if (n->body && node_contains_loop(n->body)) return true;
    if (n->init && node_contains_loop(n->init)) return true;
    if (n->update && node_contains_loop(n->update)) return true;
    for (int i = 0; i < n->n_children; ++i)
      if (node_contains_loop(n->children[i])) return true;
    return false;
  }

  void validate_function(const Node* fn) {
    const bool old_in_function = in_function_;
    const TypeSpec old_fn_ret = current_fn_ret_;
    const Node* old_fn_node = current_fn_node_;
    const std::string old_method_struct = current_method_struct_tag_;
    const std::optional<SemaStructuredNameKey> old_method_struct_key = current_method_struct_key_;
    current_method_struct_tag_.clear();
    current_method_struct_key_.reset();
    in_function_ = true;
    current_fn_ret_ = fn->type;
    current_fn_node_ = fn;
    fn_has_goto_ = fn->body && node_contains_goto(fn->body);
    fn_has_loop_ = fn->body && node_contains_loop(fn->body);
    enter_scope();
    // Pre-defined function-name identifiers (C99 §6.4.2.2, GCC extension).
    {
      TypeSpec func_ts{};
      func_ts.base = TB_CHAR;
      func_ts.ptr_level = 1;
      func_ts.is_const = true;
      bind_local("__func__", func_ts, true, 0,
                 sema_injected_local_name_key(fn, "__func__"));
      bind_local("__FUNCTION__", func_ts, true, 0,
                 sema_injected_local_name_key(fn, "__FUNCTION__"));
      bind_local("__PRETTY_FUNCTION__", func_ts, true, 0,
                 sema_injected_local_name_key(fn, "__PRETTY_FUNCTION__"));
    }
    // Inject non-type template parameter names so dependent expressions in
    // templated function and method bodies can validate before instantiation.
    bind_template_nttps(fn, fn->line);
    const Node* enclosing_record = nullptr;
    auto owner_it = method_owner_records_.find(fn);
    if (owner_it != method_owner_records_.end()) enclosing_record = owner_it->second;
    bind_template_nttps(enclosing_record, fn->line);
    for (int i = 0; i < fn->n_params; ++i) {
      const Node* p = fn->params[i];
      if (!p || !p->name || !p->name[0]) continue;
      bind_local(p->name, p->type, true, p->line, sema_local_name_key(p));
    }
    if (const Node* owner_record = enclosing_method_owner_record(fn)) {
      current_method_struct_tag_ = owner_record->name ? owner_record->name : "";
      current_method_struct_key_ = sema_symbol_name_key(owner_record);
    } else if (auto owner = enclosing_method_owner_struct_compatibility(fn); owner.has_value()) {
      current_method_struct_tag_ = *owner;
      current_method_struct_key_ = structured_record_key_for_tag(*owner);
    }
    if (!current_method_struct_tag_.empty()) {
      TypeSpec this_ts{};
      this_ts.base = TB_STRUCT;
      this_ts.tag = current_method_struct_tag_.c_str();
      this_ts.ptr_level = 1;
      this_ts.array_size = -1;
      this_ts.inner_rank = -1;
      // In this codebase, is_const with ptr_level>0 models pointee constness.
      this_ts.is_const = fn->is_const_method;
      bind_local("this", this_ts, true, fn->line,
                 sema_injected_local_name_key(fn, "this"));
    }

    // Validate constructor initializer list expressions.
    for (int i = 0; i < fn->n_ctor_inits; ++i) {
      for (int j = 0; j < fn->ctor_init_nargs[i]; ++j) {
        if (fn->ctor_init_args[i][j]) (void)infer_expr(fn->ctor_init_args[i][j]);
      }
    }

    // Skip body validation for template functions — bodies may contain
    // template-dependent expressions (e.g. Template<T>::value) that can't
    // be resolved without instantiation.
    if (fn->body && fn->n_template_params == 0) {
      if (fn->body->kind == NK_BLOCK) {
        for (int i = 0; i < fn->body->n_children; ++i) visit_stmt(fn->body->children[i]);
      } else {
        visit_stmt(fn->body);
      }
    }
    leave_scope();
    in_function_ = old_in_function;
    current_fn_ret_ = old_fn_ret;
    current_fn_node_ = old_fn_node;
    current_method_struct_tag_ = old_method_struct;
    current_method_struct_key_ = old_method_struct_key;
  }


  void validate_decl_init(const Node* decl, const TypeSpec* effective_type = nullptr) {
    if (!decl) return;
    const TypeSpec decl_ts = effective_type ? *effective_type : decl->type;
    if (!is_complete_object_type(decl_ts)) {
      emit(decl, "object has incomplete struct/union type");
    }
    // Constructor-initialized declarations: validate the constructor args.
    if (decl->is_ctor_init) {
      for (int i = 0; i < decl->n_children; ++i) {
        if (decl->children[i]) infer_expr(decl->children[i]);
      }
      return;
    }
    if (decl_ts.is_lvalue_ref && !decl->init) {
      emit(decl, "lvalue reference must be initialized");
    }
    if (decl_ts.is_rvalue_ref && !decl->init) {
      emit(decl, "rvalue reference must be initialized");
    }
    if (decl->init) {
      ExprInfo rhs = infer_expr(decl->init);
      if (decl_ts.is_lvalue_ref && !rhs.is_lvalue) {
        emit(decl, "lvalue reference must bind to an lvalue");
      }
      if (decl_ts.is_rvalue_ref && rhs.is_lvalue) {
        emit(decl, "rvalue reference cannot bind to an lvalue");
      }
      if (rhs.valid &&
          is_invalid_pointer_float_implicit_conversion(
              referred_type(decl_ts), rhs.type, is_null_pointer_constant_expr(decl->init))) {
        emit(decl, "incompatible initializer type");
      }
      mark_initialized_if_local_var(decl);
    }
  }

  void visit_stmt(const Node* n) {
    if (!n) return;

    switch (n->kind) {
      case NK_BLOCK: {
        // ival == 1 marks a synthetic multi-declarator block (e.g. `int a, b;`).
        // These share the enclosing scope; only true compound-statement blocks
        // introduce a new scope.
        bool new_scope = (n->ival != 1);
        if (new_scope) enter_scope();
        for (int i = 0; i < n->n_children; ++i) visit_stmt(n->children[i]);
        if (new_scope) leave_scope();
        return;
      }
      case NK_DECL: {
        std::optional<TypeSpec> effective_decl_type;
        if (n->type.base == TB_AUTO && n->init) {
          effective_decl_type = deduce_local_decl_type(n);
        }
        if (n->name && n->name[0]) {
          bind_local(n->name, effective_decl_type ? *effective_decl_type : n->type,
                     n->init != nullptr || n->is_ctor_init, n->line, sema_local_name_key(n));
        }
        validate_decl_init(n, effective_decl_type ? &*effective_decl_type : nullptr);
        // Track const/constexpr locals with foldable initializers for case label evaluation.
        if (n->name && n->name[0] && n->init &&
            ((effective_decl_type ? effective_decl_type->is_const : n->type.is_const) ||
             n->is_constexpr) &&
            !(effective_decl_type ? effective_decl_type->is_lvalue_ref : n->type.is_lvalue_ref) &&
            !(effective_decl_type ? effective_decl_type->is_rvalue_ref : n->type.is_rvalue_ref) &&
            (effective_decl_type ? effective_decl_type->ptr_level : n->type.ptr_level) == 0 &&
            (effective_decl_type ? effective_decl_type->array_rank : n->type.array_rank) == 0) {
          ConstEvalEnv env;
          populate_const_eval_env(env);
          if (auto r = evaluate_constant_expr(n->init, env); r.ok()) {
            record_local_const_binding(n, r.as_int());
          }
        }
        return;
      }
      case NK_STATIC_ASSERT: {
        validate_static_assert(n, nullptr);
        return;
      }
      case NK_EXPR_STMT: {
        if (n->left) (void)infer_expr(n->left);
        return;
      }
      case NK_RETURN: {
        if (!in_function_) {
          emit(n, "return statement not within function");
          return;
        }

        const bool fn_returns_void =
            current_fn_ret_.base == TB_VOID &&
            current_fn_ret_.ptr_level == 0 &&
            current_fn_ret_.array_rank == 0;

        // C89/gnu89: bare `return;` in a non-void function is allowed
        // (undefined behaviour only if the caller uses the value).
        // GCC and Clang accept this with just -Wreturn-type.

        if (n->left) {
          ExprInfo rv_info = infer_expr(n->left);
          if (fn_returns_void) {
            // GCC extension: `return void_expr;` in a void function is allowed
            // when the expression itself is void (e.g. calling another void fn).
            // Use rv_info.type (inferred by infer_expr) not n->left->type (AST).
            const bool returns_void_expr =
                rv_info.type.base == TB_VOID &&
                rv_info.type.ptr_level == 0 &&
                rv_info.type.array_rank == 0;
            if (!returns_void_expr)
              emit(n, "return with a value in function returning void");
          }
          if (current_fn_ret_.is_lvalue_ref && !rv_info.is_lvalue) {
            emit(n, "lvalue reference must bind to an lvalue");
          }
          if (current_fn_ret_.is_rvalue_ref && rv_info.is_lvalue) {
            emit(n, "rvalue reference cannot bind to an lvalue");
          }
          // Detect direct return of an uninitialized plain-scalar local.
          // Skip if the function has goto/loop statements (complex control flow
          // can make the read unreachable, leading to false positives).
          const Node* rv = n->left;
          if (!fn_has_goto_ && !fn_has_loop_ &&
              rv->kind == NK_VAR && rv->name && rv->name[0]) {
            auto sym = lookup_symbol(rv->name, rv);
            if (sym.has_value() && !sym->initialized &&
                tracks_uninit_read(sym->type)) {
              emit(rv, std::string("read of uninitialized variable '") +
                              rv->name + "'");
            }
          }
          const TypeSpec return_check_ts =
              is_any_ref_ty(current_fn_ret_) ? referred_type(current_fn_ret_) : current_fn_ret_;
          const bool self_return_compatible =
              is_deref_of_this_expr(rv) &&
              same_tagged_type_ignoring_qualification(return_check_ts, rv_info.type);
          if (!fn_returns_void && rv_info.valid &&
              !implicit_convertible(
                  return_check_ts, rv_info.type, is_null_pointer_constant_expr(n->left)) &&
              !self_return_compatible) {
            emit(n, "incompatible return type");
          }
        }
        return;
      }
      case NK_IF: {
        if (n->cond) (void)infer_expr(n->cond);
        if (n->then_) visit_stmt(n->then_);
        if (n->else_) visit_stmt(n->else_);
        return;
      }
      case NK_WHILE: {
        if (n->cond) (void)infer_expr(n->cond);
        loop_depth_ += 1;
        if (n->body) visit_stmt(n->body);
        loop_depth_ -= 1;
        return;
      }
      case NK_DO_WHILE: {
        loop_depth_ += 1;
        if (n->body) visit_stmt(n->body);
        loop_depth_ -= 1;
        if (n->cond) (void)infer_expr(n->cond);
        return;
      }
      case NK_FOR: {
        enter_scope();
        if (n->init) {
          if (n->init->kind == NK_DECL || n->init->kind == NK_BLOCK) {
            visit_stmt(n->init);
          } else {
            (void)infer_expr(n->init);
          }
        }
        if (n->cond) (void)infer_expr(n->cond);
        loop_depth_ += 1;
        if (n->body) visit_stmt(n->body);
        loop_depth_ -= 1;
        if (n->update) (void)infer_expr(n->update);
        leave_scope();
        return;
      }
      case NK_RANGE_FOR: {
        enter_scope();
        // Range-for loop variable: skip "must be initialized" check for lvalue
        // references — the init is synthesized during HIR desugaring.
        if (n->init) {
          if (n->init->kind == NK_DECL && n->init->name && n->init->name[0])
            bind_local(n->init->name, n->init->type, true /*will be initialized*/,
                       n->init->line, sema_local_name_key(n->init));
          // Still validate the init expression if present, but skip ref-init checks
          if (n->init->init) (void)infer_expr(n->init->init);
        }
        if (n->right) (void)infer_expr(n->right); // range expression
        loop_depth_ += 1;
        if (n->body) visit_stmt(n->body);
        loop_depth_ -= 1;
        leave_scope();
        return;
      }
      case NK_SWITCH: {
        if (n->cond) {
          ExprInfo c = infer_expr(n->cond);
          if (c.valid && !is_switch_integer_type(c.type)) {
            emit(n, "switch quantity is not an integer");
          }
        }
        switch_stack_.push_back(SwitchCtx{});
        if (n->body) visit_stmt(n->body);
        switch_stack_.pop_back();
        return;
      }
      case NK_CASE: {
        if (switch_stack_.empty()) {
          emit(n, "case label not within a switch statement");
        } else if (n->left) {
          ExprInfo case_expr = infer_expr(n->left);
          if (case_expr.valid && !is_switch_integer_type(case_expr.type)) {
            emit(n, "case label does not have an integer type");
          }
          ConstEvalEnv case_env;
          populate_const_eval_env(case_env);
          if (auto r = evaluate_constant_expr(n->left, case_env); !r.ok()) {
            emit(n, "case label does not reduce to an integer constant");
          } else {
            auto [it, inserted] = switch_stack_.back().case_vals.insert(r.as_int());
            if (!inserted) emit(n, "duplicate case label in switch");
          }
        }
        if (n->body) visit_stmt(n->body);
        return;
      }
      case NK_DEFAULT: {
        if (switch_stack_.empty()) {
          emit(n, "default label not within a switch statement");
        } else if (switch_stack_.back().has_default) {
          emit(n, "duplicate default label in switch");
        } else {
          switch_stack_.back().has_default = true;
        }
        if (n->body) visit_stmt(n->body);
        return;
      }
      case NK_BREAK: {
        if (loop_depth_ <= 0 && switch_stack_.empty()) {
          emit(n, "break statement not within loop or switch");
        }
        return;
      }
      case NK_CONTINUE: {
        if (loop_depth_ <= 0) {
          emit(n, "continue statement not within loop");
        }
        return;
      }
      case NK_LABEL: {
        if (n->body) visit_stmt(n->body);
        return;
      }
      case NK_ASM: {
        if (n->left) (void)infer_expr(n->left);
        for (int i = 0; i < n->n_children; ++i) {
          if (n->children[i]) (void)infer_expr(n->children[i]);
        }
        // Output operands are written by the asm statement — mark initialized.
        for (int i = 0; i < n->asm_num_outputs && i < n->n_children; ++i) {
          mark_initialized_if_local_var(n->children[i]);
        }
        return;
      }
      case NK_ENUM_DEF: {
        bind_enum_constants_local(n);
        return;
      }
      case NK_GOTO:
      case NK_EMPTY:
        return;
      default:
        (void)infer_expr(n);
        return;
    }
  }

  ExprInfo infer_expr(const Node* n) {
    ExprInfo out;
    out.type = make_int_ts();

    if (!n) return out;

    switch (n->kind) {
      case NK_INT_LIT:
        out.valid = true;
        // GCC label-address (&&label) is parsed as NK_INT_LIT with name set;
        // its type is void* (C extension for computed goto targets).
        if (n->name && n->name[0]) {
          out.type.base = TB_VOID;
          out.type.ptr_level = 1;
        }
        return out;
      case NK_FLOAT_LIT:
        out.valid = true;
        out.type = classify_float_literal_type(const_cast<Node*>(n));
        return out;
      case NK_CHAR_LIT:
        out.valid = true;
        if (n->kind == NK_CHAR_LIT) out.type = make_int_ts();
        return out;
      case NK_STR_LIT: {
        out.valid = true;
        out.type.base = TB_CHAR;
        out.type.ptr_level = 1;
        out.type.array_rank = 0;
        out.type.array_size = -1;
        // Keep C89/C99 compatibility for plain string literal decay in this pass.
        out.type.is_const = false;
        return out;
      }
      case NK_VAR: {
        if (!n->name || !n->name[0]) return out;
        if (n->is_concept_id) {
          out.valid = true;
          out.type = make_int_ts();
          out.is_lvalue = false;
          out.is_const_lvalue = false;
          return out;
        }
        std::string qname = n->name;
        size_t scope_pos = qname.rfind("::");
        if (scope_pos != std::string::npos) {
          std::string struct_tag = qname.substr(0, scope_pos);
          if (auto mts = lookup_struct_static_member_type(struct_tag, n)) {
            out.valid = true;
            out.type = *mts;
            out.is_lvalue = true;
            out.is_const_lvalue = out.type.is_const &&
                                  out.type.ptr_level == 0 &&
                                  out.type.array_rank == 0;
            return out;
          }
          // If base chain has unresolved pending template types ($expr:),
          // accept the lookup optimistically — the HIR will resolve it.
          if (!static_member_lookup_has_structured_metadata(struct_tag, n) &&
              (complete_structs_.count(struct_tag) || complete_unions_.count(struct_tag))) {
            out.valid = true;
            out.type = make_int_ts();
            out.is_lvalue = true;
            return out;
          }
        }
        auto sym = lookup_symbol(n->name, n);
        if (!sym.has_value() && !n->is_global_qualified &&
            n->n_qualifier_segments == 0 && n->unqualified_name &&
            n->unqualified_name[0] &&
            std::string(n->unqualified_name) != n->name) {
          // Compatibility for producers that still render unqualified ids as a
          // visible namespace spelling before sema has bound locals.
          sym = lookup_symbol(n->unqualified_name, n);
        }
        if (!sym.has_value()) {
          // In an out-of-class method body, unqualified names may refer to
          // struct fields (implicit this->field).  Accept them here; the HIR
          // lowerer resolves them via MemberExpr.
          if (!current_method_struct_tag_.empty()) {
            if (has_struct_instance_field(n->unqualified_text_id, n->name)) {
              out.valid = true;
              out.is_lvalue = true;
              return out;
            }
          }
          emit(n, std::string("use of undeclared identifier '") + n->name + "'");
          return out;
        }
        out.valid = true;
        out.type = sym->type;
        if (out.type.is_lvalue_ref) out.type.is_lvalue_ref = false;
        out.is_lvalue = true;
        // is_const with ptr_level>0 means the pointee is const, not the pointer.
        // Only the pointer variable itself is a const lvalue if ptr_level==0.
        out.is_const_lvalue = out.type.is_const &&
                              out.type.ptr_level == 0 &&
                              out.type.array_rank == 0;
        // Mark function names so that &func doesn't add a spurious ptr_level.
        out.is_fn_name = lookup_function_by_name(n->name, n) != nullptr ||
                         lookup_cpp_overloads_by_name(n->name, n) != nullptr;
        return out;
      }
      case NK_ADDR: {
        bool old_suppress = suppress_uninit_read_;
        suppress_uninit_read_ = true;
        ExprInfo base = infer_expr(n->left);
        suppress_uninit_read_ = old_suppress;
        // Taking the address of a local variable means it can be written
        // through the pointer by any callee — mark it as initialized to
        // avoid false-positive "read of uninitialized variable" diagnostics.
        mark_initialized_if_local_var(n->left);
        out = base;
        out.valid = base.valid;
        out.is_lvalue = false;
        out.is_const_lvalue = false;
        out.is_fn_name = false;
        out.type = base.type;
        // In C, &function-name has the same type as function-name (both decay
        // to a function pointer).  Do not add an extra ptr_level for functions.
        if (!base.is_fn_name) {
          if (out.type.array_rank > 0 && !out.type.is_ptr_to_array) {
            // &array → pointer-to-array: preserve array dimension without decay.
            // e.g. char arr[28] → &arr has type char(*)[28], not char**.
            out.type.ptr_level += 1;
            out.type.is_ptr_to_array = true;
          } else {
            out.type = decay_array(out.type);
            out.type.ptr_level += 1;
          }
        }
        return out;
      }
      case NK_DEREF: {
        ExprInfo base = infer_expr(n->left);
        out.valid = base.valid;
        out.type = decay_array(base.type);
        out.is_lvalue = true;
        if (out.type.ptr_level > 0) {
          out.type.ptr_level -= 1;
          // After dereferencing a pointer-to-array we have a plain array, not
          // another pointer-to-array.
          out.type.is_ptr_to_array = false;
        }
        // is_const=true with ptr_level>0 means the *pointee* is const, not the
        // pointer itself.  After dereference, the lvalue is const only when the
        // resulting type is a plain (non-pointer) const-qualified object.
        out.is_const_lvalue = out.type.is_const && out.type.ptr_level == 0
                              && out.type.array_rank == 0;
        return out;
      }
      case NK_INDEX: {
        ExprInfo arr = infer_expr(n->left);
        (void)infer_expr(n->right);
        out.valid = arr.valid;
        out.type = arr.type;
        // decay_array now handles multi-dim arrays by reducing rank by one;
        // then decrement ptr_level to get the element type.
        out.type = decay_array(out.type);
        if (out.type.ptr_level > 0) {
          out.type.ptr_level -= 1;
          // After indexing a pointer-to-array, the result is a plain array
          // (not another pointer-to-array).
          out.type.is_ptr_to_array = false;
        }
        out.is_lvalue = true;
        // Only mark a const lvalue when the element itself is a non-pointer,
        // non-array const scalar.  For const T* arrays the pointer element is
        // not const (only the pointee is), so avoid the false positive.
        out.is_const_lvalue = out.type.is_const && out.type.ptr_level == 0
                              && out.type.array_rank == 0;
        return out;
      }
      case NK_MEMBER: {
        ExprInfo base = infer_expr(n->left);
        // We don't track struct field types, so mark valid=false to suppress
        // false-positive incompatible-assignment-type errors for member accesses.
        out.valid = false;
        out.is_lvalue = n->is_arrow || base.is_lvalue;
        out.type = make_int_ts();
        out.is_const_lvalue = out.is_lvalue && base.is_const_lvalue;
        return out;
      }
      case NK_ASSIGN:
      case NK_COMPOUND_ASSIGN: {
        bool old_suppress = suppress_uninit_read_;
        suppress_uninit_read_ = true;
        ExprInfo lhs = infer_expr(n->left);
        suppress_uninit_read_ = old_suppress;
        ExprInfo rhs = infer_expr(n->right);
        if (lhs.is_const_lvalue) {
          emit(n, "assignment to const-qualified lvalue");
        }
        // Note: assigning const T* to T* discards const but is GCC-warned, not
        // an error (C11 6.5.16.1p2); we only reject const writes (is_const_lvalue).
        // Only type-check simple '=' assignments; compound operators (+=, -=, etc.)
        // are in-place operations whose result type is always the lhs type.
        const bool is_simple_assign = n->op && n->op[0] == '=' && n->op[1] == '\0';
        if (is_simple_assign && lhs.valid && rhs.valid &&
            is_invalid_pointer_float_implicit_conversion(
                lhs.type, rhs.type, is_null_pointer_constant_expr(n->right))) {
          emit(n, "incompatible assignment type");
        }
        mark_initialized_if_local_var(n->left);
        out.valid = lhs.valid && rhs.valid;
        out.type = lhs.type;
        out.is_lvalue = lhs.is_lvalue;
        out.is_const_lvalue = lhs.is_const_lvalue;
        return out;
      }
      case NK_UNARY: {
        ExprInfo e = infer_expr(n->left);
        out = e;
        if (n->op &&
            (std::string(n->op).rfind("++", 0) == 0 || std::string(n->op).rfind("--", 0) == 0)) {
          if (e.is_const_lvalue) {
            emit(n, "increment/decrement of const-qualified object");
          }
        }
        out.is_lvalue = false;
        out.is_const_lvalue = false;
        return out;
      }
      case NK_POSTFIX: {
        ExprInfo e = infer_expr(n->left);
        out = e;
        if (n->op &&
            (std::string(n->op).rfind("++", 0) == 0 || std::string(n->op).rfind("--", 0) == 0)) {
          if (e.is_const_lvalue) {
            emit(n, "increment/decrement of const-qualified object");
          }
        }
        out.is_lvalue = false;
        out.is_const_lvalue = false;
        return out;
      }
      case NK_CALL:
      case NK_BUILTIN_CALL: {
        for (int i = 0; i < n->n_children; ++i) {
          (void)infer_expr(n->children[i]);
        }
        if (n->kind == NK_BUILTIN_CALL && n->builtin_id != BuiltinId::Unknown) {
          bool known = false;
          out.type = classify_known_builtin_return_type(n->builtin_id, &known);
          out.valid = known;
          return out;
        }
        if (n->left && n->left->kind == NK_VAR && n->left->name && n->left->name[0]) {
          // Check for ref-overloaded functions first.
          const std::vector<FunctionSig>* ref_overloads =
              lookup_ref_overloads_by_name(n->left->name, n->left);
          if (ref_overloads && !ref_overloads->empty()) {
            const auto& overloads = *ref_overloads;
            const int argc = n->n_children;
            // Infer arg value categories.
            std::vector<ExprInfo> arg_infos;
            for (int i = 0; i < argc; ++i)
              arg_infos.push_back(infer_expr(n->children[i]));
            // Find best viable overload.
            const FunctionSig* best = nullptr;
            int best_score = -1;
            for (const auto& sig : overloads) {
              const int required = static_cast<int>(sig.params.size());
              const int min_required = sig.has_param_pack ? required - 1 : required;
              if (!sig.variadic && !sig.has_param_pack && argc != required) continue;
              if ((sig.variadic || sig.has_param_pack) && argc < min_required) continue;
              bool viable = true;
              int score = 0;
              const int check_n = std::min(argc, required);
              for (int i = 0; i < check_n; ++i) {
                const auto& arg = arg_infos[static_cast<size_t>(i)];
                if (!arg.valid) continue;
                if (sig.params[static_cast<size_t>(i)].is_lvalue_ref && !arg.is_lvalue) { viable = false; break; }
                if (sig.params[static_cast<size_t>(i)].is_rvalue_ref && arg.is_lvalue) { viable = false; break; }
                // Exact ref-category match scores higher.
                if (sig.params[static_cast<size_t>(i)].is_rvalue_ref && !arg.is_lvalue) score += 2;
                else if (sig.params[static_cast<size_t>(i)].is_lvalue_ref && arg.is_lvalue) score += 2;
                else score += 1;
              }
              if (viable && score > best_score) { best = &sig; best_score = score; }
            }
            if (best) {
              out.valid = true;
              out.type = referred_type(best->ret);
              out.is_lvalue = best->ret.is_lvalue_ref;
              out.is_const_lvalue = out.is_lvalue &&
                                    out.type.is_const &&
                                    out.type.ptr_level == 0 &&
                                    out.type.array_rank == 0;
            } else {
              emit(n, "no viable overload for function call");
          }
          return out;
          }
          const std::vector<FunctionSig>* cpp_overloads =
              lookup_cpp_overloads_by_name(n->left->name, n->left);
          if (cpp_overloads && !cpp_overloads->empty()) {
            const auto& overloads = *cpp_overloads;
            const int argc = n->n_children;
            const FunctionSig* best = nullptr;
            for (const auto& sig : overloads) {
              const int required = static_cast<int>(sig.params.size());
              const int min_required = sig.has_param_pack ? required - 1 : required;
              if (!sig.unspecified_params &&
                  ((!sig.variadic && !sig.has_param_pack && argc != required) ||
                   ((sig.variadic || sig.has_param_pack) && argc < min_required))) {
                continue;
              }
              best = &sig;
              if (!sig.unspecified_params && argc == required) break;
            }
            if (best) {
              out.valid = true;
              out.type = referred_type(best->ret);
              out.is_lvalue = best->ret.is_lvalue_ref;
              out.is_const_lvalue = out.is_lvalue &&
                                    out.type.is_const &&
                                    out.type.ptr_level == 0 &&
                                    out.type.array_rank == 0;
            } else {
              emit(n, "no viable overload for function call");
            }
            return out;
          }
          const FunctionSig* fn = lookup_function_by_name(n->left->name, n->left);
          if (fn) {
            const FunctionSig& sig = *fn;
            const int argc = n->n_children;
            const int required = static_cast<int>(sig.params.size());
            const int min_required = sig.has_param_pack ? required - 1 : required;
            if (!sig.unspecified_params &&
                ((!sig.variadic && !sig.has_param_pack && argc != required) ||
                 ((sig.variadic || sig.has_param_pack) && argc < min_required))) {
              emit(n, "function call arity mismatch");
            }
            const int check_n = std::min(argc, required);
            for (int i = 0; i < check_n; ++i) {
              ExprInfo arg = infer_expr(n->children[i]);
              const bool dependent_ref_param =
                  sig.params[i].base == TB_TYPEDEF && sig.params[i].tag &&
                  (sig.params[i].is_lvalue_ref || sig.params[i].is_rvalue_ref);
              if (!dependent_ref_param &&
                  sig.params[i].is_lvalue_ref && arg.valid && !arg.is_lvalue) {
                emit(n, "function call argument must be an lvalue for reference parameter");
              }
              if (!dependent_ref_param &&
                  sig.params[i].is_rvalue_ref && arg.valid && arg.is_lvalue) {
                emit(n, "rvalue reference parameter cannot bind to an lvalue argument");
              }
              if (arg.valid &&
                  is_invalid_pointer_float_implicit_conversion(
                      referred_type(sig.params[i]), arg.type,
                      is_null_pointer_constant_expr(n->children[i]))) {
                emit(n, "function call argument type mismatch");
              }
            }
            out.valid = true;
            out.type = referred_type(sig.ret);
            out.is_lvalue = sig.ret.is_lvalue_ref;
            out.is_const_lvalue = out.is_lvalue &&
                                  out.type.is_const &&
                                  out.type.ptr_level == 0 &&
                                  out.type.array_rank == 0;
            return out;
          }
        }
        // Unknown function: don't infer a type (to avoid false-positive
        // incompatible-type errors on the result).
        out.valid = false;
        return out;
      }
      case NK_CAST: {
        (void)infer_expr(n->left);
        out.valid = true;
        out.type = referred_type(n->type);
        out.is_lvalue = n->type.is_lvalue_ref;
        out.is_const_lvalue = out.is_lvalue &&
                              out.type.is_const &&
                              out.type.ptr_level == 0 &&
                              out.type.array_rank == 0;
        if (n->type.base == TB_TYPEDEF) {
          const bool is_owner_qualified_cast_typedef =
              can_defer_owner_qualified_cast_typedef(n->type);
          // Suppress for template type parameters — they're resolved at instantiation.
          bool is_tpl_type_param = is_current_template_type_param_name(n->type.tag);
          // Dependent member typedefs in cast targets can currently preserve the
          // owner template parameter name even when the surrounding owner type
          // is otherwise concrete. Accept these decorated placeholder names so
          // generated dependent-owner cast matrices can validate through the
          // frontend without masking ordinary unknown bare type names.
          if (!is_tpl_type_param &&
              n->type.tag &&
              is_known_template_type_param_name(n->type.tag) &&
              (n->type.ptr_level > 0 || n->type.is_fn_ptr ||
               n->type.is_lvalue_ref || n->type.is_rvalue_ref)) {
            is_tpl_type_param = true;
          }
          if (!is_tpl_type_param &&
              n->type.tag &&
              looks_like_template_placeholder_name(n->type.tag) &&
              (n->type.ptr_level > 0 || n->type.is_fn_ptr ||
               n->type.is_lvalue_ref || n->type.is_rvalue_ref)) {
            is_tpl_type_param = true;
          }
          if (!is_tpl_type_param && !is_owner_qualified_cast_typedef) {
            const std::string tname = n->type.tag ? n->type.tag : "<anonymous>";
            emit(n, "cast to unknown type name '" + tname + "'");
          }
        }
        if (!is_complete_object_type(n->type)) {
          emit(n, "cast to incomplete struct/union object type");
        }
        // Note: explicit C casts that discard const are permitted by the
        // standard (C11 6.3.2.3); GCC emits a -Wcast-qual warning but not
        // an error.  Do not reject them here.
        return out;
      }
      case NK_TERNARY: {
        (void)infer_expr(n->cond);
        ExprInfo t = infer_expr(n->then_);
        ExprInfo e = infer_expr(n->else_);
        // C11 6.5.15: if one branch is a pointer and the other is integer/null,
        // the result type is the pointer type.  Strip is_const to avoid false
        // drops_const warnings (null-pointer-constant branches are common).
        if (t.type.ptr_level > 0 && e.type.ptr_level == 0) {
          out = t;
          out.type.is_const = false;
        } else if (e.type.ptr_level > 0 && t.type.ptr_level == 0) {
          out = e;
          out.type.is_const = false;
        } else {
          out = t.valid ? t : e;
        }
        out.is_lvalue = false;
        out.is_const_lvalue = false;
        return out;
      }
      case NK_BINOP: {
        ExprInfo l = infer_expr(n->left);
        ExprInfo r = infer_expr(n->right);
        // If either operand has unknown type (e.g., struct member), the result
        // type is also unknown; propagate valid=false to suppress false positives.
        if (!l.valid || !r.valid) { out.valid = false; return out; }
        out.valid = true;
        // Pointer arithmetic: if one operand is a pointer/array and op is + or -,
        // the result has the pointer type so subsequent assignment checks don't fire.
        if (n->op && n->op[1] == '\0' &&
            (n->op[0] == '+' || n->op[0] == '-')) {
          TypeSpec lt = decay_array(l.type);
          TypeSpec rt = decay_array(r.type);
          if (lt.ptr_level > 0 && rt.ptr_level == 0) {
            out.type = lt;
          } else if (rt.ptr_level > 0 && lt.ptr_level == 0) {
            out.type = rt;
          }
          // ptr - ptr yields ptrdiff_t; keep make_int_ts() default.
        }
        // Propagate operand type for arithmetic/bitwise/shift ops so that
        // assignment checks don't falsely fire (e.g., vector types).
        // Pointer arithmetic cases already set out.type above; don't re-override.
        // Use l.type if it's a richer type than the default int.
        if (out.type.ptr_level == 0 && l.type.ptr_level == 0 &&
            (l.type.array_rank > 0 || l.type.is_vector)) {
          out.type = l.type;
        }
        return out;
      }
      case NK_COMMA_EXPR: {
        (void)infer_expr(n->left);
        ExprInfo r = infer_expr(n->right);
        out = r;
        out.is_lvalue = false;
        out.is_const_lvalue = false;
        return out;
      }
      case NK_SIZEOF_EXPR:
        {
          bool old_suppress = suppress_uninit_read_;
          suppress_uninit_read_ = true;
          ExprInfo e = infer_expr(n->left);
          suppress_uninit_read_ = old_suppress;
          if (e.valid && !is_complete_object_type(e.type)) {
            emit(n, "invalid application of sizeof to incomplete type");
          }
        }
        out.valid = true;
        return out;
      case NK_SIZEOF_PACK:
        out.valid = true;
        return out;
      case NK_SIZEOF_TYPE:
        if (!is_complete_object_type(n->type)) {
          emit(n, "invalid application of sizeof to incomplete type");
        }
        out.valid = true;
        return out;
      case NK_ALIGNOF_EXPR:
        {
          bool old_suppress = suppress_uninit_read_;
          suppress_uninit_read_ = true;
          infer_expr(n->left);
          suppress_uninit_read_ = old_suppress;
        }
        out.valid = true;
        return out;
      case NK_ALIGNOF_TYPE:
        if (!is_complete_object_type(n->type)) {
          emit(n, "invalid application of _Alignof to incomplete type");
        }
        out.valid = true;
        return out;
      case NK_INIT_LIST:
        // Aggregate initializer list: recurse into children but return valid=false
        // so that implicit_convertible checks on the destination type are skipped
        // (the initializer list itself carries no single comparable type).
        for (int i = 0; i < n->n_children; ++i) (void)infer_expr(n->children[i]);
        out.valid = false;
        return out;
      case NK_COMPOUND_LIT:
        for (int i = 0; i < n->n_children; ++i) (void)infer_expr(n->children[i]);
        out.valid = true;
        out.type = n->type;  // compound literal has an explicit type
        return out;
      default:
        return out;
    }
  }
};

}  // namespace

ValidateResult validate_program(const Node* root) {
  Validator v;
  return v.run(root);
}

void print_diagnostics(const std::vector<Diagnostic>& diags, const std::string& file) {
  for (const auto& d : diags) {
    const char* diag_file = (d.file && d.file[0]) ? d.file : file.c_str();
    const int diag_column = d.column > 0 ? d.column : 1;
    std::cerr << diag_file << ":" << d.line << ":" << diag_column
              << ": error: " << d.message << "\n";
  }
}

}  // namespace c4c::sema
