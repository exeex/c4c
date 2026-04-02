#include "ast_to_hir.hpp"
#include "consteval.hpp"
#include "hir_lowerer_internal.hpp"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstring>
#include <functional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace c4c::hir {

SourceSpan make_span(const Node* n) {
  if (!n) {
    return {};
  }
  SourceSpan s{};
  s.begin.line = n->line;
  s.end.line = n->line;
  return s;
}

/// Build a NamespaceQualifier from an AST node's structured qualifier fields.
NamespaceQualifier make_ns_qual(const Node* n) {
  NamespaceQualifier q;
  if (!n) return q;
  q.is_global_qualified = n->is_global_qualified;
  q.context_id = n->namespace_context_id;
  for (int i = 0; i < n->n_qualifier_segments; ++i) {
    if (n->qualifier_segments[i])
      q.segments.emplace_back(n->qualifier_segments[i]);
  }
  return q;
}

std::string decode_string_node(const Node* n) {
  if (!n || n->kind != NK_STR_LIT || !n->sval) return {};
  std::string raw = n->sval;
  if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"') {
    raw = raw.substr(1, raw.size() - 2);
  } else if (raw.size() >= 3 && raw[0] == 'L' && raw[1] == '"' && raw.back() == '"') {
    raw = raw.substr(2, raw.size() - 3);
  }
  std::string out;
  for (size_t i = 0; i < raw.size(); ++i) {
    const unsigned char ch = static_cast<unsigned char>(raw[i]);
    if (ch != '\\' || i + 1 >= raw.size()) {
      out.push_back(static_cast<char>(ch));
      continue;
    }
    const char esc = raw[++i];
    switch (esc) {
      case 'n': out.push_back('\n'); break;
      case 'r': out.push_back('\r'); break;
      case 't': out.push_back('\t'); break;
      case '\\': out.push_back('\\'); break;
      case '\'': out.push_back('\''); break;
      case '"': out.push_back('"'); break;
      case '0': out.push_back('\0'); break;
      default: out.push_back(esc); break;
    }
  }
  return out;
}

std::string strip_quoted_string(const char* raw) {
  if (!raw) return {};
  std::string s = raw;
  if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
    s = s.substr(1, s.size() - 2);
  }
  return s;
}

std::string rewrite_gcc_asm_template(std::string text) {
  std::string out;
  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '%') {
      out.push_back(text[i]);
      continue;
    }
    if (i + 1 < text.size() && text[i + 1] == '%') {
      out += "%%";
      ++i;
      continue;
    }
    size_t j = i + 1;
    std::string modifier;
    if (j < text.size() && std::isalpha(static_cast<unsigned char>(text[j]))) {
      modifier.push_back(text[j]);
      ++j;
    }
    size_t num_begin = j;
    while (j < text.size() && std::isdigit(static_cast<unsigned char>(text[j]))) ++j;
    if (j == num_begin) {
      out.push_back('%');
      continue;
    }
    out += "${";
    out += text.substr(num_begin, j - num_begin);
    if (!modifier.empty()) {
      out += ":";
      out += modifier;
    }
    out += "}";
    i = j - 1;
  }
  return out;
}

std::string sanitize_symbol(std::string s) {
  for (char& ch : s) {
    const bool ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9') || ch == '_';
    if (!ok) ch = '_';
  }
  if (s.empty()) s = "anon";
  return s;
}

std::optional<QualifiedMethodRef> try_parse_qualified_struct_method_name(
    const Node* fn, bool include_const_suffix) {
  if (!fn || fn->kind != NK_FUNCTION || !fn->name) return std::nullopt;
  const std::string name(fn->name);
  const size_t sep = name.rfind("::");
  if (sep == std::string::npos || sep == 0 || sep + 2 >= name.size())
    return std::nullopt;

  QualifiedMethodRef ref;
  ref.struct_tag = name.substr(0, sep);
  ref.method_name = name.substr(sep + 2);
  ref.key = ref.struct_tag + "::" + ref.method_name;
  if (include_const_suffix && fn->is_const_method) ref.key += "_const";
  return ref;
}

UnaryOp map_unary_op(const char* op) {
  if (!op) return UnaryOp::Plus;
  const std::string s(op);
  if (s.rfind("++", 0) == 0) return UnaryOp::PreInc;
  if (s.rfind("--", 0) == 0) return UnaryOp::PreDec;
  if (s == "+") return UnaryOp::Plus;
  if (s == "-") return UnaryOp::Minus;
  if (s == "!") return UnaryOp::Not;
  if (s == "~") return UnaryOp::BitNot;
  return UnaryOp::Plus;
}

BinaryOp map_binary_op(const char* op) {
  if (!op) return BinaryOp::Add;
  const std::string s(op);
  if (s == "+") return BinaryOp::Add;
  if (s == "-") return BinaryOp::Sub;
  if (s == "*") return BinaryOp::Mul;
  if (s == "/") return BinaryOp::Div;
  if (s == "%") return BinaryOp::Mod;
  if (s == "<<") return BinaryOp::Shl;
  if (s == ">>") return BinaryOp::Shr;
  if (s == "&") return BinaryOp::BitAnd;
  if (s == "|") return BinaryOp::BitOr;
  if (s == "^") return BinaryOp::BitXor;
  if (s == "<") return BinaryOp::Lt;
  if (s == "<=") return BinaryOp::Le;
  if (s == ">") return BinaryOp::Gt;
  if (s == ">=") return BinaryOp::Ge;
  if (s == "==") return BinaryOp::Eq;
  if (s == "!=") return BinaryOp::Ne;
  if (s == "&&") return BinaryOp::LAnd;
  if (s == "||") return BinaryOp::LOr;
  if (s == ",") return BinaryOp::Comma;
  return BinaryOp::Add;
}

AssignOp map_assign_op(const char* op, NodeKind kind) {
  if (!op) return AssignOp::Set;
  const std::string s(op);
  if (s == "=") return AssignOp::Set;
  if (s == "+=" || s == "+") return AssignOp::Add;
  if (s == "-=" || s == "-") return AssignOp::Sub;
  if (s == "*=" || s == "*") return AssignOp::Mul;
  if (s == "/=" || s == "/") return AssignOp::Div;
  if (s == "%=" || s == "%") return AssignOp::Mod;
  if (s == "<<=" || s == "<<") return AssignOp::Shl;
  if (s == ">>=" || s == ">>") return AssignOp::Shr;
  if (s == "&=" || s == "&") return AssignOp::BitAnd;
  if (s == "|=" || s == "|") return AssignOp::BitOr;
  if (s == "^=" || s == "^") return AssignOp::BitXor;
  if (kind == NK_ASSIGN) return AssignOp::Set;
  return AssignOp::Set;
}

TypeSpec infer_int_literal_type(const Node* n) {
  TypeSpec ts = n ? n->type : TypeSpec{};
  if (ts.base == TB_VOID && ts.ptr_level == 0 && ts.array_rank == 0 && !ts.is_fn_ptr)
    ts.base = TB_INT;
  ts.array_rank = 0;
  ts.array_size = -1;
  if (!n || !n->sval) return ts;

  const char* sv = n->sval;
  const size_t len = std::strlen(sv);
  int lcount = 0;
  bool has_u = false;
  for (int i = static_cast<int>(len) - 1; i >= 0; --i) {
    const char c = sv[i];
    if (c == 'l' || c == 'L') {
      ++lcount;
    } else if (c == 'u' || c == 'U') {
      has_u = true;
    } else if (c == 'i' || c == 'I' || c == 'j' || c == 'J') {
      // GNU imaginary suffix is ignored here for integral rank.
    } else {
      break;
    }
  }

  if (lcount >= 2) {
    ts.base = has_u ? TB_ULONGLONG : TB_LONGLONG;
    return ts;
  }
  if (lcount == 1) {
    ts.base = has_u ? TB_ULONG : TB_LONG;
    return ts;
  }
  if (has_u) { ts.base = TB_UINT; return ts; }

  // For unsuffixed hex/octal literals the C standard allows unsigned types
  // when the value doesn't fit in the signed type (C11 6.4.4.1p5).
  // Decimal literals never get an unsigned type without an explicit U suffix.
  bool is_hex_or_octal = (sv[0] == '0' && len > 1 &&
                          (sv[1] == 'x' || sv[1] == 'X' ||
                           (sv[1] >= '0' && sv[1] <= '9')));
  if (is_hex_or_octal && n && ts.base == TB_INT) {
    unsigned long long uval = static_cast<unsigned long long>(n->ival);
    // int → unsigned int → long → unsigned long → long long → unsigned long long
    if (uval > 0x7FFFFFFFull && uval <= 0xFFFFFFFFull) {
      ts.base = TB_UINT;
    } else if (uval > 0xFFFFFFFFull && uval <= 0x7FFFFFFFFFFFFFFFull) {
      ts.base = TB_LONGLONG;
    } else if (uval > 0x7FFFFFFFFFFFFFFFull) {
      ts.base = TB_ULONGLONG;
    }
  }
  return ts;
}

TypeSpec normalize_generic_type(TypeSpec ts) {
  if (!is_vector_ty(ts) && ts.array_rank > 0 && !ts.is_ptr_to_array) {
    ts.array_rank = 0;
    ts.array_size = -1;
    ts.ptr_level += 1;
  }
  // Generic selection uses lvalue conversions on the controlling expression.
  // Bare function designators decay to function pointers.
  if (ts.is_fn_ptr && ts.ptr_level == 0) {
    ts.ptr_level = 1;
  }
  // Strip only top-level qualifiers (scalar rvalues).  In this codebase,
  // ptr_level>0 qualifiers model pointee qualifiers and must be preserved.
  if (ts.ptr_level == 0 && ts.array_rank == 0 && !is_vector_ty(ts)) {
    ts.is_const = false;
    ts.is_volatile = false;
  }
  return ts;
}

bool generic_type_compatible(TypeSpec a, TypeSpec b) {
  a = normalize_generic_type(a);
  b = normalize_generic_type(b);
  if (a.base != b.base) return false;
  if (a.is_vector != b.is_vector) return false;
  if (a.is_vector &&
      (a.vector_lanes != b.vector_lanes || a.vector_bytes != b.vector_bytes))
    return false;
  if (a.ptr_level != b.ptr_level) return false;
  if (a.is_const != b.is_const || a.is_volatile != b.is_volatile) return false;
  if (a.array_rank != b.array_rank) return false;
  if (a.is_fn_ptr != b.is_fn_ptr) return false;
  if (a.base == TB_STRUCT || a.base == TB_UNION || a.base == TB_ENUM) {
    const char* atag = a.tag ? a.tag : "";
    const char* btag = b.tag ? b.tag : "";
    return std::strcmp(atag, btag) == 0;
  }
  return true;
}

bool has_concrete_type(const TypeSpec& ts) {
  return ts.base != TB_VOID || ts.ptr_level > 0 || ts.array_rank > 0 ||
         ts.is_fn_ptr || is_vector_ty(ts);
}

int align_to(int value, int align) {
  if (align <= 1) return value;
  return (value + align - 1) / align * align;
}

namespace {

class LayoutQueries {
 public:
  explicit LayoutQueries(const hir::Module& module) : module_(module) {}

  int struct_size_bytes(const char* tag) const {
    if (!tag || !tag[0]) return 4;
    const auto it = module_.struct_defs.find(tag);
    if (it == module_.struct_defs.end()) return 4;
    return it->second.size_bytes;
  }

  int struct_align_bytes(const char* tag) const {
    if (!tag || !tag[0]) return 4;
    const auto it = module_.struct_defs.find(tag);
    if (it == module_.struct_defs.end()) return 4;
    return std::max(1, it->second.align_bytes);
  }

  int type_size_bytes(const TypeSpec& ts) const {
    if (ts.array_rank > 0) {
      if (ts.array_size == 0) return 0;
      if (ts.array_size > 0) {
        const TypeSpec elem = array_element_type(ts);
        return static_cast<int>(ts.array_size) * type_size_bytes(elem);
      }
      return 8;
    }
    if (ts.is_vector && ts.vector_bytes > 0) return static_cast<int>(ts.vector_bytes);
    if (ts.ptr_level > 0 && ts.is_ptr_to_array) return 8;
    if (ts.ptr_level > 0 || ts.is_fn_ptr) return 8;
    if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0) {
      return struct_size_bytes(ts.tag);
    }
    return sizeof_base(ts.base);
  }

  int type_align_bytes(const TypeSpec& ts) const {
    int natural = 1;
    if (ts.array_rank > 0) {
      natural = type_align_bytes(array_element_type(ts));
    } else if (ts.is_vector && ts.vector_bytes > 0) {
      natural = static_cast<int>(ts.vector_bytes);
    } else if (ts.ptr_level > 0 || ts.is_fn_ptr) {
      natural = 8;
    } else if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0) {
      natural = struct_align_bytes(ts.tag);
    } else {
      natural = std::max(1, static_cast<int>(align_base(ts.base, ts.ptr_level)));
    }
    if (ts.align_bytes > 0) natural = std::max(natural, ts.align_bytes);
    return natural;
  }

  int field_size_bytes(const HirStructField& field) const {
    if (field.size_bytes > 0 || field.is_flexible_array) return field.size_bytes;
    return type_size_bytes(field.elem_type);
  }

  int field_align_bytes(const HirStructField& field) const {
    if (field.align_bytes > 0) return field.align_bytes;
    return type_align_bytes(field.elem_type);
  }

 private:
  static TypeSpec array_element_type(const TypeSpec& ts) {
    TypeSpec elem = ts;
    elem.array_rank--;
    if (elem.array_rank > 0) {
      for (int i = 0; i < elem.array_rank; ++i) elem.array_dims[i] = elem.array_dims[i + 1];
    }
    elem.array_size = (elem.array_rank > 0) ? elem.array_dims[0] : -1;
    return elem;
  }

  const hir::Module& module_;
};

bool has_cpp_empty_object_size(const hir::Module& module) {
  return module.source_profile == SourceProfile::CppSubset ||
         module.source_profile == SourceProfile::C4;
}

void compute_empty_record_layout(const hir::Module& module, HirStructDef& def) {
  def.align_bytes = std::max(1, def.struct_align);
  if (!has_cpp_empty_object_size(module)) {
    def.size_bytes = 0;
    return;
  }
  def.size_bytes = std::max(1, def.align_bytes);
}

TypeSpec field_layout_type(const HirStructField& field) {
  TypeSpec field_ts = field.elem_type;
  if (field.array_first_dim >= 0) {
    for (int i = std::min(field_ts.array_rank, 7); i > 0; --i) {
      field_ts.array_dims[i] = field_ts.array_dims[i - 1];
    }
    field_ts.array_rank = std::min(field_ts.array_rank + 1, 8);
    field_ts.array_size = field.array_first_dim;
    field_ts.array_dims[0] = field.array_first_dim;
  }
  return field_ts;
}

void prepare_field_layout(const hir::Module& module, HirStructField& field, int pack) {
  const LayoutQueries queries(module);
  if (field.bit_width >= 0) {
    field.align_bytes = std::max(1, field.storage_unit_bits / 8);
    field.size_bytes = std::max(1, field.storage_unit_bits / 8);
  } else {
    const TypeSpec field_ts = field_layout_type(field);
    field.align_bytes = queries.type_align_bytes(field_ts);
    field.size_bytes = queries.type_size_bytes(field_ts);
  }
  if (pack > 0 && field.align_bytes > pack) field.align_bytes = pack;
}

void apply_struct_align(HirStructDef& def) {
  if (def.struct_align > 0)
    def.align_bytes = std::max(def.align_bytes, def.struct_align);
}

void compute_union_layout(const hir::Module& module, HirStructDef& def, int pack) {
  const LayoutQueries queries(module);
  def.align_bytes = 1;
  def.size_bytes = 0;
  for (auto& field : def.fields) {
    field.offset_bytes = 0;
    int field_align = queries.field_align_bytes(field);
    if (pack > 0 && field_align > pack) field_align = pack;
    def.align_bytes = std::max(def.align_bytes, field_align);
    def.size_bytes = std::max(def.size_bytes, queries.field_size_bytes(field));
  }
  apply_struct_align(def);
  def.size_bytes = align_to(def.size_bytes, def.align_bytes);
}

void compute_record_layout(const hir::Module& module, HirStructDef& def, int pack) {
  const LayoutQueries queries(module);
  def.align_bytes = 1;
  int offset = 0;
  int last_llvm_idx = -1;
  int last_offset = 0;
  for (auto& field : def.fields) {
    if (field.llvm_idx != last_llvm_idx) {
      int field_align = queries.field_align_bytes(field);
      if (pack > 0 && field_align > pack) field_align = pack;
      offset = align_to(offset, field_align);
      last_offset = offset;
      offset += queries.field_size_bytes(field);
      last_llvm_idx = field.llvm_idx;
      def.align_bytes = std::max(def.align_bytes, field_align);
    }
    field.offset_bytes = last_offset;
  }
  apply_struct_align(def);
  def.size_bytes = align_to(offset, def.align_bytes);
}

}  // namespace

void compute_struct_layout(hir::Module* module, HirStructDef& def) {
  if (!module) return;

  const int pack = def.pack_align;  // 0 = default, >0 = cap alignment

  if (def.fields.empty()) {
    compute_empty_record_layout(*module, def);
    return;
  }

  for (auto& field : def.fields) prepare_field_layout(*module, field, pack);

  if (def.is_union) {
    compute_union_layout(*module, def, pack);
    return;
  }

  compute_record_layout(*module, def, pack);
}

bool Lowerer::is_referenced_without_template_args(
    const char* fn_name, const std::vector<const Node*>& items) {
  if (!fn_name) return false;
  for (const Node* item : items) {
    if (item->kind != NK_FUNCTION || !item->body) continue;
    if (item->n_template_params > 0) continue;  // Skip template function bodies
    if (has_plain_call(item->body, fn_name)) return true;
  }
  return false;
}

bool Lowerer::has_plain_call(const Node* n, const char* fn_name) {
  if (!n) return false;
  if (n->kind == NK_CALL && n->left && n->left->kind == NK_VAR &&
      n->left->name && strcmp(n->left->name, fn_name) == 0 &&
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


TypeBindings Lowerer::build_call_bindings(const Node* call_var, const Node* fn_def,
                                          const TypeBindings* enclosing_bindings) {
  TypeBindings bindings;
  if (!call_var || !fn_def || fn_def->n_template_params <= 0) return bindings;
  const int total_args = call_var->n_template_args > 0 ? call_var->n_template_args : 0;
  int arg_index = 0;
  for (int i = 0; i < fn_def->n_template_params; ++i) {
    if (!fn_def->template_param_names[i]) continue;
    const bool is_nttp =
        fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i];
    const bool is_pack =
        fn_def->template_param_is_pack && fn_def->template_param_is_pack[i];
    if (is_nttp) {
      if (is_pack) arg_index = total_args;
      else if (arg_index < total_args) ++arg_index;
      continue;
    }
    if (is_pack) {
      for (int pack_index = 0; arg_index < total_args; ++arg_index, ++pack_index) {
        TypeSpec arg_ts = call_var->template_arg_types[arg_index];
        if (arg_ts.base == TB_TYPEDEF && arg_ts.tag && enclosing_bindings) {
          auto resolved = enclosing_bindings->find(arg_ts.tag);
          if (resolved != enclosing_bindings->end()) arg_ts = resolved->second;
        }
        bindings[pack_binding_name(fn_def->template_param_names[i], pack_index)] = arg_ts;
      }
      continue;
    }
    if (arg_index >= total_args) continue;
    TypeSpec arg_ts = call_var->template_arg_types[arg_index++];
    if (arg_ts.base == TB_TYPEDEF && arg_ts.tag && enclosing_bindings) {
      auto resolved = enclosing_bindings->find(arg_ts.tag);
      if (resolved != enclosing_bindings->end()) arg_ts = resolved->second;
    }
    bindings[fn_def->template_param_names[i]] = arg_ts;
  }
  if (fn_def->template_param_has_default) {
    for (int i = 0; i < fn_def->n_template_params; ++i) {
      if (!fn_def->template_param_names[i]) continue;
      if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) continue;
      if (fn_def->template_param_is_pack && fn_def->template_param_is_pack[i]) continue;
      if (bindings.count(fn_def->template_param_names[i])) continue;
      if (fn_def->template_param_has_default[i]) {
        bindings[fn_def->template_param_names[i]] = fn_def->template_param_default_types[i];
      }
    }
  }
  return bindings;
}

NttpBindings Lowerer::build_call_nttp_bindings(const Node* call_var, const Node* fn_def,
                                               const NttpBindings* enclosing_nttp) {
  NttpBindings bindings;
  if (!call_var || !fn_def || fn_def->n_template_params <= 0) return bindings;
  const int total_args = call_var->n_template_args > 0 ? call_var->n_template_args : 0;
  int arg_index = 0;
  for (int i = 0; i < fn_def->n_template_params; ++i) {
    if (!fn_def->template_param_names[i]) continue;
    const bool is_nttp =
        fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i];
    const bool is_pack =
        fn_def->template_param_is_pack && fn_def->template_param_is_pack[i];
    if (!is_nttp) {
      if (is_pack) arg_index = total_args;
      else if (arg_index < total_args) ++arg_index;
      continue;
    }
    if (is_pack) {
      for (int pack_index = 0; arg_index < total_args; ++arg_index, ++pack_index) {
        if (!(call_var->template_arg_is_value && call_var->template_arg_is_value[arg_index])) {
          continue;
        }
        const std::string key = pack_binding_name(fn_def->template_param_names[i], pack_index);
        if (call_var->template_arg_nttp_names && call_var->template_arg_nttp_names[arg_index]) {
          if (enclosing_nttp) {
            auto it = enclosing_nttp->find(call_var->template_arg_nttp_names[arg_index]);
            if (it != enclosing_nttp->end()) bindings[key] = it->second;
          }
          continue;
        }
        bindings[key] = call_var->template_arg_values[arg_index];
      }
      continue;
    }
    if (arg_index >= total_args) continue;
    if (call_var->template_arg_is_value && call_var->template_arg_is_value[arg_index]) {
      if (call_var->template_arg_nttp_names && call_var->template_arg_nttp_names[arg_index]) {
        if (enclosing_nttp) {
          auto it = enclosing_nttp->find(call_var->template_arg_nttp_names[arg_index]);
          if (it != enclosing_nttp->end()) {
            bindings[fn_def->template_param_names[i]] = it->second;
            ++arg_index;
            continue;
          }
        }
        ++arg_index;
        continue;
      }
      bindings[fn_def->template_param_names[i]] = call_var->template_arg_values[arg_index];
    }
    ++arg_index;
  }
  if (fn_def->template_param_has_default) {
    for (int i = 0; i < fn_def->n_template_params; ++i) {
      if (!fn_def->template_param_names[i]) continue;
      if (!fn_def->template_param_is_nttp || !fn_def->template_param_is_nttp[i]) continue;
      if (fn_def->template_param_is_pack && fn_def->template_param_is_pack[i]) continue;
      if (bindings.count(fn_def->template_param_names[i])) continue;
      if (fn_def->template_param_has_default[i]) {
        bindings[fn_def->template_param_names[i]] = fn_def->template_param_default_values[i];
      }
    }
  }
  return bindings;
}

bool Lowerer::has_forwarded_nttp(const Node* call_var) {
  if (!call_var || !call_var->template_arg_nttp_names) return false;
  for (int i = 0; i < call_var->n_template_args; ++i) {
    if (call_var->template_arg_nttp_names[i]) return true;
  }
  return false;
}

std::optional<TypeSpec> Lowerer::try_infer_arg_type_for_deduction(
    const Node* expr, const Node* enclosing_fn) {
  if (!expr) return std::nullopt;

  if (has_concrete_type(expr->type)) return expr->type;

  switch (expr->kind) {
    case NK_INT_LIT:
      return infer_int_literal_type(expr);
    case NK_FLOAT_LIT: {
      TypeSpec ts = expr->type;
      if (!has_concrete_type(ts))
        ts = classify_float_literal_type(const_cast<Node*>(expr));
      return ts;
    }
    case NK_CHAR_LIT: {
      TypeSpec ts = expr->type;
      if (!has_concrete_type(ts)) ts.base = TB_INT;
      return ts;
    }
    case NK_STR_LIT: {
      TypeSpec ts{};
      ts.base = TB_CHAR;
      ts.ptr_level = 1;
      return ts;
    }
    case NK_VAR: {
      if (!expr->name || !enclosing_fn) return std::nullopt;
      for (int i = 0; i < enclosing_fn->n_params; ++i) {
        const Node* p = enclosing_fn->params[i];
        if (p && p->name && std::strcmp(p->name, expr->name) == 0 &&
            has_concrete_type(p->type)) {
          return p->type;
        }
      }
      if (enclosing_fn->body) {
        const Node* body = enclosing_fn->body;
        for (int i = 0; i < body->n_children; ++i) {
          const Node* stmt = body->children[i];
          if (stmt && stmt->kind == NK_DECL && stmt->name &&
              std::strcmp(stmt->name, expr->name) == 0 &&
              has_concrete_type(stmt->type)) {
            return stmt->type;
          }
        }
      }
      return std::nullopt;
    }
    case NK_CAST:
      if (has_concrete_type(expr->type)) return expr->type;
      return std::nullopt;
    case NK_ADDR:
      if (expr->left) {
        auto inner = try_infer_arg_type_for_deduction(expr->left, enclosing_fn);
        if (inner) {
          inner->ptr_level++;
          return inner;
        }
      }
      return std::nullopt;
    case NK_DEREF:
      if (expr->left) {
        auto inner = try_infer_arg_type_for_deduction(expr->left, enclosing_fn);
        if (inner && inner->ptr_level > 0) {
          inner->ptr_level--;
          return inner;
        }
      }
      return std::nullopt;
    case NK_UNARY:
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

TypeBindings Lowerer::try_deduce_template_type_args(
    const Node* call_node, const Node* fn_def, const Node* enclosing_fn) {
  TypeBindings deduced;
  if (!call_node || !fn_def || fn_def->n_template_params <= 0) return deduced;

  std::unordered_set<std::string> type_param_names;
  for (int i = 0; i < fn_def->n_template_params; ++i) {
    if (fn_def->template_param_names[i] &&
        !(fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i])) {
      type_param_names.insert(fn_def->template_param_names[i]);
    }
  }
  if (type_param_names.empty()) return deduced;

  const int n_args = call_node->n_children;
  const int n_params = fn_def->n_params;
  const int match_count = std::min(n_args, n_params);

  for (int i = 0; i < match_count; ++i) {
    const Node* param = fn_def->params[i];
    const Node* arg = call_node->children[i];
    if (!param || !arg) continue;

    const TypeSpec& param_ts = param->type;

    if (param_ts.base == TB_TYPEDEF && param_ts.tag &&
        type_param_names.count(param_ts.tag) &&
        param_ts.ptr_level == 0 && param_ts.array_rank == 0 &&
        param_ts.is_rvalue_ref) {
      auto arg_type = try_infer_arg_type_for_deduction(arg, enclosing_fn);
      if (!arg_type) continue;
      TypeSpec deduced_ts = *arg_type;
      if (is_ast_lvalue(arg)) {
        deduced_ts.is_lvalue_ref = true;
        deduced_ts.is_rvalue_ref = false;
      }
      auto existing = deduced.find(param_ts.tag);
      if (existing != deduced.end()) {
        if (existing->second.base != deduced_ts.base ||
            existing->second.ptr_level != deduced_ts.ptr_level ||
            existing->second.tag != deduced_ts.tag ||
            existing->second.is_lvalue_ref != deduced_ts.is_lvalue_ref ||
            existing->second.is_rvalue_ref != deduced_ts.is_rvalue_ref) {
          return {};
        }
      } else {
        deduced[param_ts.tag] = deduced_ts;
      }
    } else if (param_ts.base == TB_TYPEDEF && param_ts.tag &&
               type_param_names.count(param_ts.tag) &&
               param_ts.ptr_level == 0 && param_ts.array_rank == 0) {
      auto arg_type = try_infer_arg_type_for_deduction(arg, enclosing_fn);
      if (!arg_type) continue;
      TypeSpec deduced_ts = *arg_type;
      auto existing = deduced.find(param_ts.tag);
      if (existing != deduced.end()) {
        if (existing->second.base != deduced_ts.base ||
            existing->second.ptr_level != deduced_ts.ptr_level ||
            existing->second.tag != deduced_ts.tag) {
          return {};
        }
      } else {
        deduced[param_ts.tag] = deduced_ts;
      }
    } else if (param_ts.base == TB_TYPEDEF && param_ts.tag &&
               type_param_names.count(param_ts.tag) &&
               param_ts.ptr_level > 0 && param_ts.array_rank == 0) {
      auto arg_type = try_infer_arg_type_for_deduction(arg, enclosing_fn);
      if (!arg_type || arg_type->ptr_level < param_ts.ptr_level) continue;
      TypeSpec deduced_ts = *arg_type;
      deduced_ts.ptr_level -= param_ts.ptr_level;
      auto existing = deduced.find(param_ts.tag);
      if (existing != deduced.end()) {
        if (existing->second.base != deduced_ts.base ||
            existing->second.ptr_level != deduced_ts.ptr_level) {
          return {};
        }
      } else {
        deduced[param_ts.tag] = deduced_ts;
      }
    }
  }

  return deduced;
}

bool Lowerer::deduction_covers_all_type_params(const TypeBindings& deduced,
                                               const Node* fn_def) {
  for (int i = 0; i < fn_def->n_template_params; ++i) {
    if (!fn_def->template_param_names[i]) continue;
    if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) continue;
    const std::string pname = fn_def->template_param_names[i];
    if (deduced.count(pname)) continue;
    if (fn_def->template_param_has_default && fn_def->template_param_has_default[i]) continue;
    return false;
  }
  return true;
}

void Lowerer::fill_deduced_defaults(TypeBindings& deduced, const Node* fn_def) {
  if (!fn_def->template_param_has_default) return;
  for (int i = 0; i < fn_def->n_template_params; ++i) {
    if (!fn_def->template_param_names[i]) continue;
    if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) continue;
    const std::string pname = fn_def->template_param_names[i];
    if (deduced.count(pname)) continue;
    if (fn_def->template_param_has_default[i]) {
      deduced[pname] = fn_def->template_param_default_types[i];
    }
  }
}

TypeBindings Lowerer::merge_explicit_and_deduced_type_bindings(
    const Node* call_node, const Node* call_var, const Node* fn_def,
    const TypeBindings* enclosing_bindings, const Node* enclosing_fn) {
  TypeBindings bindings = build_call_bindings(call_var, fn_def, enclosing_bindings);
  if (!call_node || !fn_def) return bindings;

  TypeBindings deduced = try_deduce_template_type_args(call_node, fn_def, enclosing_fn);
  for (const auto& [name, ts] : deduced) {
    bindings.emplace(name, ts);
  }
  fill_deduced_defaults(bindings, fn_def);
  return bindings;
}

#if 0
ExprId Lowerer::lower_expr(FunctionCtx* ctx, const Node* n) {
  if (!n) {
    TypeSpec ts{};
    ts.base = TB_INT;
    return append_expr(nullptr, IntLiteral{0, false}, ts);
  }

  switch (n->kind) {
    case NK_INT_LIT: {
      // GCC &&label produces NK_INT_LIT with name set to the label name.
      if (n->name && n->name[0]) {
        LabelAddrExpr la{};
        la.label_name = n->name;
        la.fn_name = ctx ? ctx->fn->name : "";
        TypeSpec void_ptr{};
        void_ptr.base = TB_VOID;
        void_ptr.ptr_level = 1;
        return append_expr(n, std::move(la), void_ptr);
      }
      return append_expr(n, IntLiteral{n->ival, false}, infer_int_literal_type(n));
    }
    case NK_FLOAT_LIT: {
      TypeSpec ts = n->type;
      if (!has_concrete_type(ts)) ts = classify_float_literal_type(const_cast<Node*>(n));
      return append_expr(n, FloatLiteral{n->fval}, ts);
    }
    case NK_STR_LIT: {
      StringLiteral s{};
      s.raw = n->sval ? n->sval : "";
      s.is_wide = s.raw.size() >= 2 && s.raw[0] == 'L' && s.raw[1] == '"';
      TypeSpec ts = n->type;
      return append_expr(n, std::move(s), ts, ValueCategory::LValue);
    }
    case NK_CHAR_LIT: {
      // In C, character constants have type int.
      TypeSpec ts = n->type;
      ts.base = TB_INT;
      return append_expr(n, CharLiteral{n->ival}, ts);
    }
    case NK_PACK_EXPANSION:
      return lower_expr(ctx, n->left);
    case NK_VAR: {
      if (n->name && n->name[0]) {
        if (n->has_template_args && find_template_struct_primary(n->name)) {
          std::string arg_refs;
          for (int i = 0; i < n->n_template_args; ++i) {
            if (!arg_refs.empty()) arg_refs += ",";
            if (n->template_arg_is_value && n->template_arg_is_value[i]) {
              const char* fwd_name = n->template_arg_nttp_names ?
                  n->template_arg_nttp_names[i] : nullptr;
              if (fwd_name && fwd_name[0]) arg_refs += fwd_name;
              else arg_refs += std::to_string(n->template_arg_values[i]);
            } else if (n->template_arg_types) {
              const TypeSpec& arg_ts = n->template_arg_types[i];
              arg_refs += encode_template_type_arg_ref_hir(arg_ts);
            }
          }
          TypeSpec tmp_ts{};
          tmp_ts.base = TB_STRUCT;
          tmp_ts.array_size = -1;
          tmp_ts.inner_rank = -1;
          tmp_ts.tpl_struct_origin = n->name;
          tmp_ts.tpl_struct_arg_refs = ::strdup(arg_refs.c_str());
          const Node* primary_tpl = find_template_struct_primary(n->name);
          TypeBindings tpl_empty;
          NttpBindings nttp_empty;
          seed_and_resolve_pending_template_type_if_needed(
              tmp_ts, ctx ? ctx->tpl_bindings : tpl_empty,
              ctx ? ctx->nttp_bindings : nttp_empty, n,
              PendingTemplateTypeKind::OwnerStruct, "nameref-tpl-ctor",
              primary_tpl);
          if (tmp_ts.tag && module_->struct_defs.count(tmp_ts.tag)) {
            const LocalId tmp_lid = next_local_id();
            const std::string tmp_name = "__tmp_struct_" + std::to_string(tmp_lid.value);
            LocalDecl tmp{};
            tmp.id = tmp_lid;
            tmp.name = tmp_name;
            tmp.type = qtype_from(tmp_ts, ValueCategory::LValue);
            tmp.storage = StorageClass::Auto;
            tmp.span = make_span(n);
            if (ctx) {
              ctx->locals[tmp.name] = tmp.id;
              ctx->local_types[tmp.id.value] = tmp_ts;
              append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
              DeclRef tmp_ref{};
              tmp_ref.name = tmp_name;
              tmp_ref.local = tmp_lid;
              return append_expr(n, tmp_ref, tmp_ts, ValueCategory::LValue);
            }
          }
        }
        std::string qname = n->name;
        size_t scope_pos = qname.rfind("::");
        if (scope_pos != std::string::npos) {
          std::string struct_tag = qname.substr(0, scope_pos);
          std::string member = qname.substr(scope_pos + 2);
          if (!find_struct_static_member_decl(struct_tag, member) &&
              n->has_template_args && find_template_struct_primary(struct_tag)) {
            std::string arg_refs;
            for (int i = 0; i < n->n_template_args; ++i) {
              if (!arg_refs.empty()) arg_refs += ",";
              if (n->template_arg_is_value && n->template_arg_is_value[i]) {
                const char* fwd_name = n->template_arg_nttp_names ?
                    n->template_arg_nttp_names[i] : nullptr;
                if (fwd_name && fwd_name[0]) arg_refs += fwd_name;
                else arg_refs += std::to_string(n->template_arg_values[i]);
              } else if (n->template_arg_types) {
                const TypeSpec& arg_ts = n->template_arg_types[i];
                arg_refs += encode_template_type_arg_ref_hir(arg_ts);
              }
            }
            TypeSpec pending_ts{};
            pending_ts.base = TB_STRUCT;
            pending_ts.array_size = -1;
            pending_ts.inner_rank = -1;
            pending_ts.tpl_struct_origin = ::strdup(struct_tag.c_str());
            pending_ts.tpl_struct_arg_refs = ::strdup(arg_refs.c_str());
            const Node* primary_tpl = find_template_struct_primary(struct_tag);
            TypeBindings tpl_empty;
            NttpBindings nttp_empty;
            seed_and_resolve_pending_template_type_if_needed(
                pending_ts, ctx ? ctx->tpl_bindings : tpl_empty,
                ctx ? ctx->nttp_bindings : nttp_empty, n,
                PendingTemplateTypeKind::OwnerStruct, "nameref-scope-tpl",
                primary_tpl);
            if (pending_ts.tag && pending_ts.tag[0]) struct_tag = pending_ts.tag;
          }
          if (auto v = find_struct_static_member_const_value(struct_tag, member)) {
            TypeSpec ts{};
            if (const Node* decl = find_struct_static_member_decl(struct_tag, member))
              ts = decl->type;
            if (ts.base == TB_VOID) ts.base = TB_INT;
            return append_expr(n, IntLiteral{*v, false}, ts);
          }
          if (const Node* decl = find_struct_static_member_decl(struct_tag, member)) {
            if (decl->init) {
              long long v = static_eval_int(decl->init, enum_consts_);
              TypeSpec ts = decl->type;
              if (ts.base == TB_VOID) ts.base = TB_INT;
              return append_expr(n, IntLiteral{v, false}, ts);
            }
          }
        }
        auto it = enum_consts_.find(n->name);
        if (it != enum_consts_.end()) {
          TypeSpec ts{};
          ts.base = TB_INT;
          return append_expr(n, IntLiteral{it->second, false}, ts);
        }
        // Non-type template parameter: emit as integer literal.
        if (ctx) {
          auto nttp_it = ctx->nttp_bindings.find(n->name);
          if (nttp_it != ctx->nttp_bindings.end()) {
            TypeSpec ts{};
            ts.base = TB_INT;
            ts.array_size = -1;
            ts.inner_rank = -1;
            return append_expr(n, IntLiteral{nttp_it->second, false}, ts);
          }
        }
      }
      DeclRef r{};
      r.name = n->name ? n->name : "<anon_var>";
      r.ns_qual = make_ns_qual(n);
      bool has_local_binding = false;
      if (ctx) {
        auto lit = ctx->locals.find(r.name);
        if (lit != ctx->locals.end()) {
          r.local = lit->second;
          has_local_binding = true;
        }
        auto sit = ctx->static_globals.find(r.name);
        if (sit != ctx->static_globals.end()) {
          r.global = sit->second;
          if (const GlobalVar* gv = module_->find_global(*r.global)) r.name = gv->name;
          has_local_binding = true;
        }
        if (!has_local_binding) {
          auto pit = ctx->params.find(r.name);
          if (pit != ctx->params.end()) r.param_index = pit->second;
        }
      }
      if (!has_local_binding) {
        auto git = module_->global_index.find(r.name);
        if (git != module_->global_index.end()) r.global = git->second;
      }
      if (ctx && !ctx->method_struct_tag.empty() && !has_local_binding &&
          !r.param_index && !r.global) {
        if (auto v = find_struct_static_member_const_value(ctx->method_struct_tag, r.name)) {
          TypeSpec ts{};
          if (const Node* decl =
                  find_struct_static_member_decl(ctx->method_struct_tag, r.name)) {
            ts = decl->type;
          }
          if (ts.base == TB_VOID) ts.base = TB_INT;
          return append_expr(n, IntLiteral{*v, false}, ts);
        }
        auto sit = module_->struct_defs.find(ctx->method_struct_tag);
        if (sit != module_->struct_defs.end()) {
          for (const auto& fld : sit->second.fields) {
            if (fld.name == r.name) {
              DeclRef this_ref{};
              this_ref.name = "this";
              auto pit = ctx->params.find("this");
              if (pit != ctx->params.end()) this_ref.param_index = pit->second;
              TypeSpec this_ts{};
              this_ts.base = TB_STRUCT;
              this_ts.tag = sit->second.tag.c_str();
              this_ts.ptr_level = 1;
              ExprId this_id = append_expr(n, this_ref, this_ts, ValueCategory::LValue);
              MemberExpr me{};
              me.base = this_id;
              me.field = r.name;
              me.is_arrow = true;
              return append_expr(n, me, n->type, ValueCategory::LValue);
            }
          }
        }
      }
      TypeSpec var_ts = n->type;
      if (var_ts.base == TB_VOID && var_ts.ptr_level == 0 && var_ts.array_rank == 0 &&
          !r.local && !r.param_index && !r.global) {
        auto fit = module_->fn_index.find(r.name);
        if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
          var_ts = module_->functions[fit->second.value].return_type.spec;
          var_ts.ptr_level++;
          var_ts.is_fn_ptr = true;
        }
      }
      ExprId ref_id = append_expr(n, r, var_ts, ValueCategory::LValue);
      if (ctx) {
        if (auto storage_ts = storage_type_for_declref(ctx, r);
            storage_ts && is_any_ref_ts(*storage_ts)) {
          UnaryExpr u{};
          u.op = UnaryOp::Deref;
          u.operand = ref_id;
          return append_expr(n, u, reference_value_ts(*storage_ts), ValueCategory::LValue);
        }
      }
      return ref_id;
    }
    case NK_UNARY: {
      if (n->op) {
        const char* op_name = nullptr;
        if (std::string(n->op) == "++pre") op_name = "operator_preinc";
        else if (std::string(n->op) == "--pre") op_name = "operator_predec";
        else if (std::string(n->op) == "+") op_name = "operator_plus";
        else if (std::string(n->op) == "-") op_name = "operator_minus";
        else if (std::string(n->op) == "!") op_name = "operator_not";
        else if (std::string(n->op) == "~") op_name = "operator_bitnot";
        if (op_name) {
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {});
          if (op.valid()) return op;
        }
      }
      UnaryExpr u{};
      u.op = map_unary_op(n->op);
      u.operand = lower_expr(ctx, n->left);
      if (u.op == UnaryOp::Not) u.operand = maybe_bool_convert(ctx, u.operand, n->left);
      return append_expr(n, u, n->type);
    }
    case NK_POSTFIX: {
      if (n->op) {
        const char* op_name = nullptr;
        std::string op_str(n->op);
        if (op_str == "++") op_name = "operator_postinc";
        else if (op_str == "--") op_name = "operator_postdec";
        if (op_name) {
          TypeSpec int_ts{};
          int_ts.base = TB_INT;
          ExprId dummy = append_expr(n, IntLiteral{0, false}, int_ts);
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {}, {dummy});
          if (op.valid()) return op;
        }
      }
      UnaryExpr u{};
      const std::string op = n->op ? n->op : "";
      u.op = (op == "--") ? UnaryOp::PostDec : UnaryOp::PostInc;
      u.operand = lower_expr(ctx, n->left);
      return append_expr(n, u, n->type);
    }
    case NK_ADDR: {
      {
        ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_addr", {});
        if (op.valid()) return op;
      }
      if (n->left && n->left->kind == NK_VAR && n->left->name &&
          ct_state_->has_consteval_def(n->left->name)) {
        std::string diag = "error: cannot take address of consteval function '";
        diag += n->left->name;
        diag += "'";
        throw std::runtime_error(diag);
      }
      UnaryExpr u{};
      u.op = UnaryOp::AddrOf;
      u.operand = lower_expr(ctx, n->left);
      return append_expr(n, u, n->type);
    }
    case NK_DEREF: {
      {
        ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_deref", {});
        if (op.valid()) return op;
      }
      UnaryExpr u{};
      u.op = UnaryOp::Deref;
      u.operand = lower_expr(ctx, n->left);
      return append_expr(n, u, n->type, ValueCategory::LValue);
    }
    case NK_COMMA_EXPR: {
      BinaryExpr b{};
      b.op = BinaryOp::Comma;
      b.lhs = lower_expr(ctx, n->left);
      b.rhs = lower_expr(ctx, n->right);
      return append_expr(n, b, n->type);
    }
    case NK_BINOP: {
      if (n->op && n->right) {
        const char* op_name = nullptr;
        std::string op_str(n->op);
        if (op_str == "==") op_name = "operator_eq";
        else if (op_str == "!=") op_name = "operator_neq";
        else if (op_str == "+") op_name = "operator_plus";
        else if (op_str == "-") op_name = "operator_minus";
        else if (op_str == "*") op_name = "operator_mul";
        else if (op_str == "/") op_name = "operator_div";
        else if (op_str == "%") op_name = "operator_mod";
        else if (op_str == "&") op_name = "operator_bitand";
        else if (op_str == "|") op_name = "operator_bitor";
        else if (op_str == "^") op_name = "operator_bitxor";
        else if (op_str == "<<") op_name = "operator_shl";
        else if (op_str == ">>") op_name = "operator_shr";
        else if (op_str == "<") op_name = "operator_lt";
        else if (op_str == ">") op_name = "operator_gt";
        else if (op_str == "<=") op_name = "operator_le";
        else if (op_str == ">=") op_name = "operator_ge";
        if (op_name) {
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {n->right});
          if (op.valid()) return op;
        }
      }
      BinaryExpr b{};
      b.op = map_binary_op(n->op);
      b.lhs = lower_expr(ctx, n->left);
      b.rhs = lower_expr(ctx, n->right);
      if (b.op == BinaryOp::LAnd || b.op == BinaryOp::LOr) {
        b.lhs = maybe_bool_convert(ctx, b.lhs, n->left);
        b.rhs = maybe_bool_convert(ctx, b.rhs, n->right);
      }
      return append_expr(n, b, n->type);
    }
    case NK_ASSIGN: {
      if (n->kind == NK_ASSIGN && ctx && n->op) {
        const char* op_name = nullptr;
        std::string op_str(n->op);
        if (op_str == "=") op_name = "operator_assign";
        else if (op_str == "+=") op_name = "operator_plus_assign";
        else if (op_str == "-=") op_name = "operator_minus_assign";
        else if (op_str == "*=") op_name = "operator_mul_assign";
        else if (op_str == "/=") op_name = "operator_div_assign";
        else if (op_str == "%=") op_name = "operator_mod_assign";
        else if (op_str == "&=") op_name = "operator_and_assign";
        else if (op_str == "|=") op_name = "operator_or_assign";
        else if (op_str == "^=") op_name = "operator_xor_assign";
        else if (op_str == "<<=") op_name = "operator_shl_assign";
        else if (op_str == ">>=") op_name = "operator_shr_assign";
        if (op_name) {
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {n->right});
          if (op.valid()) return op;
        }
      }
      AssignExpr a{};
      a.op = map_assign_op(n->op, n->kind);
      a.lhs = lower_expr(ctx, n->left);
      a.rhs = lower_expr(ctx, n->right);
      return append_expr(n, a, n->type);
    }
    case NK_COMPOUND_ASSIGN: {
      if (n->op && ctx) {
        const char* op_name = nullptr;
        std::string op_str(n->op);
        if (op_str == "+=") op_name = "operator_plus_assign";
        else if (op_str == "-=") op_name = "operator_minus_assign";
        else if (op_str == "*=") op_name = "operator_mul_assign";
        else if (op_str == "/=") op_name = "operator_div_assign";
        else if (op_str == "%=") op_name = "operator_mod_assign";
        else if (op_str == "&=") op_name = "operator_and_assign";
        else if (op_str == "|=") op_name = "operator_or_assign";
        else if (op_str == "^=") op_name = "operator_xor_assign";
        else if (op_str == "<<=") op_name = "operator_shl_assign";
        else if (op_str == ">>=") op_name = "operator_shr_assign";
        if (op_name) {
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {n->right});
          if (op.valid()) return op;
        }
      }
      AssignExpr a{};
      a.op = map_assign_op(n->op, n->kind);
      a.lhs = lower_expr(ctx, n->left);
      a.rhs = lower_expr(ctx, n->right);
      return append_expr(n, a, n->type);
    }
    case NK_CAST: {
      CastExpr c{};
      TypeSpec cast_ts = n->type;
      if (ctx && !ctx->tpl_bindings.empty() && cast_ts.base == TB_TYPEDEF && cast_ts.tag) {
        auto it = ctx->tpl_bindings.find(cast_ts.tag);
        if (it != ctx->tpl_bindings.end()) {
          const TypeSpec& concrete = it->second;
          cast_ts.base = concrete.base;
          cast_ts.tag = concrete.tag;
        }
      }
      if (ctx && !ctx->tpl_bindings.empty() && cast_ts.tpl_struct_origin) {
        seed_and_resolve_pending_template_type_if_needed(
            cast_ts, ctx->tpl_bindings, ctx->nttp_bindings, n,
            PendingTemplateTypeKind::CastTarget, "cast-target");
      }
      c.to_type = qtype_from(cast_ts);
      c.expr = lower_expr(ctx, n->left);
      if (cast_ts.is_fn_ptr) c.fn_ptr_sig = fn_ptr_sig_from_decl_node(n);
      return append_expr(n, c, cast_ts);
    }
    case NK_CALL:
    case NK_BUILTIN_CALL:
      return lower_call_expr(ctx, n);
    case NK_VA_ARG: {
      VaArgExpr v{};
      v.ap = lower_expr(ctx, n->left);
      return append_expr(n, v, n->type);
    }
    case NK_INDEX: {
      if (n->right) {
        ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_subscript", {n->right});
        if (op.valid()) return op;
      }
      IndexExpr idx{};
      idx.base = lower_expr(ctx, n->left);
      idx.index = lower_expr(ctx, n->right);
      return append_expr(n, idx, n->type, ValueCategory::LValue);
    }
    case NK_MEMBER: {
      if (n->is_arrow && n->left) {
        ExprId arrow_ptr = try_lower_operator_call(ctx, n, n->left, "operator_arrow", {});
        if (arrow_ptr.valid()) {
          for (int depth = 0; depth < 8; ++depth) {
            const Expr* res_expr = module_->find_expr(arrow_ptr);
            if (!res_expr) break;
            TypeSpec rts = res_expr->type.spec;
            if (rts.ptr_level > 0) break;
            if (rts.base != TB_STRUCT || !rts.tag) break;
            std::string base_key = std::string(rts.tag) + "::operator_arrow";
            std::string const_key = base_key + "_const";
            auto mit = rts.is_const ? struct_methods_.find(const_key) : struct_methods_.find(base_key);
            if (mit == struct_methods_.end()) {
              mit = rts.is_const ? struct_methods_.find(base_key) : struct_methods_.find(const_key);
            }
            if (mit == struct_methods_.end()) break;
            auto fit = module_->fn_index.find(mit->second);
            TypeSpec fn_ts{};
            fn_ts.base = TB_VOID;
            if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
              fn_ts = module_->functions[fit->second.value].return_type.spec;
            }
            if (fn_ts.base == TB_VOID) {
              auto rit2 = struct_method_ret_types_.find(std::string(rts.tag) + "::operator_arrow");
              if (rit2 == struct_method_ret_types_.end()) {
                rit2 = struct_method_ret_types_.find(std::string(rts.tag) + "::operator_arrow_const");
              }
              if (rit2 != struct_method_ret_types_.end()) fn_ts = rit2->second;
            }
            LocalDecl tmp{};
            tmp.id = next_local_id();
            tmp.name = "__arrow_tmp";
            tmp.type = qtype_from(rts, ValueCategory::LValue);
            tmp.init = arrow_ptr;
            const LocalId tmp_lid = tmp.id;
            ctx->locals[tmp.name] = tmp.id;
            ctx->local_types[tmp.id.value] = rts;
            append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
            DeclRef tmp_ref{};
            tmp_ref.name = "__arrow_tmp";
            tmp_ref.local = tmp_lid;
            ExprId tmp_id = append_expr(n, tmp_ref, rts, ValueCategory::LValue);
            CallExpr cc{};
            DeclRef dr{};
            dr.name = mit->second;
            TypeSpec callee_ts = fn_ts;
            callee_ts.ptr_level++;
            cc.callee = append_expr(n, dr, callee_ts);
            UnaryExpr addr{};
            addr.op = UnaryOp::AddrOf;
            addr.operand = tmp_id;
            TypeSpec ptr_ts = rts;
            ptr_ts.ptr_level++;
            cc.args.push_back(append_expr(n, addr, ptr_ts));
            arrow_ptr = append_expr(n, cc, fn_ts);
          }
          MemberExpr m{};
          m.base = arrow_ptr;
          m.field = n->name ? n->name : "<anon_field>";
          m.is_arrow = true;
          return append_expr(n, m, n->type, ValueCategory::LValue);
        }
      }
      MemberExpr m{};
      m.base = lower_expr(ctx, n->left);
      m.field = n->name ? n->name : "<anon_field>";
      m.is_arrow = n->is_arrow;
      return append_expr(n, m, n->type, ValueCategory::LValue);
    }
    case NK_TERNARY: {
      if (const Node* cond = (n->cond ? n->cond : n->left)) {
        ConstEvalEnv env{&enum_consts_, nullptr, ctx ? &ctx->local_const_bindings : nullptr};
        if (auto r = evaluate_constant_expr(cond, env); r.ok()) {
          return lower_expr(ctx, (r.as_int() != 0) ? n->then_ : n->else_);
        }
      }
      if (ctx && (contains_stmt_expr(n->then_) || contains_stmt_expr(n->else_))) {
        TypeSpec result_ts = n->type;
        if (result_ts.base == TB_VOID) result_ts.base = TB_INT;
        LocalDecl tmp{};
        tmp.id = next_local_id();
        tmp.name = "__tern_tmp";
        tmp.type = qtype_from(result_ts, ValueCategory::LValue);
        TypeSpec zero_ts{};
        zero_ts.base = TB_INT;
        tmp.init = append_expr(n, IntLiteral{0, false}, zero_ts);
        const LocalId tmp_lid = tmp.id;
        ctx->locals[tmp.name] = tmp.id;
        ctx->local_types[tmp.id.value] = result_ts;
        append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
        const Node* cond_n = n->cond ? n->cond : n->left;
        ExprId cond_expr = maybe_bool_convert(ctx, lower_expr(ctx, cond_n), cond_n);
        const BlockId then_b = create_block(*ctx);
        const BlockId else_b = create_block(*ctx);
        const BlockId after_b = create_block(*ctx);
        IfStmt ifs{};
        ifs.cond = cond_expr;
        ifs.then_block = then_b;
        ifs.else_block = else_b;
        ifs.after_block = after_b;
        append_stmt(*ctx, Stmt{StmtPayload{ifs}, make_span(n)});
        ensure_block(*ctx, ctx->current_block).has_explicit_terminator = true;
        auto emit_branch = [&](BlockId blk, const Node* branch) {
          ctx->current_block = blk;
          ExprId val = lower_expr(ctx, branch);
          DeclRef lhs_ref{};
          lhs_ref.name = "__tern_tmp";
          lhs_ref.local = tmp_lid;
          ExprId lhs_id = append_expr(n, lhs_ref, result_ts, ValueCategory::LValue);
          AssignExpr assign{};
          assign.op = AssignOp::Set;
          assign.lhs = lhs_id;
          assign.rhs = val;
          ExprId assign_id = append_expr(n, assign, result_ts);
          append_stmt(*ctx, Stmt{StmtPayload{ExprStmt{assign_id}}, make_span(n)});
          if (!ensure_block(*ctx, ctx->current_block).has_explicit_terminator) {
            GotoStmt j{};
            j.target.resolved_block = after_b;
            append_stmt(*ctx, Stmt{StmtPayload{j}, make_span(n)});
            ensure_block(*ctx, ctx->current_block).has_explicit_terminator = true;
          }
        };
        emit_branch(then_b, n->then_);
        emit_branch(else_b, n->else_);
        ctx->current_block = after_b;
        DeclRef ref{};
        ref.name = "__tern_tmp";
        ref.local = tmp_lid;
        return append_expr(n, ref, result_ts, ValueCategory::LValue);
      }
      TernaryExpr t{};
      {
        const Node* cond_n = n->cond ? n->cond : n->left;
        t.cond = maybe_bool_convert(ctx, lower_expr(ctx, cond_n), cond_n);
      }
      t.then_expr = lower_expr(ctx, n->then_);
      t.else_expr = lower_expr(ctx, n->else_);
      return append_expr(n, t, n->type);
    }
    case NK_GENERIC: {
      TypeSpec ctrl_ts = infer_generic_ctrl_type(ctx, n->left);
      const Node* selected = nullptr;
      const Node* default_expr = nullptr;
      for (int i = 0; i < n->n_children; ++i) {
        const Node* assoc = n->children[i];
        if (!assoc) continue;
        const Node* expr = assoc->left;
        if (!expr) continue;
        if (assoc->ival == 1) {
          if (!default_expr) default_expr = expr;
          continue;
        }
        if (generic_type_compatible(ctrl_ts, assoc->type)) {
          selected = expr;
          break;
        }
      }
      if (!selected) selected = default_expr;
      if (selected) return lower_expr(ctx, selected);
      TypeSpec ts = n->type;
      if (ts.base == TB_VOID) ts.base = TB_INT;
      return append_expr(n, IntLiteral{0, false}, ts);
    }
    case NK_STMT_EXPR: {
      TypeSpec ts = n->type;
      if (ts.base == TB_VOID) ts.base = TB_INT;
      if (ctx && n->body) return lower_stmt_expr_block(*ctx, n->body, ts);
      return append_expr(n, IntLiteral{0, false}, ts);
    }
    case NK_REAL_PART:
    case NK_IMAG_PART: {
      UnaryExpr u{};
      u.op = (n->kind == NK_REAL_PART) ? UnaryOp::RealPart : UnaryOp::ImagPart;
      u.operand = lower_expr(ctx, n->left);
      return append_expr(n, u, n->type,
                         (n->left && n->left->type.ptr_level == 0 &&
                          n->left->type.array_rank == 0)
                             ? ValueCategory::LValue
                             : ValueCategory::RValue);
    }
    case NK_SIZEOF_EXPR: {
      SizeofExpr s{};
      s.expr = lower_expr(ctx, n->left);
      TypeSpec ts{};
      ts.base = TB_ULONG;
      return append_expr(n, s, ts);
    }
    case NK_SIZEOF_PACK: {
      long long pack_size = 0;
      std::string pack_name = n->sval ? n->sval : "";
      if (pack_name.empty() && n->left && n->left->kind == NK_VAR && n->left->name) {
        pack_name = n->left->name;
      }
      if (ctx && !pack_name.empty()) {
        pack_size = count_pack_bindings_for_name(ctx->tpl_bindings, ctx->nttp_bindings, pack_name);
      }
      IntLiteral lit{};
      lit.value = pack_size;
      TypeSpec ts{};
      ts.base = TB_ULONG;
      return append_expr(n, lit, ts);
    }
    case NK_SIZEOF_TYPE:
      return lower_builtin_sizeof_type(ctx, n);
    case NK_ALIGNOF_TYPE:
      return lower_builtin_alignof_type(ctx, n);
    case NK_ALIGNOF_EXPR:
      return lower_builtin_alignof_expr(ctx, n);
    case NK_COMPOUND_LIT: {
      if (!ctx) {
        TypeSpec ts = n->type;
        if (ts.base == TB_VOID) ts.base = TB_INT;
        return append_expr(n, IntLiteral{0, false}, ts);
      }
      const TypeSpec clit_ts = n->type;
      const Node* init_list = (n->left && n->left->kind == NK_INIT_LIST) ? n->left : nullptr;
      TypeSpec actual_ts = clit_ts;
      if (actual_ts.array_rank > 0 && actual_ts.array_size < 0 && init_list) {
        long long count = init_list->n_children;
        for (int ci = 0; ci < init_list->n_children; ++ci) {
          const Node* item = init_list->children[ci];
          if (item && item->kind == NK_INIT_ITEM && item->is_designated &&
              item->is_index_desig && item->desig_val + 1 > count) {
            count = item->desig_val + 1;
          }
        }
        actual_ts.array_size = count;
        actual_ts.array_dims[0] = count;
      }
      LocalDecl d{};
      d.id = next_local_id();
      d.name = "<clit>";
      d.type = qtype_from(actual_ts, ValueCategory::LValue);
      d.storage = StorageClass::Auto;
      d.span = make_span(n);
      const LocalId lid = d.id;
      const TypeSpec decl_ts = actual_ts;
      const bool is_struct_or_union =
          (decl_ts.base == TB_STRUCT || decl_ts.base == TB_UNION) && decl_ts.ptr_level == 0 &&
          decl_ts.array_rank == 0;
      const bool is_array = decl_ts.array_rank > 0;
      const bool is_vector = is_vector_ty(decl_ts);
      if ((is_struct_or_union || is_array || is_vector) && init_list) {
        TypeSpec int_ts{};
        int_ts.base = TB_INT;
        d.init = append_expr(n, IntLiteral{0, false}, int_ts);
      } else if (init_list && init_list->n_children > 0) {
        d.init = lower_expr(ctx, unwrap_init_scalar_value(init_list));
      } else if (n->left && !init_list) {
        d.init = lower_expr(ctx, n->left);
      }
      ctx->local_types[lid.value] = decl_ts;
      append_stmt(*ctx, Stmt{StmtPayload{std::move(d)}, make_span(n)});
      DeclRef dr{};
      dr.name = "<clit>";
      dr.local = lid;
      ExprId base_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);
      if (init_list && (is_struct_or_union || is_array || is_vector)) {
        auto is_agg = [](const TypeSpec& ts) {
          return ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0 &&
                  ts.array_rank == 0) ||
                 is_vector_ty(ts);
        };
        auto append_assign = [&](ExprId lhs_id, const TypeSpec& lhs_ts, const Node* rhs_node) {
          if (!rhs_node) return;
          if (lhs_ts.array_rank == 1 && rhs_node->kind == NK_STR_LIT && rhs_node->sval) {
            TypeSpec elem_ts = lhs_ts;
            elem_ts.array_rank = 0;
            elem_ts.array_size = -1;
            if (elem_ts.ptr_level == 0 &&
                (elem_ts.base == TB_CHAR || elem_ts.base == TB_SCHAR || elem_ts.base == TB_UCHAR)) {
              const bool is_wide = rhs_node->sval[0] == 'L';
              const auto vals = decode_string_literal_values(rhs_node->sval, is_wide);
              const long long max_count = lhs_ts.array_size > 0 ? lhs_ts.array_size
                                                                : static_cast<long long>(vals.size());
              const long long emit_n =
                  std::min<long long>(max_count, static_cast<long long>(vals.size()));
              for (long long idx = 0; idx < emit_n; ++idx) {
                TypeSpec idx_ts{};
                idx_ts.base = TB_INT;
                ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
                IndexExpr ie{};
                ie.base = lhs_id;
                ie.index = idx_id;
                ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
                ExprId val_id =
                    append_expr(n, IntLiteral{vals[static_cast<size_t>(idx)], false}, idx_ts);
                AssignExpr ae{};
                ae.lhs = ie_id;
                ae.rhs = val_id;
                ExprId ae_id = append_expr(n, ae, elem_ts);
                ExprStmt es{};
                es.expr = ae_id;
                append_stmt(*ctx, Stmt{StmtPayload{es}, make_span(n)});
              }
              return;
            }
          }
          const Node* val_node = unwrap_init_scalar_value(rhs_node);
          ExprId val_id = lower_expr(ctx, val_node);
          AssignExpr ae{};
          ae.lhs = lhs_id;
          ae.rhs = val_id;
          ExprId ae_id = append_expr(n, ae, lhs_ts);
          ExprStmt es{};
          es.expr = ae_id;
          append_stmt(*ctx, Stmt{StmtPayload{es}, make_span(n)});
        };
        auto can_direct_assign_agg = [&](const TypeSpec& lhs_ts, const Node* rhs_node) -> bool {
          if (!rhs_node) return false;
          if (!is_agg(lhs_ts)) return false;
          const TypeSpec rhs_ts = infer_generic_ctrl_type(ctx, rhs_node);
          if (rhs_ts.ptr_level != 0 || rhs_ts.array_rank != 0) return false;
          if (rhs_ts.base != lhs_ts.base) return false;
          if (rhs_ts.base == TB_STRUCT || rhs_ts.base == TB_UNION || rhs_ts.base == TB_ENUM) {
            const char* lt = lhs_ts.tag ? lhs_ts.tag : "";
            const char* rt = rhs_ts.tag ? rhs_ts.tag : "";
            return std::strcmp(lt, rt) == 0;
          }
          return true;
        };
        auto is_direct_char_array_init = [&](const TypeSpec& lhs_ts, const Node* rhs_node) -> bool {
          if (!rhs_node) return false;
          if (lhs_ts.array_rank != 1 || lhs_ts.ptr_level != 0) return false;
          if (!(lhs_ts.base == TB_CHAR || lhs_ts.base == TB_SCHAR || lhs_ts.base == TB_UCHAR)) {
            return false;
          }
          return rhs_node->kind == NK_STR_LIT;
        };
        std::function<void(const TypeSpec&, ExprId, const Node*, int&)> consume_from_list;
        consume_from_list = [&](const TypeSpec& cur_ts, ExprId cur_lhs, const Node* list_node,
                                int& cursor) {
          if (!list_node || list_node->kind != NK_INIT_LIST) return;
          if (cursor < 0) cursor = 0;
          if (is_agg(cur_ts) && cur_ts.tag) {
            const auto sit = module_->struct_defs.find(cur_ts.tag);
            if (sit == module_->struct_defs.end()) return;
            const auto& sd = sit->second;
            size_t next_field = 0;
            while (cursor < list_node->n_children && next_field < sd.fields.size()) {
              const Node* item = list_node->children[cursor];
              if (!item) {
                ++cursor;
                ++next_field;
                continue;
              }
              size_t fi = next_field;
              if (item->kind == NK_INIT_ITEM && item->is_designated && !item->is_index_desig &&
                  item->desig_field) {
                for (size_t k = 0; k < sd.fields.size(); ++k) {
                  if (sd.fields[k].name == item->desig_field) {
                    fi = k;
                    break;
                  }
                }
              }
              if (fi >= sd.fields.size()) {
                ++cursor;
                continue;
              }
              const HirStructField& fld = sd.fields[fi];
              TypeSpec field_ts = field_type_of(fld);
              MemberExpr me{};
              me.base = cur_lhs;
              me.field = fld.name;
              me.is_arrow = false;
              ExprId me_id = append_expr(n, me, field_ts, ValueCategory::LValue);
              const Node* val_node = init_item_value_node(item);
              if (is_agg(field_ts) || field_ts.array_rank > 0) {
                if (val_node && val_node->kind == NK_INIT_LIST) {
                  int sub_cursor = 0;
                  consume_from_list(field_ts, me_id, val_node, sub_cursor);
                  ++cursor;
                } else if (is_direct_char_array_init(field_ts, val_node)) {
                  append_assign(me_id, field_ts, val_node);
                  ++cursor;
                } else if (can_direct_assign_agg(field_ts, val_node)) {
                  append_assign(me_id, field_ts, val_node);
                  ++cursor;
                } else if (val_node && item->kind == NK_INIT_ITEM && item->is_designated &&
                           !item->is_index_desig && item->desig_field) {
                  append_assign(me_id, field_ts, val_node);
                  ++cursor;
                } else {
                  consume_from_list(field_ts, me_id, list_node, cursor);
                }
              } else {
                if (val_node && val_node->kind == NK_INIT_LIST) {
                  int sub_cursor = 0;
                  consume_from_list(field_ts, me_id, val_node, sub_cursor);
                  ++cursor;
                } else {
                  append_assign(me_id, field_ts, val_node);
                  ++cursor;
                }
              }
              next_field = fi + 1;
            }
            return;
          }
          if (cur_ts.array_rank > 0 || is_vector_ty(cur_ts)) {
            if (cursor < list_node->n_children) {
              const Node* item0 = list_node->children[cursor];
              const Node* val0 = init_item_value_node(item0);
              if (is_direct_char_array_init(cur_ts, val0)) {
                append_assign(cur_lhs, cur_ts, val0);
                ++cursor;
                return;
              }
            }
            TypeSpec elem_ts = cur_ts;
            long long bound = 0;
            if (is_vector_ty(cur_ts)) {
              elem_ts = vector_element_type(cur_ts);
              bound = cur_ts.vector_lanes > 0 ? cur_ts.vector_lanes : (1LL << 30);
            } else {
              elem_ts.array_rank--;
              elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
              bound = cur_ts.array_size > 0 ? cur_ts.array_size : (1LL << 30);
            }
            long long next_idx = 0;
            while (cursor < list_node->n_children && next_idx < bound) {
              const Node* item = list_node->children[cursor];
              if (!item) {
                ++cursor;
                ++next_idx;
                continue;
              }
              long long idx = next_idx;
              if (item->kind == NK_INIT_ITEM && item->is_designated && item->is_index_desig) {
                idx = item->desig_val;
              }
              TypeSpec idx_ts{};
              idx_ts.base = TB_INT;
              ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
              IndexExpr ie{};
              ie.base = cur_lhs;
              ie.index = idx_id;
              ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
              const Node* val_node = init_item_value_node(item);
              if (is_agg(elem_ts) || elem_ts.array_rank > 0) {
                if (val_node && val_node->kind == NK_INIT_LIST) {
                  int sub_cursor = 0;
                  consume_from_list(elem_ts, ie_id, val_node, sub_cursor);
                  ++cursor;
                } else if (is_direct_char_array_init(elem_ts, val_node)) {
                  append_assign(ie_id, elem_ts, val_node);
                  ++cursor;
                } else if (can_direct_assign_agg(elem_ts, val_node)) {
                  append_assign(ie_id, elem_ts, val_node);
                  ++cursor;
                } else {
                  consume_from_list(elem_ts, ie_id, list_node, cursor);
                }
              } else {
                if (val_node && val_node->kind == NK_INIT_LIST) {
                  int sub_cursor = 0;
                  consume_from_list(elem_ts, ie_id, val_node, sub_cursor);
                  ++cursor;
                } else {
                  append_assign(ie_id, elem_ts, val_node);
                  ++cursor;
                }
              }
              next_idx = idx + 1;
            }
            return;
          }
          if (cursor < list_node->n_children) {
            const Node* item = list_node->children[cursor];
            const Node* val_node = init_item_value_node(item);
            if (val_node && val_node->kind == NK_INIT_LIST) {
              int sub_cursor = 0;
              consume_from_list(cur_ts, cur_lhs, val_node, sub_cursor);
            } else {
              append_assign(cur_lhs, cur_ts, val_node);
            }
            ++cursor;
          }
        };
        int cursor = 0;
        consume_from_list(decl_ts, base_id, init_list, cursor);
      }
      return base_id;
    }
    case NK_NEW_EXPR: {
      TypeSpec alloc_ts = n->type;
      resolve_typedef_to_struct(alloc_ts);
      bool is_array = n->ival != 0;
      SizeofTypeExpr sot{};
      sot.type = qtype_from(alloc_ts, ValueCategory::RValue);
      TypeSpec ulong_ts{};
      ulong_ts.base = TB_ULONG;
      ExprId size_id = append_expr(n, sot, ulong_ts);
      if (is_array && n->right) {
        ExprId count_id = lower_expr(ctx, n->right);
        CastExpr cast_count{};
        cast_count.to_type = qtype_from(ulong_ts, ValueCategory::RValue);
        cast_count.expr = count_id;
        ExprId count_ulong = append_expr(n, cast_count, ulong_ts);
        BinaryExpr mul{};
        mul.op = BinaryOp::Mul;
        mul.lhs = size_id;
        mul.rhs = count_ulong;
        size_id = append_expr(n, mul, ulong_ts);
      }
      std::string op_fn;
      bool is_class_specific = false;
      if (!n->is_global_qualified && alloc_ts.base == TB_STRUCT && alloc_ts.tag) {
        std::string class_key = std::string(alloc_ts.tag) + "::";
        class_key += is_array ? "operator_new_array" : "operator_new";
        if (struct_methods_.count(class_key)) {
          op_fn = struct_methods_[class_key];
          is_class_specific = true;
        }
      }
      if (op_fn.empty()) op_fn = is_array ? "operator_new_array" : "operator_new";
      TypeSpec void_ptr_ts{};
      void_ptr_ts.base = TB_VOID;
      void_ptr_ts.ptr_level = 1;
      ExprId raw_ptr;
      if (n->left && n->is_global_qualified) {
        raw_ptr = lower_expr(ctx, n->left);
      } else {
        CallExpr call{};
        DeclRef callee_ref{};
        callee_ref.name = op_fn;
        TypeSpec fn_ptr_ts{};
        fn_ptr_ts.base = TB_VOID;
        fn_ptr_ts.ptr_level = 1;
        call.callee = append_expr(n, callee_ref, fn_ptr_ts);
        if (is_class_specific) {
          call.args.push_back(append_expr(n, IntLiteral{0, false}, void_ptr_ts));
        }
        call.args.push_back(size_id);
        if (n->left) {
          for (int pi = 0; pi < n->n_params; ++pi) {
            call.args.push_back(lower_expr(ctx, n->params[pi]));
          }
        }
        raw_ptr = append_expr(n, call, void_ptr_ts);
      }
      TypeSpec result_ts = alloc_ts;
      result_ts.ptr_level++;
      CastExpr cast{};
      cast.to_type = qtype_from(result_ts, ValueCategory::RValue);
      cast.expr = raw_ptr;
      ExprId typed_ptr = append_expr(n, cast, result_ts);
      if (n->n_children > 0 && alloc_ts.base == TB_STRUCT && alloc_ts.tag) {
        auto cit = struct_constructors_.find(alloc_ts.tag);
        if (cit != struct_constructors_.end() && !cit->second.empty()) {
          const CtorOverload* best = nullptr;
          if (cit->second.size() == 1) {
            best = &cit->second[0];
          } else {
            for (const auto& ov : cit->second) {
              if (ov.method_node->n_params != n->n_children) continue;
              best = &ov;
              break;
            }
          }
          if (best && !best->method_node->is_deleted) {
            LocalDecl tmp_decl{};
            tmp_decl.id = next_local_id();
            std::string tmp_name = "__new_tmp_" + std::to_string(tmp_decl.id.value);
            tmp_decl.name = tmp_name;
            tmp_decl.type = qtype_from(result_ts, ValueCategory::RValue);
            tmp_decl.init = typed_ptr;
            append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp_decl)}, make_span(n)});
            DeclRef tmp_ref{};
            tmp_ref.name = tmp_name;
            tmp_ref.local = LocalId{tmp_decl.id.value};
            ExprId tmp_id = append_expr(n, tmp_ref, result_ts, ValueCategory::LValue);
            CallExpr ctor_call{};
            DeclRef ctor_ref{};
            ctor_ref.name = best->mangled_name;
            TypeSpec ctor_fn_ts{};
            ctor_fn_ts.base = TB_VOID;
            ctor_fn_ts.ptr_level = 1;
            ctor_call.callee = append_expr(n, ctor_ref, ctor_fn_ts);
            ctor_call.args.push_back(tmp_id);
            for (int i = 0; i < n->n_children; ++i) {
              ctor_call.args.push_back(lower_expr(ctx, n->children[i]));
            }
            TypeSpec void_ts{};
            void_ts.base = TB_VOID;
            ExprId ctor_result = append_expr(n, ctor_call, void_ts);
            ExprStmt es{};
            es.expr = ctor_result;
            append_stmt(*ctx, Stmt{StmtPayload{std::move(es)}, make_span(n)});
            DeclRef ret_ref{};
            ret_ref.name = tmp_name;
            ret_ref.local = LocalId{tmp_decl.id.value};
            return append_expr(n, ret_ref, result_ts, ValueCategory::LValue);
          }
        }
      }
      return typed_ptr;
    }
    case NK_DELETE_EXPR: {
      bool is_array = n->ival != 0;
      ExprId operand = lower_expr(ctx, n->left);
      TypeSpec operand_ts = infer_generic_ctrl_type(ctx, n->left);
      std::string op_fn;
      bool is_class_specific = false;
      if (operand_ts.base == TB_STRUCT && operand_ts.tag && operand_ts.ptr_level > 0) {
        std::string class_key = std::string(operand_ts.tag) + "::";
        class_key += is_array ? "operator_delete_array" : "operator_delete";
        if (struct_methods_.count(class_key)) {
          op_fn = struct_methods_[class_key];
          is_class_specific = true;
        }
      }
      if (op_fn.empty()) op_fn = is_array ? "operator_delete_array" : "operator_delete";
      TypeSpec void_ptr_ts{};
      void_ptr_ts.base = TB_VOID;
      void_ptr_ts.ptr_level = 1;
      CastExpr cast{};
      cast.to_type = qtype_from(void_ptr_ts, ValueCategory::RValue);
      cast.expr = operand;
      ExprId void_operand = append_expr(n, cast, void_ptr_ts);
      CallExpr call{};
      DeclRef callee_ref{};
      callee_ref.name = op_fn;
      TypeSpec fn_ptr_ts{};
      fn_ptr_ts.base = TB_VOID;
      fn_ptr_ts.ptr_level = 1;
      call.callee = append_expr(n, callee_ref, fn_ptr_ts);
      if (is_class_specific) {
        call.args.push_back(append_expr(n, IntLiteral{0, false}, void_ptr_ts));
      }
      call.args.push_back(void_operand);
      TypeSpec void_ts{};
      void_ts.base = TB_VOID;
      ExprId del_call = append_expr(n, call, void_ts);
      ExprStmt es{};
      es.expr = del_call;
      append_stmt(*ctx, Stmt{StmtPayload{std::move(es)}, make_span(n)});
      TypeSpec int_ts{};
      int_ts.base = TB_INT;
      return append_expr(n, IntLiteral{0, false}, int_ts);
    }
    case NK_INVALID_EXPR:
    case NK_INVALID_STMT: {
      TypeSpec ts{};
      ts.base = TB_INT;
      return append_expr(n, IntLiteral{0, false}, ts);
    }
    default: {
      TypeSpec ts = n->type;
      if (ts.base == TB_VOID && n->kind != NK_CALL) ts.base = TB_INT;
      return append_expr(n, IntLiteral{0, false}, ts);
    }
  }
}
#endif


void Lowerer::lower_struct_def(const Node* sd) {
  if (!sd || sd->kind != NK_STRUCT_DEF) return;
  // Skip primary templates and specialization patterns that still depend on
  // template parameters. Concrete explicit specializations and realized
  // instantiations are still lowered as ordinary structs.
  if (is_primary_template_struct_def(sd) ||
      is_dependent_template_struct_specialization(sd)) {
    return;
  }
  const char* tag = sd->name;
  if (!tag || !tag[0]) return;

  // Gather fields: they may be in sd->fields[] (n_fields) OR sd->children[] (n_children)
  // The IR builder uses sd->fields; but some parsers store struct fields in children.
  int num_fields = sd->n_fields > 0 ? sd->n_fields : sd->n_children;
  auto get_field = [&](int i) -> const Node* {
    return sd->n_fields > 0 ? sd->fields[i] : sd->children[i];
  };

  // If already fully populated, skip (forward decl followed by full def is OK)
  if (module_->struct_defs.count(tag) &&
      !module_->struct_defs.at(tag).fields.empty() &&
      num_fields == 0) return;

  HirStructDef def;
  def.tag = tag;
  def.ns_qual = make_ns_qual(sd);
  def.is_union = sd->is_union;
  def.pack_align = sd->pack_align;
  def.struct_align = sd->struct_align;
  for (int bi = 0; bi < sd->n_bases; ++bi) {
    TypeSpec base = sd->base_types[bi];
    if (base.tpl_struct_origin) {
      // Resolve pending template base type (e.g. deferred $expr: NTTP args).
      // Build bindings from the struct's own template args if available.
      TypeBindings base_tpl_bindings;
      NttpBindings base_nttp_bindings;
      if (sd->n_template_args > 0 && sd->n_template_params > 0) {
        for (int pi = 0; pi < sd->n_template_params && pi < sd->n_template_args; ++pi) {
          const char* pname = sd->template_param_names[pi];
          if (sd->template_param_is_nttp[pi]) {
            base_nttp_bindings[pname] = sd->template_arg_values[pi];
          } else {
            base_tpl_bindings[pname] = sd->template_arg_types[pi];
          }
        }
      }
      seed_and_resolve_pending_template_type_if_needed(
          base, base_tpl_bindings, base_nttp_bindings, sd,
          PendingTemplateTypeKind::BaseType,
          std::string("struct-base:") + (tag ? tag : ""));
    }
    if (!resolve_struct_member_typedef_if_ready(&base) &&
        base.deferred_member_type_name && base.tag && base.tag[0]) {
      TypeBindings empty_tb;
      NttpBindings empty_nb;
      seed_pending_template_type(
          base, empty_tb, empty_nb, sd, PendingTemplateTypeKind::MemberTypedef,
          std::string("struct-base-member:") + (tag ? tag : ""));
    }
    if (base.tag && base.tag[0]) def.base_tags.push_back(base.tag);
  }

  int llvm_idx = 0;
  // Bitfield packing state (for structs only; unions always use offset 0)
  int bf_unit_start_bit = -1;  // bit position where current storage unit starts (-1 = none)
  int bf_unit_bits = 0;        // size of current storage unit in bits
  int bf_current_bit = 0;      // current bit position within the storage unit

  // Build NTTP bindings for template instantiations so static constexpr
  // members that reference NTTP parameters (e.g. `static constexpr T value = v;`)
  // are evaluated correctly.
  NttpBindings struct_nttp_bindings;
  if (sd->n_template_args > 0 && sd->n_template_params > 0) {
    for (int pi = 0; pi < sd->n_template_params && pi < sd->n_template_args; ++pi) {
      const char* pname = sd->template_param_names[pi];
      if (sd->template_param_is_nttp[pi]) {
        struct_nttp_bindings[pname] = sd->template_arg_values[pi];
      }
    }
  }

  for (int i = 0; i < num_fields; ++i) {
    const Node* f = get_field(i);
    if (!f) continue;
    // Struct methods live in sd->children[] alongside fields for some parser
    // paths, but they are not data members and must not participate in
    // layout. They are collected separately after the layout pass.
    if (f->kind == NK_FUNCTION) continue;
    if (f->name && f->name[0])
      struct_static_member_decls_[tag][f->name] = f;
    if (f->is_static && f->is_constexpr && f->name && f->name[0] && f->init) {
      struct_static_member_const_values_[tag][f->name] =
          struct_nttp_bindings.empty()
              ? static_eval_int(f->init, enum_consts_)
              : eval_const_int_with_nttp_bindings(f->init, struct_nttp_bindings);
    }
    if (f->is_static)
      continue;

    const int bit_width = static_cast<int>(f->ival);  // -1 = not bitfield, 0+ = bitfield width
    const bool is_bitfield = (bit_width >= 0);

    // Skip anonymous non-bitfield fields (they have no name) but keep named fields
    // and bitfield fields (including anonymous bitfields for layout purposes).
    if (!f->name && !is_bitfield) continue;
    // Zero-width bitfield: force alignment, close current storage unit
    if (is_bitfield && bit_width == 0) {
      if (!sd->is_union && bf_unit_start_bit >= 0) {
        bf_unit_start_bit = -1;
        bf_current_bit = 0;
      }
      continue;
    }
    // Anonymous bitfields with width > 0 but no name: skip as field but advance bit position
    if (!f->name && is_bitfield) {
      if (!sd->is_union && bf_unit_start_bit >= 0) {
        bf_current_bit += bit_width;
        if (bf_current_bit > bf_unit_bits) {
          // Doesn't fit, start new unit
          bf_unit_start_bit = -1;
          bf_current_bit = 0;
        }
      }
      continue;
    }

    HirStructField hf;
    hf.name = f->name;
    TypeSpec ft = f->type;

    if (is_bitfield && !sd->is_union) {
      // Determine signedness from original declared type
      const bool bf_signed = (ft.base == TB_INT || ft.base == TB_CHAR ||
                              ft.base == TB_SCHAR || ft.base == TB_SHORT ||
                              ft.base == TB_LONG || ft.base == TB_LONGLONG ||
                              ft.base == TB_INT128);
      // Determine storage unit size from declared type
      int decl_unit_bits = static_cast<int>(sizeof_base(ft.base) * 8);
      if (decl_unit_bits < 8) decl_unit_bits = 8;

      // Can we pack into current storage unit?
      bool can_pack = (bf_unit_start_bit >= 0) &&
                      (bf_current_bit + bit_width <= bf_unit_bits);

      if (can_pack) {
        hf.bit_offset = bf_current_bit;
        hf.storage_unit_bits = bf_unit_bits;
        hf.bit_width = bit_width;
        hf.llvm_idx = llvm_idx - 1;  // same LLVM field as previous
      } else {
        // Start new storage unit
        bf_unit_start_bit = 0;
        bf_unit_bits = decl_unit_bits;
        bf_current_bit = 0;
        hf.bit_offset = 0;
        hf.storage_unit_bits = decl_unit_bits;
        hf.bit_width = bit_width;
        hf.llvm_idx = llvm_idx;
        ++llvm_idx;
      }
      bf_current_bit = hf.bit_offset + bit_width;
      hf.is_bf_signed = bf_signed;

      // Set elem_type to the storage unit's integer type
      TypeSpec sft{};
      switch (hf.storage_unit_bits) {
        case 8:  sft.base = TB_UCHAR; break;
        case 16: sft.base = TB_USHORT; break;
        case 32: sft.base = TB_UINT; break;
        case 64: sft.base = TB_ULONGLONG; break;
        default: sft.base = TB_UINT; break;
      }
      hf.elem_type = sft;
      hf.is_anon_member = f->is_anon_field;
      def.fields.push_back(std::move(hf));
      continue;
    }

    if (is_bitfield && sd->is_union) {
      const bool bf_signed = (ft.base == TB_INT || ft.base == TB_CHAR ||
                              ft.base == TB_SCHAR || ft.base == TB_SHORT ||
                              ft.base == TB_LONG || ft.base == TB_LONGLONG ||
                              ft.base == TB_INT128);
      int decl_unit_bits = static_cast<int>(sizeof_base(ft.base) * 8);
      if (decl_unit_bits < 8) decl_unit_bits = 8;
      hf.bit_width = bit_width;
      hf.bit_offset = 0;
      hf.storage_unit_bits = decl_unit_bits;
      hf.is_bf_signed = bf_signed;
      hf.llvm_idx = 0;
      TypeSpec sft{};
      switch (hf.storage_unit_bits) {
        case 8:  sft.base = TB_UCHAR; break;
        case 16: sft.base = TB_USHORT; break;
        case 32: sft.base = TB_UINT; break;
        case 64: sft.base = TB_ULONGLONG; break;
        default: sft.base = TB_UINT; break;
      }
      hf.elem_type = sft;
      hf.is_anon_member = f->is_anon_field;
      def.fields.push_back(std::move(hf));
      continue;
    }

    // Non-bitfield: flush any open bitfield storage unit
    if (!sd->is_union && bf_unit_start_bit >= 0) {
      bf_unit_start_bit = -1;
      bf_current_bit = 0;
    }

    // Extract first array dimension (keep base element type for LLVM)
    if (ft.array_rank > 0) {
      hf.is_flexible_array = (ft.array_size < 0);
      // Flexible array member is represented as zero-length for the nominal
      // struct layout, while remaining explicit in HIR metadata.
      hf.array_first_dim = (ft.array_size >= 0) ? ft.array_size : 0;
      for (int i = 0; i + 1 < ft.array_rank; ++i) ft.array_dims[i] = ft.array_dims[i + 1];
      if (ft.array_rank > 0) ft.array_dims[ft.array_rank - 1] = -1;
      --ft.array_rank;
      ft.array_size = (ft.array_rank > 0) ? ft.array_dims[0] : -1;
    }
    hf.elem_type = ft;
    hf.align_bytes = ft.align_bytes > 0 ? ft.align_bytes : 0;
    hf.is_anon_member = f->is_anon_field;
    hf.llvm_idx = sd->is_union ? 0 : llvm_idx;
    // Phase C: capture fn_ptr signature for callable struct fields.
    // Use canonical type directly (struct fields aren't in ResolvedTypeTable).
    {
      auto ct = std::make_shared<sema::CanonicalType>(sema::canonicalize_declarator_type(f));
      if (sema::is_callable_type(*ct)) {
        const auto* fsig = sema::get_function_sig(*ct);
        if (fsig) {
          FnPtrSig sig{};
          sig.canonical_sig = ct;
          if (fsig->return_type)
            sig.return_type = qtype_from(sema::typespec_from_canonical(*fsig->return_type));
          sig.variadic = fsig->is_variadic;
          sig.unspecified_params = fsig->unspecified_params;
          for (const auto& param : fsig->params)
            sig.params.push_back(qtype_from(sema::typespec_from_canonical(param), ValueCategory::LValue));
          hf.fn_ptr_sig = std::move(sig);
        }
      }
    }
    def.fields.push_back(std::move(hf));
    if (!sd->is_union) ++llvm_idx;
  }

  compute_struct_layout(module_, def);

  if (!module_->struct_defs.count(tag))
    module_->struct_def_order.push_back(tag);
  module_->struct_defs[tag] = std::move(def);

  // Collect struct methods (stored in sd->children[]) for later lowering.
  // If the struct was parser-instantiated from a template, extract the
  // template bindings so method bodies can resolve pending template types.
  TypeBindings method_tpl_bindings;
  NttpBindings method_nttp_bindings;
  if (sd->n_template_args > 0 && sd->template_param_names) {
    for (int i = 0; i < sd->n_template_args; ++i) {
      const char* pname = sd->template_param_names[i];
      if (!pname) continue;
      if (sd->template_param_is_nttp && sd->template_param_is_nttp[i]) {
        method_nttp_bindings[pname] = sd->template_arg_values[i];
      } else if (sd->template_arg_types) {
        method_tpl_bindings[pname] = sd->template_arg_types[i];
      }
    }
  }
  for (int i = 0; i < sd->n_children; ++i) {
    const Node* method = sd->children[i];
    if (!method || method->kind != NK_FUNCTION || !method->name) continue;
    const char* const_suffix = method->is_const_method ? "_const" : "";
    // Constructors get special handling: stored in struct_constructors_
    // with unique mangled names to support overloading by parameter types.
    if (method->is_constructor) {
      auto& ctors = struct_constructors_[tag];
      int ctor_idx = (int)ctors.size();
      std::string mangled = std::string(tag) + "__" + method->name;
      if (ctor_idx > 0) mangled += "__" + std::to_string(ctor_idx);
      ctors.push_back({mangled, method});
      // Also register in struct_methods_ so the first ctor is findable.
      if (ctor_idx == 0) {
        std::string key = std::string(tag) + "::" + method->name;
        struct_methods_[key] = mangled;
        struct_method_ret_types_[key] = method->type;
      }
      pending_methods_.push_back({mangled, std::string(tag), method,
                                  method_tpl_bindings, method_nttp_bindings});
      continue;
    }
    if (method->is_destructor) {
      std::string mangled = std::string(tag) + "__dtor";
      struct_destructors_[tag] = {mangled, method};
      pending_methods_.push_back({mangled, std::string(tag), method,
                                  method_tpl_bindings, method_nttp_bindings});
      continue;
    }
    std::string mangled = std::string(tag) + "__" + method->name + const_suffix;
    std::string key = std::string(tag) + "::" + method->name + const_suffix;
    // Detect ref-overloaded methods: if this key already exists, check for
    // ref-qualifier difference and register in the overload set.
    if (struct_methods_.count(key)) {
      // This is a second overload of the same method — mangle differently.
      mangled += "__rref";
      // Register both in the ref_overload_set_ for call-site resolution.
      auto& ovset = ref_overload_set_[key];
      if (ovset.empty()) {
        // Register the first overload.
        RefOverloadEntry e0;
        e0.mangled_name = struct_methods_[key];
        // Find the first method's params from pending_methods_.
        for (const auto& pm : pending_methods_) {
          if (pm.mangled == struct_methods_[key]) {
            for (int pi = 0; pi < pm.method_node->n_params; ++pi) {
              e0.param_is_rvalue_ref.push_back(pm.method_node->params[pi]->type.is_rvalue_ref);
              e0.param_is_lvalue_ref.push_back(pm.method_node->params[pi]->type.is_lvalue_ref);
            }
            break;
          }
        }
        ovset.push_back(std::move(e0));
      }
      RefOverloadEntry e1;
      e1.mangled_name = mangled;
      for (int pi = 0; pi < method->n_params; ++pi) {
        e1.param_is_rvalue_ref.push_back(method->params[pi]->type.is_rvalue_ref);
        e1.param_is_lvalue_ref.push_back(method->params[pi]->type.is_lvalue_ref);
      }
      ovset.push_back(std::move(e1));
    }
    struct_methods_[key] = mangled;
    struct_method_ret_types_[key] = method->type;
    pending_methods_.push_back({mangled, std::string(tag), method,
                                method_tpl_bindings, method_nttp_bindings});
  }
}

#if 0
void Lowerer::resolve_typedef_to_struct(TypeSpec& ts) const {
  if (ts.base != TB_TYPEDEF || !ts.tag) return;
  auto sit = module_->struct_defs.find(ts.tag);
  if (sit != module_->struct_defs.end()) {
    ts.base = TB_STRUCT;
    ts.tag = sit->second.tag.c_str();
  }
}

FunctionId Lowerer::next_fn_id() { return module_->alloc_function_id(); }

GlobalId Lowerer::next_global_id() { return module_->alloc_global_id(); }

LocalId Lowerer::next_local_id() { return module_->alloc_local_id(); }

BlockId Lowerer::next_block_id() { return module_->alloc_block_id(); }

ExprId Lowerer::next_expr_id() { return module_->alloc_expr_id(); }

bool Lowerer::contains_stmt_expr(const Node* n) {
  if (!n) return false;
  if (n->kind == NK_STMT_EXPR) return true;
  if (contains_stmt_expr(n->left)) return true;
  if (contains_stmt_expr(n->right)) return true;
  if (contains_stmt_expr(n->cond)) return true;
  if (contains_stmt_expr(n->then_)) return true;
  if (contains_stmt_expr(n->else_)) return true;
  if (contains_stmt_expr(n->body)) return true;
  if (contains_stmt_expr(n->init)) return true;
  if (contains_stmt_expr(n->update)) return true;
  for (int i = 0; i < n->n_children; ++i)
    if (contains_stmt_expr(n->children[i])) return true;
  return false;
}
#endif


#if 0
QualType Lowerer::qtype_from(const TypeSpec& t, ValueCategory c) {
  QualType qt{};
  qt.spec = t;
  qt.category = c;
  return qt;
}

std::optional<FnPtrSig> Lowerer::fn_ptr_sig_from_decl_node(const Node* n) {
  if (!n) return std::nullopt;

  // Canonical path: extract fn_ptr sig from sema's resolved type table.
  if (resolved_types_) {
    auto ct = resolved_types_->lookup(n);
    if (ct && sema::is_callable_type(*ct)) {
      const auto* fsig = sema::get_function_sig(*ct);
      if (fsig) {
        FnPtrSig sig{};
        sig.canonical_sig = ct;
        // Fix nested fn_ptr return types using parser-side declarator details.
        if (fsig->return_type) {
          TypeSpec ret_ts = sema::typespec_from_canonical(*fsig->return_type);
          if ((n->n_ret_fn_ptr_params > 0 || n->ret_fn_ptr_variadic) &&
              !ret_ts.is_fn_ptr) {
            ret_ts.is_fn_ptr = true;
            ret_ts.ptr_level = std::max(ret_ts.ptr_level, 1);
          }
          sig.return_type = qtype_from(ret_ts);
        }
        sig.variadic = fsig->is_variadic;
        sig.unspecified_params = fsig->unspecified_params;
        for (const auto& param : fsig->params) {
          sig.params.push_back(qtype_from(sema::typespec_from_canonical(param),
                                          ValueCategory::LValue));
        }
        if (sig.params.empty() && (n->n_fn_ptr_params > 0 || n->fn_ptr_variadic)) {
          sig.variadic = n->fn_ptr_variadic;
          sig.unspecified_params = false;
          for (int i = 0; i < n->n_fn_ptr_params; ++i) {
            const Node* param = n->fn_ptr_params[i];
            sig.params.push_back(qtype_from(param->type, ValueCategory::LValue));
          }
        }
        return sig;
      }
    }
  }

  // All fn_ptr declarations should already be represented in ResolvedTypeTable.
  return std::nullopt;
}

long long Lowerer::eval_const_int_with_nttp_bindings(
    const Node* n, const NttpBindings& nttp_bindings) const {
  if (!n) return 0;
  if (n->kind == NK_INT_LIT || n->kind == NK_CHAR_LIT) return n->ival;
  if (n->kind == NK_VAR && n->name) {
    auto nttp_it = nttp_bindings.find(n->name);
    if (nttp_it != nttp_bindings.end()) return nttp_it->second;
    auto enum_it = enum_consts_.find(n->name);
    if (enum_it != enum_consts_.end()) return enum_it->second;
    return 0;
  }
  if (n->kind == NK_CAST && n->left) {
    long long v = eval_const_int_with_nttp_bindings(n->left, nttp_bindings);
    TypeSpec ts = n->type;
    if (ts.ptr_level == 0) {
      int bits = 0;
      switch (ts.base) {
        case TB_BOOL: bits = 1; break;
        case TB_CHAR:
        case TB_UCHAR:
        case TB_SCHAR: bits = 8; break;
        case TB_SHORT:
        case TB_USHORT: bits = 16; break;
        case TB_INT:
        case TB_UINT:
        case TB_ENUM: bits = 32; break;
        default: break;
      }
      if (bits > 0 && bits < 64) {
        long long mask = (1LL << bits) - 1;
        v &= mask;
        if (!is_unsigned_base(ts.base) && ts.base != TB_BOOL && (v >> (bits - 1)))
          v |= ~mask;
      }
    }
    return v;
  }
  if (n->kind == NK_UNARY && n->op && n->left) {
    if (strcmp(n->op, "-") == 0)
      return -eval_const_int_with_nttp_bindings(n->left, nttp_bindings);
    if (strcmp(n->op, "+") == 0)
      return eval_const_int_with_nttp_bindings(n->left, nttp_bindings);
    if (strcmp(n->op, "~") == 0)
      return ~eval_const_int_with_nttp_bindings(n->left, nttp_bindings);
  }
  if (n->kind == NK_BINOP && n->op && n->left && n->right) {
    long long l = eval_const_int_with_nttp_bindings(n->left, nttp_bindings);
    long long r = eval_const_int_with_nttp_bindings(n->right, nttp_bindings);
    if (strcmp(n->op, "+") == 0) return l + r;
    if (strcmp(n->op, "-") == 0) return l - r;
    if (strcmp(n->op, "*") == 0) return l * r;
    if (strcmp(n->op, "/") == 0 && r != 0) return l / r;
    if (strcmp(n->op, "%") == 0 && r != 0) return l % r;
    if (strcmp(n->op, "&") == 0) return l & r;
    if (strcmp(n->op, "|") == 0) return l | r;
    if (strcmp(n->op, "^") == 0) return l ^ r;
    if (strcmp(n->op, "<<") == 0) return l << (r & 63);
    if (strcmp(n->op, ">>") == 0) return l >> (r & 63);
    if (strcmp(n->op, "<") == 0) return l < r;
    if (strcmp(n->op, ">") == 0) return l > r;
    if (strcmp(n->op, "<=") == 0) return l <= r;
    if (strcmp(n->op, ">=") == 0) return l >= r;
    if (strcmp(n->op, "==") == 0) return l == r;
    if (strcmp(n->op, "!=") == 0) return l != r;
    if (strcmp(n->op, "&&") == 0) return (l != 0) && (r != 0);
    if (strcmp(n->op, "||") == 0) return (l != 0) || (r != 0);
  }
  return 0;
}

std::string Lowerer::pack_binding_name(const std::string& base, int index) {
  return base + "#" + std::to_string(index);
}

bool Lowerer::parse_pack_binding_name(const std::string& key,
                                      const std::string& base,
                                      int* out_index) {
  if (key.size() <= base.size() + 1) return false;
  if (key.compare(0, base.size(), base) != 0) return false;
  if (key[base.size()] != '#') return false;
  int index = 0;
  try {
    index = std::stoi(key.substr(base.size() + 1));
  } catch (...) {
    return false;
  }
  if (index < 0) return false;
  if (out_index) *out_index = index;
  return true;
}

long long Lowerer::count_pack_bindings_for_name(const TypeBindings& bindings,
                                                const NttpBindings& nttp_bindings,
                                                const std::string& base) {
  int max_index = -1;
  for (const auto& [key, _] : bindings) {
    int pack_index = 0;
    if (parse_pack_binding_name(key, base, &pack_index))
      max_index = std::max(max_index, pack_index);
  }
  for (const auto& [key, _] : nttp_bindings) {
    int pack_index = 0;
    if (parse_pack_binding_name(key, base, &pack_index))
      max_index = std::max(max_index, pack_index);
  }
  return max_index + 1;
}

bool Lowerer::is_any_ref_ts(const TypeSpec& ts) {
  return ts.is_lvalue_ref || ts.is_rvalue_ref;
}

TypeSpec Lowerer::reference_storage_ts(TypeSpec ts) {
  if (ts.is_lvalue_ref || ts.is_rvalue_ref) ts.ptr_level += 1;
  return ts;
}

TypeSpec Lowerer::reference_value_ts(TypeSpec ts) {
  if (!ts.is_lvalue_ref && !ts.is_rvalue_ref) return ts;
  ts.is_lvalue_ref = false;
  ts.is_rvalue_ref = false;
  if (ts.ptr_level > 0) ts.ptr_level -= 1;
  return ts;
}

#endif
TypeSpec Lowerer::infer_generic_ctrl_type(FunctionCtx* ctx, const Node* n) {
  if (!n) return {};
  if (has_concrete_type(n->type)) return n->type;
  switch (n->kind) {
    case NK_INT_LIT:
      return infer_int_literal_type(n);
    case NK_FLOAT_LIT: {
      TypeSpec ts = n->type;
      if (!has_concrete_type(ts)) ts = classify_float_literal_type(const_cast<Node*>(n));
      return ts;
    }
    case NK_CHAR_LIT: {
      TypeSpec ts = n->type;
      if (!has_concrete_type(ts)) ts.base = TB_INT;
      return ts;
    }
    case NK_STR_LIT: {
      TypeSpec ts{};
      ts.base = TB_CHAR;
      ts.ptr_level = 1;
      return ts;
    }
    case NK_VAR: {
      const std::string name = n->name ? n->name : "";
      // Non-type template parameter: infer as int.
      if (ctx && !name.empty()) {
        auto nttp_it = ctx->nttp_bindings.find(name);
        if (nttp_it != ctx->nttp_bindings.end()) {
          TypeSpec ts{};
          ts.base = TB_INT;
          ts.array_size = -1;
          ts.inner_rank = -1;
          return ts;
        }
      }
      if (n->name && n->name[0] &&
          n->has_template_args && find_template_struct_primary(n->name)) {
        std::string arg_refs;
        for (int i = 0; i < n->n_template_args; ++i) {
          if (!arg_refs.empty()) arg_refs += ",";
          if (n->template_arg_is_value && n->template_arg_is_value[i]) {
            const char* fwd_name = n->template_arg_nttp_names ?
                n->template_arg_nttp_names[i] : nullptr;
            if (fwd_name && fwd_name[0]) arg_refs += fwd_name;
            else arg_refs += std::to_string(n->template_arg_values[i]);
          } else if (n->template_arg_types) {
            const TypeSpec& arg_ts = n->template_arg_types[i];
            arg_refs += encode_template_type_arg_ref_hir(arg_ts);
          }
        }
        TypeSpec tmp_ts{};
        tmp_ts.base = TB_STRUCT;
        tmp_ts.array_size = -1;
        tmp_ts.inner_rank = -1;
        tmp_ts.tpl_struct_origin = n->name;
        tmp_ts.tpl_struct_arg_refs = ::strdup(arg_refs.c_str());
        const Node* primary_tpl = find_template_struct_primary(n->name);
        TypeBindings tpl_empty;
        NttpBindings nttp_empty;
        seed_and_resolve_pending_template_type_if_needed(
            tmp_ts,
            ctx ? ctx->tpl_bindings : tpl_empty,
            ctx ? ctx->nttp_bindings : nttp_empty,
            n, PendingTemplateTypeKind::OwnerStruct, "generic-ctrl-type-var",
            primary_tpl);
        if (tmp_ts.tag && module_->struct_defs.count(tmp_ts.tag)) return tmp_ts;
      }
      if (ctx) {
        auto lit = ctx->locals.find(name);
        if (lit != ctx->locals.end()) {
          auto tt = ctx->local_types.find(lit->second.value);
          if (tt != ctx->local_types.end()) return reference_value_ts(tt->second);
        }
        auto pit = ctx->params.find(name);
        if (pit != ctx->params.end() && ctx->fn &&
            pit->second < ctx->fn->params.size())
          return reference_value_ts(ctx->fn->params[pit->second].type.spec);
        auto sit = ctx->static_globals.find(name);
        if (sit != ctx->static_globals.end()) {
          if (const GlobalVar* gv = module_->find_global(sit->second))
            return reference_value_ts(gv->type.spec);
        }
      }
      auto git = module_->global_index.find(name);
      if (git != module_->global_index.end()) {
        if (const GlobalVar* gv = module_->find_global(git->second))
          return reference_value_ts(gv->type.spec);
      }
      auto fit = module_->fn_index.find(name);
      if (fit != module_->fn_index.end()) {
        if (const Function* fn = module_->find_function(fit->second)) {
          TypeSpec ts = fn->return_type.spec;
          ts.is_fn_ptr = true;  // bare function designator
          ts.ptr_level = 0;
          ts.array_rank = 0;
          ts.array_size = -1;
          return ts;
        }
      }
      return n->type;
    }
    case NK_ADDR: {
      TypeSpec ts = infer_generic_ctrl_type(ctx, n->left);
      if (ts.array_rank > 0 && !is_vector_ty(ts)) {
        ts.array_rank = 0;
        ts.array_size = -1;
      }
      ts.ptr_level += 1;
      return ts;
    }
    case NK_DEREF: {
      TypeSpec ts = infer_generic_ctrl_type(ctx, n->left);
      if (ts.ptr_level == 0 && ts.base == TB_STRUCT && ts.tag) {
        // Check for operator_deref method - return its return type.
        // Try both non-const and const variants.
        std::string base_key = std::string(ts.tag) + "::operator_deref";
        auto rit = struct_method_ret_types_.find(base_key);
        if (rit == struct_method_ret_types_.end())
          rit = struct_method_ret_types_.find(base_key + "_const");
        if (rit != struct_method_ret_types_.end())
          return rit->second;
      }
      if (ts.ptr_level > 0) ts.ptr_level -= 1;
      else if (ts.array_rank > 0) ts.array_rank -= 1;
      return ts;
    }
    case NK_MEMBER: {
      TypeSpec base_ts = infer_generic_ctrl_type(ctx, n->left);
      if (n->is_arrow && base_ts.ptr_level > 0) base_ts.ptr_level -= 1;
      if (base_ts.tag) {
        auto it = module_->struct_defs.find(base_ts.tag);
        if (it != module_->struct_defs.end()) {
          for (const auto& f : it->second.fields) {
            if (f.name == (n->name ? n->name : "")) return field_type_of(f);
          }
        }
      }
      return n->type;
    }
    case NK_INDEX: {
      TypeSpec ts = infer_generic_ctrl_type(ctx, n->left);
      if (ts.ptr_level > 0) ts.ptr_level -= 1;
      else if (is_vector_ty(ts)) return vector_element_type(ts);
      else if (ts.array_rank > 0) {
        ts.array_rank -= 1;
        ts.array_size = (ts.array_rank > 0) ? ts.array_dims[0] : -1;
      }
      return ts;
    }
    case NK_CAST:
    case NK_COMPOUND_LIT: {
      TypeSpec ts = n->type;
      if (ctx && !ctx->tpl_bindings.empty() &&
          ts.base == TB_TYPEDEF && ts.tag) {
        auto it = ctx->tpl_bindings.find(ts.tag);
        if (it != ctx->tpl_bindings.end()) {
          ts.base = it->second.base;
          ts.tag = it->second.tag;
        }
      }
      if (ctx && !ctx->tpl_bindings.empty() && ts.tpl_struct_origin) {
        seed_and_resolve_pending_template_type_if_needed(
            ts, ctx->tpl_bindings, ctx->nttp_bindings, n,
            PendingTemplateTypeKind::DeclarationType, "generic-ctrl-type");
      }
      return ts;
    }
    case NK_BINOP: {
      const TypeSpec l = infer_generic_ctrl_type(ctx, n->left);
      const TypeSpec r = infer_generic_ctrl_type(ctx, n->right);
      if (is_vector_ty(l)) return l;
      if (is_vector_ty(r)) return r;
      const bool ptr_l = l.ptr_level > 0 || l.array_rank > 0;
      const bool ptr_r = r.ptr_level > 0 || r.array_rank > 0;
      if (n->op && n->op[0] && !n->op[1]) {
        const char op = n->op[0];
        if ((op == '+' || op == '-') && ptr_l && !ptr_r) return normalize_generic_type(l);
        if (op == '+' && ptr_r && !ptr_l) return normalize_generic_type(r);
      }
      if (l.base == TB_LONGLONG || l.base == TB_ULONGLONG ||
          r.base == TB_LONGLONG || r.base == TB_ULONGLONG) {
        TypeSpec ts{};
        ts.base = (l.base == TB_ULONGLONG || r.base == TB_ULONGLONG)
                      ? TB_ULONGLONG
                      : TB_LONGLONG;
        return ts;
      }
      if (l.base == TB_LONG || l.base == TB_ULONG || r.base == TB_LONG || r.base == TB_ULONG) {
        TypeSpec ts{};
        ts.base = (l.base == TB_ULONG || r.base == TB_ULONG) ? TB_ULONG : TB_LONG;
        return ts;
      }
      if (l.base == TB_DOUBLE || r.base == TB_DOUBLE) {
        TypeSpec ts{};
        ts.base = TB_DOUBLE;
        return ts;
      }
      if (l.base == TB_FLOAT || r.base == TB_FLOAT) {
        TypeSpec ts{};
        ts.base = TB_FLOAT;
        return ts;
      }
      TypeSpec ts{};
      ts.base = TB_INT;
      return ts;
    }
    case NK_CALL:
    case NK_BUILTIN_CALL: {
      if (n->left && n->left->kind == NK_VAR && n->left->name &&
          n->n_children == 0 && n->left->has_template_args &&
          find_template_struct_primary(n->left->name)) {
        TypeSpec callee_ts = infer_generic_ctrl_type(ctx, n->left);
        if (callee_ts.base == TB_STRUCT) return callee_ts;
      }
      if (n->left && n->left->kind == NK_VAR && n->left->name) {
        auto sit = module_->struct_defs.find(n->left->name);
        if (sit != module_->struct_defs.end()) {
          TypeSpec ts{};
          ts.base = TB_STRUCT;
          ts.tag = sit->second.tag.c_str();
          return ts;
        }
      }
      // Try to infer the return type of the called function.
      if (n->left && n->left->kind == NK_VAR && n->left->name) {
        const std::string callee_name = n->left->name;
        // Check deduced template call first.
        auto dit = deduced_template_calls_.find(n);
        if (dit != deduced_template_calls_.end()) {
          auto fit = module_->fn_index.find(dit->second.mangled_name);
          if (fit != module_->fn_index.end()) {
            const Function* fn = module_->find_function(fit->second);
            if (fn) return reference_value_ts(fn->return_type.spec);
          }
        }
        // Direct function lookup.
        auto fit = module_->fn_index.find(callee_name);
        if (fit != module_->fn_index.end()) {
          const Function* fn = module_->find_function(fit->second);
          if (fn) return reference_value_ts(fn->return_type.spec);
        }
        // operator() on struct variable: look up operator_call return type.
        TypeSpec callee_ts = infer_generic_ctrl_type(ctx, n->left);
        if (callee_ts.base == TB_STRUCT && callee_ts.ptr_level == 0 && callee_ts.tag) {
          if (auto rit = find_struct_method_return_type(callee_ts.tag, "operator_call",
                                                        callee_ts.is_const))
            return reference_value_ts(*rit);
        }
      }
      break;
    }
    default:
      break;
  }
  return n->type;
}

#if 0
std::optional<std::string> Lowerer::find_struct_method_mangled(
    const std::string& tag,
    const std::string& method,
    bool is_const_obj) const {
  const std::string base_key = tag + "::" + method;
  const std::string const_key = base_key + "_const";
  auto try_local = [&]() -> std::optional<std::string> {
    auto it = is_const_obj ? struct_methods_.find(const_key)
                           : struct_methods_.find(base_key);
    if (it != struct_methods_.end()) return it->second;
    it = is_const_obj ? struct_methods_.find(base_key)
                      : struct_methods_.find(const_key);
    if (it != struct_methods_.end()) return it->second;
    return std::nullopt;
  };
  if (auto local = try_local()) return local;
  auto dit = module_->struct_defs.find(tag);
  if (dit != module_->struct_defs.end()) {
    for (const auto& base_tag : dit->second.base_tags) {
      if (auto inherited =
              find_struct_method_mangled(base_tag, method, is_const_obj)) {
        return inherited;
      }
    }
  }
  return std::nullopt;
}

std::optional<TypeSpec> Lowerer::find_struct_method_return_type(
    const std::string& tag,
    const std::string& method,
    bool is_const_obj) const {
  const std::string base_key = tag + "::" + method;
  const std::string const_key = base_key + "_const";
  auto try_local = [&]() -> std::optional<TypeSpec> {
    auto it = is_const_obj ? struct_method_ret_types_.find(const_key)
                           : struct_method_ret_types_.find(base_key);
    if (it != struct_method_ret_types_.end()) return it->second;
    it = is_const_obj ? struct_method_ret_types_.find(base_key)
                      : struct_method_ret_types_.find(const_key);
    if (it != struct_method_ret_types_.end()) return it->second;
    return std::nullopt;
  };
  if (auto local = try_local()) return local;
  auto dit = module_->struct_defs.find(tag);
  if (dit != module_->struct_defs.end()) {
    for (const auto& base_tag : dit->second.base_tags) {
      if (auto inherited =
              find_struct_method_return_type(base_tag, method, is_const_obj)) {
        return inherited;
      }
    }
  }
  return std::nullopt;
}

std::optional<TypeSpec> Lowerer::infer_call_result_type_from_callee(
    FunctionCtx* ctx, const Node* callee) {
  if (!callee) return std::nullopt;
  if (callee->kind == NK_DEREF) {
    return infer_call_result_type_from_callee(ctx, callee->left);
  }
  if (callee->kind != NK_VAR || !callee->name || !callee->name[0]) {
    return std::nullopt;
  }
  const std::string name = callee->name;
  if (ctx) {
    const auto pit = ctx->param_fn_ptr_sigs.find(name);
    if (pit != ctx->param_fn_ptr_sigs.end()) return pit->second.return_type.spec;
    const auto lit = ctx->local_fn_ptr_sigs.find(name);
    if (lit != ctx->local_fn_ptr_sigs.end()) return lit->second.return_type.spec;
    const auto sit = ctx->static_globals.find(name);
    if (sit != ctx->static_globals.end()) {
      if (const GlobalVar* gv = module_->find_global(sit->second)) {
        if (gv->fn_ptr_sig) return gv->fn_ptr_sig->return_type.spec;
      }
    }
  }
  const auto git = module_->global_index.find(name);
  if (git != module_->global_index.end()) {
    if (const GlobalVar* gv = module_->find_global(git->second)) {
      if (gv->fn_ptr_sig) return gv->fn_ptr_sig->return_type.spec;
    }
  }
  const auto fit = module_->fn_index.find(name);
  if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
    return module_->functions[fit->second.value].return_type.spec;
  }
  return std::nullopt;
}

std::optional<TypeSpec> Lowerer::storage_type_for_declref(
    FunctionCtx* ctx, const DeclRef& r) {
  if (r.local && ctx) {
    auto it = ctx->local_types.find(r.local->value);
    if (it != ctx->local_types.end()) return it->second;
  }
  if (r.param_index && ctx && ctx->fn && *r.param_index < ctx->fn->params.size()) {
    return ctx->fn->params[*r.param_index].type.spec;
  }
  if (r.global) {
    if (const GlobalVar* gv = module_->find_global(*r.global)) return gv->type.spec;
  }
  return std::nullopt;
}
#endif

bool Lowerer::template_struct_has_pack_params(const Node* primary_tpl) {
  if (!primary_tpl || !primary_tpl->template_param_is_pack) return false;
  for (int pi = 0; pi < primary_tpl->n_template_params; ++pi) {
    if (primary_tpl->template_param_is_pack[pi]) return true;
  }
  return false;
}

Block& Lowerer::ensure_block(FunctionCtx& ctx, BlockId id) {
  for (auto& b : ctx.fn->blocks) {
    if (b.id.value == id.value) return b;
  }
  ctx.fn->blocks.push_back(Block{id, {}, false});
  return ctx.fn->blocks.back();
}

BlockId Lowerer::create_block(FunctionCtx& ctx) {
  const BlockId id = next_block_id();
  ctx.fn->blocks.push_back(Block{id, {}, false});
  return id;
}

void Lowerer::append_stmt(FunctionCtx& ctx, Stmt stmt) {
  Block& b = ensure_block(ctx, ctx.current_block);
  b.stmts.push_back(std::move(stmt));
}

ExprId Lowerer::append_expr(const Node* src,
                            ExprPayload payload,
                            const TypeSpec& ts,
                            ValueCategory c) {
  Expr e{};
  e.id = next_expr_id();
  e.span = make_span(src);
  e.type = qtype_from(ts, c);
  e.payload = std::move(payload);
  module_->expr_pool.push_back(std::move(e));
  return module_->expr_pool.back().id;
}


#if 0
GlobalId Lowerer::lower_static_local_global(FunctionCtx& ctx, const Node* n) {
  GlobalVar g{};
  g.id = next_global_id();
  g.name = "__static_local_" + sanitize_symbol(ctx.fn->name) + "_" + std::to_string(g.id.value);
  g.type = qtype_from(n->type, ValueCategory::LValue);
  g.fn_ptr_sig = fn_ptr_sig_from_decl_node(n);
  g.linkage = {true, false, false};  // internal linkage
  g.is_const = n->type.is_const;
  g.span = make_span(n);
  if (n->init) {
    g.init = lower_global_init(n->init, &ctx);
    g.type.spec = resolve_array_ts(g.type.spec, g.init);
    g.init = normalize_global_init(g.type.spec, g.init);
  }
  module_->global_index[g.name] = g.id;
  module_->globals.push_back(std::move(g));
  return g.id;
}

GlobalInit Lowerer::lower_global_init(const Node* n,
                                      FunctionCtx* ctx,
                                      bool allow_named_const_fold) {
  if (!n) return std::monostate{};
  if (n->kind == NK_INIT_LIST) {
    return lower_init_list(n, ctx);
  }
  // Compound literal at top level: extract its init list.
  if (n->kind == NK_COMPOUND_LIT && n->left && n->left->kind == NK_INIT_LIST) {
    return lower_init_list(n->left, ctx);
  }
  InitScalar s{};
  if (!ctx && allow_named_const_fold) {
    ConstEvalEnv env{&enum_consts_, &const_int_bindings_};
    if (auto r = evaluate_constant_expr(n, env); r.ok()) {
      TypeSpec ts = n->type;
      if (ts.base == TB_VOID) ts.base = TB_INT;
      s.expr = append_expr(n, IntLiteral{r.as_int(), false}, ts);
      return s;
    }
  }
  s.expr = lower_expr(ctx, n);
  return s;
}

InitList Lowerer::lower_init_list(const Node* n, FunctionCtx* ctx) {
  InitList out{};
  for (int i = 0; i < n->n_children; ++i) {
    const Node* it = n->children[i];
    if (!it) continue;
    InitListItem item{};

    const Node* value_node = it;
    if (it->kind == NK_INIT_ITEM) {
      if (it->is_designated) {
        if (it->is_index_desig) {
          item.index_designator = it->desig_val;
        } else if (it->desig_field) {
          item.field_designator = std::string(it->desig_field);
        }
      }
      value_node = it->left ? it->left : it->right;
      if (!value_node) value_node = it->init;
    }

    if (value_node && value_node->kind == NK_INIT_LIST) {
      item.value = std::make_shared<InitList>(lower_init_list(value_node, ctx));
    } else if (value_node && value_node->kind == NK_COMPOUND_LIT &&
               value_node->left && value_node->left->kind == NK_INIT_LIST) {
      // Compound literal with init list: treat as a sub-list so the emitter
      // can use field-by-field init (not brace elision from the parent).
      item.value = std::make_shared<InitList>(lower_init_list(value_node->left, ctx));
    } else if (!ctx && value_node && value_node->kind == NK_ADDR &&
               value_node->left && value_node->left->kind == NK_COMPOUND_LIT) {
      // Global scope: &(T){...} inside an init list — hoist the compound
      // literal to an anonymous global and use its address.
      InitScalar s{};
      s.expr = hoist_compound_literal_to_global(value_node, value_node->left);
      item.value = s;
    } else {
      InitScalar s{};
      s.expr = lower_expr(ctx, value_node);
      item.value = s;
    }
    out.items.push_back(std::move(item));
  }
  return out;
}

const Node* Lowerer::init_item_value_node(const Node* item) const {
  if (!item) return nullptr;
  if (item->kind != NK_INIT_ITEM) return item;
  const Node* v = item->left ? item->left : item->right;
  if (!v) v = item->init;
  if (!v && item->n_children > 0) {
    for (int i = 0; i < item->n_children; ++i) {
      if (item->children && item->children[i]) {
        v = item->children[i];
        break;
      }
    }
  }
  return v;
}

const Node* Lowerer::unwrap_init_scalar_value(const Node* node) const {
  const Node* cur = node;
  for (int guard = 0; guard < 32 && cur; ++guard) {
    if (cur->kind == NK_INIT_ITEM) {
      const Node* next = init_item_value_node(cur);
      if (!next || next == cur) break;
      cur = next;
      continue;
    }
    if (cur->kind == NK_INIT_LIST) {
      const Node* first = nullptr;
      for (int i = 0; i < cur->n_children; ++i) {
        if (cur->children && cur->children[i]) {
          first = cur->children[i];
          break;
        }
      }
      if (!first) break;
      const Node* next = init_item_value_node(first);
      if (!next || next == cur) break;
      cur = next;
      continue;
    }
    break;
  }
  return cur;
}

bool Lowerer::has_side_effect_expr(const Node* n) const {
  if (!n) return false;
  switch (n->kind) {
    case NK_CALL:
    case NK_BUILTIN_CALL:
    case NK_ASSIGN:
    case NK_COMPOUND_ASSIGN:
    case NK_COMMA_EXPR:
      return true;
    case NK_POSTFIX: {
      const char* op = n->op ? n->op : "";
      if (std::strcmp(op, "++") == 0 || std::strcmp(op, "--") == 0) return true;
      break;
    }
    case NK_UNARY: {
      const char* op = n->op ? n->op : "";
      if (std::strcmp(op, "++pre") == 0 || std::strcmp(op, "--pre") == 0) return true;
      break;
    }
    default:
      break;
  }
  if (has_side_effect_expr(n->left)) return true;
  if (has_side_effect_expr(n->right)) return true;
  if (has_side_effect_expr(n->cond)) return true;
  if (has_side_effect_expr(n->then_)) return true;
  if (has_side_effect_expr(n->else_)) return true;
  if (has_side_effect_expr(n->init)) return true;
  if (has_side_effect_expr(n->update)) return true;
  if (has_side_effect_expr(n->body)) return true;
  for (int i = 0; i < n->n_children; ++i) {
    if (has_side_effect_expr(n->children ? n->children[i] : nullptr)) return true;
  }
  return false;
}

bool Lowerer::is_simple_constant_expr(const Node* n) const {
  if (!n) return false;
  switch (n->kind) {
    case NK_INT_LIT:
    case NK_FLOAT_LIT:
    case NK_CHAR_LIT:
      return true;
    case NK_CAST:
      return is_simple_constant_expr(n->left);
    case NK_UNARY: {
      const char* op = n->op ? n->op : "";
      if (std::strcmp(op, "+") == 0 || std::strcmp(op, "-") == 0 ||
          std::strcmp(op, "~") == 0) {
        return is_simple_constant_expr(n->left);
      }
      return false;
    }
    default:
      return false;
  }
}

bool Lowerer::can_fast_path_scalar_array_init(const Node* init_list) const {
  if (!init_list || init_list->kind != NK_INIT_LIST) return false;
  for (int i = 0; i < init_list->n_children; ++i) {
    const Node* item = init_list->children ? init_list->children[i] : nullptr;
    if (!item) continue;
    if (item->kind == NK_INIT_ITEM && item->is_designated) return false;
    const Node* v = init_item_value_node(item);
    if (!v) return false;
    if (v->kind == NK_INIT_LIST || v->kind == NK_COMPOUND_LIT) return false;
    if (has_side_effect_expr(v)) return false;
    if (!is_simple_constant_expr(v)) return false;
  }
  return true;
}

bool Lowerer::struct_has_member_dtors(const std::string& tag) {
  auto sit = module_->struct_defs.find(tag);
  if (sit == module_->struct_defs.end()) return false;
  for (auto it = sit->second.fields.rbegin(); it != sit->second.fields.rend(); ++it) {
    if (it->elem_type.base == TB_STRUCT && it->elem_type.ptr_level == 0 &&
        it->elem_type.tag) {
      std::string ftag = it->elem_type.tag;
      if (struct_destructors_.count(ftag) || struct_has_member_dtors(ftag)) return true;
    }
  }
  return false;
}
#endif

void Lowerer::emit_defaulted_method_body(FunctionCtx& ctx,
                                         Function& fn,
                                         const std::string& struct_tag,
                                         const Node* method_node) {
  auto sit = module_->struct_defs.find(struct_tag);
  bool is_copy_or_move_ctor = method_node->is_constructor && method_node->n_params == 1;
  bool is_copy_or_move_assign =
      (method_node->operator_kind == OP_ASSIGN) && method_node->n_params == 1;

  if (is_copy_or_move_ctor || is_copy_or_move_assign) {
    if (sit != module_->struct_defs.end()) {
      DeclRef this_ref{};
      this_ref.name = "this";
      auto pit = ctx.params.find("this");
      if (pit != ctx.params.end()) this_ref.param_index = pit->second;
      TypeSpec this_ts{};
      this_ts.base = TB_STRUCT;
      this_ts.tag = sit->second.tag.c_str();
      this_ts.ptr_level = 1;
      ExprId this_id = append_expr(method_node, this_ref, this_ts, ValueCategory::LValue);

      std::string other_name =
          method_node->params[0]->name ? method_node->params[0]->name : "<anon_param>";
      DeclRef other_ref{};
      other_ref.name = other_name;
      auto opit = ctx.params.find(other_name);
      if (opit != ctx.params.end()) other_ref.param_index = opit->second;
      TypeSpec other_ts = this_ts;
      ExprId other_id =
          append_expr(method_node, other_ref, other_ts, ValueCategory::LValue);

      for (const auto& field : sit->second.fields) {
        TypeSpec field_ts = field.elem_type;
        if (field.array_first_dim >= 0) {
          field_ts.array_rank = 1;
          field_ts.array_size = field.array_first_dim;
        }

        MemberExpr lhs_me{};
        lhs_me.base = this_id;
        lhs_me.field = field.name;
        lhs_me.is_arrow = true;
        ExprId lhs_member =
            append_expr(method_node, lhs_me, field_ts, ValueCategory::LValue);

        MemberExpr rhs_me{};
        rhs_me.base = other_id;
        rhs_me.field = field.name;
        rhs_me.is_arrow = true;
        ExprId rhs_member =
            append_expr(method_node, rhs_me, field_ts, ValueCategory::LValue);

        AssignExpr ae{};
        ae.lhs = lhs_member;
        ae.rhs = rhs_member;
        ExprId assign_id = append_expr(method_node, ae, field_ts);
        ExprStmt es{};
        es.expr = assign_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
      }
    }
  }

  if (is_copy_or_move_assign) {
    DeclRef this_ref2{};
    this_ref2.name = "this";
    auto pit2 = ctx.params.find("this");
    if (pit2 != ctx.params.end()) this_ref2.param_index = pit2->second;
    TypeSpec this_ts2{};
    this_ts2.base = TB_STRUCT;
    this_ts2.tag =
        sit != module_->struct_defs.end() ? sit->second.tag.c_str() : struct_tag.c_str();
    this_ts2.ptr_level = 1;
    ExprId this_ret =
        append_expr(method_node, this_ref2, this_ts2, ValueCategory::LValue);
    ReturnStmt rs{};
    rs.expr = this_ret;
    append_stmt(ctx, Stmt{StmtPayload{rs}, make_span(method_node)});
  } else if (!method_node->is_destructor) {
    ReturnStmt rs{};
    append_stmt(ctx, Stmt{StmtPayload{rs}, make_span(method_node)});
  }
}

void Lowerer::emit_member_dtor_calls(FunctionCtx& ctx,
                                     const std::string& struct_tag,
                                     ExprId this_ptr_id,
                                     const Node* span_node) {
  auto sit = module_->struct_defs.find(struct_tag);
  if (sit == module_->struct_defs.end()) return;
  const auto& fields = sit->second.fields;
  for (auto it = fields.rbegin(); it != fields.rend(); ++it) {
    const auto& field = *it;
    if (field.elem_type.base != TB_STRUCT || field.elem_type.ptr_level != 0 ||
        !field.elem_type.tag) {
      continue;
    }
    std::string ftag = field.elem_type.tag;
    bool has_explicit_dtor = struct_destructors_.count(ftag) > 0;
    bool has_member_dtors = struct_has_member_dtors(ftag);
    if (!has_explicit_dtor && !has_member_dtors) continue;

    MemberExpr me{};
    me.base = this_ptr_id;
    me.field = field.name;
    me.is_arrow = true;
    TypeSpec field_ts = field.elem_type;
    ExprId member_id = append_expr(span_node, me, field_ts, ValueCategory::LValue);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = member_id;
    TypeSpec ptr_ts = field_ts;
    ptr_ts.ptr_level++;
    ExprId member_ptr_id = append_expr(span_node, addr, ptr_ts);

    if (has_explicit_dtor) {
      auto dit = struct_destructors_.find(ftag);
      CallExpr c{};
      DeclRef callee_ref{};
      callee_ref.name = dit->second.mangled_name;
      TypeSpec fn_ts{};
      fn_ts.base = TB_VOID;
      TypeSpec callee_ts = fn_ts;
      callee_ts.ptr_level++;
      c.callee = append_expr(span_node, callee_ref, callee_ts);
      c.args.push_back(member_ptr_id);
      ExprId call_id = append_expr(span_node, c, fn_ts);
      ExprStmt es{};
      es.expr = call_id;
      append_stmt(ctx, Stmt{StmtPayload{es}, make_span(span_node)});
    } else {
      emit_member_dtor_calls(ctx, ftag, member_ptr_id, span_node);
    }
  }
}

void Lowerer::emit_dtor_calls(FunctionCtx& ctx, size_t since, const Node* span_node) {
  for (size_t i = ctx.dtor_stack.size(); i > since; --i) {
    const auto& dl = ctx.dtor_stack[i - 1];
    auto dit = struct_destructors_.find(dl.struct_tag);

    DeclRef var_ref{};
    var_ref.local = dl.local_id;
    auto lt = ctx.local_types.find(dl.local_id.value);
    TypeSpec var_ts{};
    if (lt != ctx.local_types.end()) var_ts = lt->second;
    ExprId var_id = append_expr(span_node, var_ref, var_ts, ValueCategory::LValue);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = var_id;
    TypeSpec ptr_ts = var_ts;
    ptr_ts.ptr_level++;
    ExprId addr_id = append_expr(span_node, addr, ptr_ts);

    if (dit != struct_destructors_.end()) {
      CallExpr c{};
      DeclRef callee_ref{};
      callee_ref.name = dit->second.mangled_name;
      TypeSpec fn_ts{};
      fn_ts.base = TB_VOID;
      TypeSpec callee_ts = fn_ts;
      callee_ts.ptr_level++;
      c.callee = append_expr(span_node, callee_ref, callee_ts);
      c.args.push_back(addr_id);
      ExprId call_id = append_expr(span_node, c, fn_ts);
      ExprStmt es{};
      es.expr = call_id;
      append_stmt(ctx, Stmt{StmtPayload{es}, make_span(span_node)});
    } else {
      emit_member_dtor_calls(ctx, dl.struct_tag, addr_id, span_node);
    }
  }
}

void Lowerer::lower_global(const Node* gv) {
  GlobalInit computed_init{};
  bool has_init = false;

  // Handle compound literal at global scope: `T *p = &(T){...};`
  // The compound literal must be lowered to a separate static global, and
  // the parent global initialized to point at it.
  if (gv->init && gv->init->kind == NK_ADDR &&
      gv->init->left && gv->init->left->kind == NK_COMPOUND_LIT) {
    ExprId addr_id = hoist_compound_literal_to_global(gv->init, gv->init->left);
    InitScalar sc{};
    sc.expr = addr_id;
    computed_init = sc;
    has_init = true;
  }

  // For init lists that may contain nested &(compound_lit) items, lower the
  // init BEFORE allocating this global's id so that compound-literal globals
  // are created first (their DeclRef exprs need valid GlobalIds).
  GlobalInit early_init{};
  bool early_init_done = false;
  if (!has_init && gv->init) {
    early_init = lower_global_init(gv->init, nullptr, gv->type.is_const || gv->is_constexpr);
    early_init_done = true;
  }

  GlobalVar g{};
  g.id = next_global_id();
  g.name = gv->name ? gv->name : "<anon_global>";
  g.ns_qual = make_ns_qual(gv);
  {
    TypeBindings empty_tpl_bindings;
    NttpBindings empty_nttp_bindings;
    TypeSpec global_ts = gv->type;
    seed_and_resolve_pending_template_type_if_needed(
        global_ts, empty_tpl_bindings, empty_nttp_bindings, gv,
        PendingTemplateTypeKind::DeclarationType,
        std::string("global-decl:") + g.name);
    resolve_typedef_to_struct(global_ts);
    g.type = qtype_from(global_ts, ValueCategory::LValue);
  }
  g.fn_ptr_sig = fn_ptr_sig_from_decl_node(gv);
  g.linkage = {gv->is_static, gv->is_extern, false, weak_symbols_.count(g.name) > 0,
                static_cast<Visibility>(gv->visibility)};
  g.is_const = gv->type.is_const;
  g.span = make_span(gv);

  // Deduce unsized array dimension from wide string literal initializer
  if (gv->init && g.type.spec.array_rank > 0 && g.type.spec.array_size < 0 &&
      gv->init->kind == NK_STR_LIT && gv->init->sval && gv->init->sval[0] == 'L') {
    const auto vals = decode_string_literal_values(gv->init->sval, true);
    g.type.spec.array_size = static_cast<long long>(vals.size());
    g.type.spec.array_dims[0] = g.type.spec.array_size;
  }

  if (has_init) {
    g.init = computed_init;
  } else if (early_init_done) {
    g.init = early_init;
    g.type.spec = resolve_array_ts(g.type.spec, g.init);
    g.init = normalize_global_init(g.type.spec, g.init);
  }

  if (g.is_const) {
    if (const auto* scalar = std::get_if<InitScalar>(&g.init)) {
      const Expr& e = module_->expr_pool[scalar->expr.value];
      if (const auto* lit = std::get_if<IntLiteral>(&e.payload)) {
        const_int_bindings_[g.name] = lit->value;
        ct_state_->register_const_int_binding(g.name, lit->value);
      } else if (const auto* ch = std::get_if<CharLiteral>(&e.payload)) {
        const_int_bindings_[g.name] = ch->value;
        ct_state_->register_const_int_binding(g.name, ch->value);
      }
    }
  }

  module_->global_index[g.name] = g.id;
  module_->globals.push_back(std::move(g));
}

void Lowerer::lower_local_decl_stmt(FunctionCtx& ctx, const Node* n) {
  // Local function prototype (e.g. `int f1(char *);` inside a function body):
  // if the name is already registered as a known function, skip creating a
  // local alloca; later references will resolve directly to the global function.
  // Require n_params > 0 to distinguish from a plain variable declaration
  // whose name coincidentally matches a function name.
  if (n->name && !n->init && n->n_params > 0 && module_->fn_index.count(n->name)) return;

  // Local extern declaration: `extern T v;` inside a function refers to
  // the global with that name (C99 6.2.2p4). Erase any shadowing local
  // binding so the global is found by the NK_VAR resolution fallback.
  if (n->is_extern && !n->init && n->name && n->name[0]) {
    const auto git = module_->global_index.find(n->name);
    if (git != module_->global_index.end()) {
      ctx.locals.erase(n->name);
      ctx.static_globals[n->name] = git->second;
    }
    return;
  }

  if (n->is_static) {
    if (n->name && n->name[0]) {
      ctx.static_globals[n->name] = lower_static_local_global(ctx, n);
    }
    return;
  }

  LocalDecl d{};
  d.id = next_local_id();
  d.name = n->name ? n->name : "<anon_local>";
  // Substitute template type parameters in local variable types.
  {
    TypeSpec decl_ts = n->type;
    if (ctx.tpl_bindings.size() && decl_ts.base == TB_TYPEDEF && decl_ts.tag) {
      auto it = ctx.tpl_bindings.find(decl_ts.tag);
      if (it != ctx.tpl_bindings.end()) {
        const TypeSpec& concrete = it->second;
        decl_ts.base = concrete.base;
        decl_ts.tag = concrete.tag;
      }
    }
    // Resolve pending template struct types in local variable decls.
    seed_and_resolve_pending_template_type_if_needed(
        decl_ts, ctx.tpl_bindings, ctx.nttp_bindings, n,
        PendingTemplateTypeKind::DeclarationType,
        std::string("local-decl:") + d.name);
    resolve_typedef_to_struct(decl_ts);
    d.type = qtype_from(reference_storage_ts(decl_ts), ValueCategory::LValue);
  }
  d.fn_ptr_sig = fn_ptr_sig_from_decl_node(n);
  if (d.fn_ptr_sig) ctx.local_fn_ptr_sigs[d.name] = *d.fn_ptr_sig;
  // Deduce unsized array dimension from initializer list
  if (n->init && d.type.spec.array_rank > 0 && d.type.spec.array_size < 0) {
    if (n->init->kind == NK_INIT_LIST) {
      long long count = n->init->n_children;
      for (int ci = 0; ci < n->init->n_children; ++ci) {
        const Node* item = n->init->children[ci];
        if (item && item->kind == NK_INIT_ITEM && item->is_designated &&
            item->is_index_desig && item->desig_val + 1 > count)
          count = item->desig_val + 1;
      }
      d.type.spec.array_size = count;
      d.type.spec.array_dims[0] = count;
    } else if (n->init->kind == NK_STR_LIT && n->init->sval) {
      const bool is_wide = n->init->sval[0] == 'L';
      const auto vals = decode_string_literal_values(n->init->sval, is_wide);
      d.type.spec.array_size = static_cast<long long>(vals.size());
      d.type.spec.array_dims[0] = d.type.spec.array_size;
    }
  }
  // VLA: only if size is still unknown after deduction
  if (n->type.array_rank > 0 && n->type.array_size_expr &&
      (d.type.spec.array_size <= 0 || d.type.spec.array_dims[0] <= 0)) {
    d.vla_size = lower_expr(&ctx, n->type.array_size_expr);
  }
  d.storage = n->is_static ? StorageClass::Static : StorageClass::Auto;
  d.span = make_span(n);
  const bool is_array_with_init_list =
      n->init && n->init->kind == NK_INIT_LIST &&
      (d.type.spec.array_rank > 0 || d.type.spec.is_vector);
  const bool is_array_with_string_init =
      n->init && n->init->kind == NK_STR_LIT && d.type.spec.array_rank == 1;
  const bool is_struct_with_init_list =
      n->init && n->init->kind == NK_INIT_LIST &&
      (d.type.spec.base == TB_STRUCT || d.type.spec.base == TB_UNION) &&
      d.type.spec.ptr_level == 0 && d.type.spec.array_rank == 0;
  const bool use_array_init_fast_path =
      is_array_with_init_list && !d.type.spec.is_vector &&
      can_fast_path_scalar_array_init(n->init);
  // C++ copy initialization: T var = expr; where T has a copy/move constructor.
  // Detect this early so we can skip setting d.init (the ctor call is emitted
  // after the decl, similar to the is_ctor_init path).
  bool is_struct_copy_init = false;
  if (n->init && !n->is_ctor_init && n->init->kind != NK_INIT_LIST &&
      d.type.spec.base == TB_STRUCT && d.type.spec.ptr_level == 0 &&
      d.type.spec.array_rank == 0 && d.type.spec.tag &&
      !is_lvalue_ref_ts(n->type) && !n->type.is_rvalue_ref) {
    auto cit = struct_constructors_.find(d.type.spec.tag);
    if (cit != struct_constructors_.end()) {
      // Check if any constructor takes a single param of same struct type (copy/move ctor).
      for (const auto& ov : cit->second) {
        if (ov.method_node->n_params == 1) {
          TypeSpec param_ts = ov.method_node->params[0]->type;
          resolve_typedef_to_struct(param_ts);
          if (param_ts.tag && strcmp(param_ts.tag, d.type.spec.tag) == 0 &&
              (param_ts.is_lvalue_ref || param_ts.is_rvalue_ref)) {
            is_struct_copy_init = true;
            break;
          }
        }
      }
    }
  }
  if (is_lvalue_ref_ts(n->type) && n->init) {
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = lower_expr(&ctx, n->init);
    d.init = append_expr(n->init, addr, d.type.spec);
  } else if (n->type.is_rvalue_ref && n->init) {
    // Rvalue reference: materialize the rvalue into a temporary, then
    // store a pointer to that temporary as the reference value.
    ExprId init_val = lower_expr(&ctx, n->init);
    TypeSpec val_ts = reference_value_ts(n->type);
    // Create a hidden temporary local for the rvalue
    LocalDecl tmp{};
    tmp.id = next_local_id();
    tmp.name = ("__rref_tmp_" + std::to_string(d.id.value)).c_str();
    tmp.type = qtype_from(val_ts, ValueCategory::LValue);
    tmp.init = init_val;
    const LocalId tmp_lid = tmp.id;
    ctx.locals[tmp.name] = tmp.id;
    ctx.local_types[tmp.id.value] = val_ts;
    append_stmt(ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
    // Take address of temporary
    DeclRef tmp_ref{};
    tmp_ref.name = ("__rref_tmp_" + std::to_string(d.id.value)).c_str();
    tmp_ref.local = tmp_lid;
    ExprId var_id = append_expr(n->init, tmp_ref, val_ts, ValueCategory::LValue);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = var_id;
    d.init = append_expr(n->init, addr, d.type.spec);
  } else if (!is_array_with_init_list && !is_array_with_string_init &&
             !is_struct_with_init_list && !is_struct_copy_init && n->init)
    d.init = lower_expr(&ctx, n->init);
  // For aggregate init lists / string array init, emit zeroinitializer first
  // then overlay explicit element/field assignments below.
  if (is_struct_with_init_list || is_array_with_init_list || is_array_with_string_init) {
    TypeSpec int_ts{};
    int_ts.base = TB_INT;
    d.init = append_expr(n, IntLiteral{0, false}, int_ts);
  }
  const LocalId lid = d.id;
  const TypeSpec decl_ts = d.type.spec;
  ctx.locals[d.name] = d.id;
  ctx.local_types[d.id.value] = d.type.spec;
  // Track const/constexpr locals with foldable int initializers.
  if (n->name && n->name[0] && n->init &&
      (n->type.is_const || n->is_constexpr) &&
      !n->type.is_lvalue_ref && !n->type.is_rvalue_ref &&
      n->type.ptr_level == 0 && n->type.array_rank == 0) {
    ConstEvalEnv cenv{&enum_consts_, &const_int_bindings_, &ctx.local_const_bindings};
    if (auto cr = evaluate_constant_expr(n->init, cenv); cr.ok()) {
      ctx.local_const_bindings[n->name] = cr.as_int();
    }
  }
  append_stmt(ctx, Stmt{StmtPayload{std::move(d)}, make_span(n)});

  // C++ constructor invocation: Type var(args) -> call StructTag__StructTag(&var, args...)
  if (n->is_ctor_init && decl_ts.tag) {
    auto cit = struct_constructors_.find(decl_ts.tag);
    if (cit != struct_constructors_.end() && !cit->second.empty()) {
      // Resolve best constructor overload by matching argument types.
      const CtorOverload* best = nullptr;
      if (cit->second.size() == 1) {
        best = &cit->second[0];
      } else {
        // Score each constructor overload.
        int best_score = -1;
        for (const auto& ov : cit->second) {
          if (ov.method_node->n_params != n->n_children) continue;
          int score = 0;
          bool viable = true;
          for (int pi = 0; pi < ov.method_node->n_params && viable; ++pi) {
            TypeSpec param_ts = ov.method_node->params[pi]->type;
            resolve_typedef_to_struct(param_ts);
            TypeSpec arg_ts = infer_generic_ctrl_type(&ctx, n->children[pi]);
            bool arg_is_lvalue = is_ast_lvalue(n->children[pi]);
            // Strip ref qualifiers for base type comparison.
            TypeSpec param_base = param_ts;
            param_base.is_lvalue_ref = false;
            param_base.is_rvalue_ref = false;
            if (param_base.base == arg_ts.base &&
                param_base.ptr_level == arg_ts.ptr_level) {
              score += 2;  // base type match
              // Ref-qualifier scoring: prefer T&& for rvalues, const T& for lvalues.
              if (param_ts.is_rvalue_ref && !arg_is_lvalue) {
                score += 4;  // rvalue ref matches rvalue arg
              } else if (param_ts.is_rvalue_ref && arg_is_lvalue) {
                viable = false;  // T&& cannot bind lvalue
              } else if (param_ts.is_lvalue_ref && arg_is_lvalue) {
                score += 3;  // lvalue ref matches lvalue arg
              } else if (param_ts.is_lvalue_ref && !arg_is_lvalue) {
                score += 1;  // const T& can bind rvalue (lower priority)
              }
            } else if (param_base.ptr_level == 0 && arg_ts.ptr_level == 0 &&
                       param_base.base != TB_STRUCT && arg_ts.base != TB_STRUCT &&
                       param_base.base != TB_VOID && arg_ts.base != TB_VOID) {
              score += 1;  // implicit scalar conversion
            } else {
              viable = false;
            }
          }
          if (viable && score > best_score) {
            best_score = score;
            best = &ov;
          }
        }
      }
      if (best) {
        if (best->method_node->is_deleted) {
          std::string diag = "error: call to deleted constructor '";
          diag += decl_ts.tag;
          diag += "'";
          throw std::runtime_error(diag);
        }
        CallExpr c{};
        DeclRef callee_ref{};
        callee_ref.name = best->mangled_name;
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        c.callee = append_expr(n, callee_ref, callee_ts);
        // First arg: &var (this pointer).
        DeclRef var_ref{};
        var_ref.name = n->name ? n->name : "<anon_local>";
        var_ref.local = lid;
        ExprId var_id = append_expr(n, var_ref, decl_ts, ValueCategory::LValue);
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = var_id;
        TypeSpec ptr_ts = decl_ts;
        ptr_ts.ptr_level++;
        c.args.push_back(append_expr(n, addr, ptr_ts));
        // Explicit args; handle reference params.
        for (int i = 0; i < n->n_children; ++i) {
          TypeSpec param_ts = best->method_node->params[i]->type;
          resolve_typedef_to_struct(param_ts);
          if (param_ts.is_rvalue_ref || param_ts.is_lvalue_ref) {
            // Reference parameter: pass address of the argument.
            const Node* arg = n->children[i];
            const Node* inner = arg;
            // Unwrap static_cast<T&&>(x) to get x.
            if (inner->kind == NK_CAST && inner->left) inner = inner->left;
            // Check if the argument is a function call returning a reference type.
            // In that case, the call already returns a pointer; no AddrOf needed.
            bool arg_returns_ref = false;
            if (inner->kind == NK_CALL && inner->left &&
                inner->left->kind == NK_VAR && inner->left->name) {
              TypeSpec call_ret = infer_generic_ctrl_type(&ctx, arg);
              (void)call_ret;
              // infer_generic_ctrl_type strips refs, so check original return type.
              auto dit = deduced_template_calls_.find(inner);
              if (dit != deduced_template_calls_.end()) {
                auto fit = module_->fn_index.find(dit->second.mangled_name);
                if (fit != module_->fn_index.end()) {
                  const Function* fn = module_->find_function(fit->second);
                  if (fn && is_any_ref_ts(fn->return_type.spec)) arg_returns_ref = true;
                }
              }
              if (!arg_returns_ref) {
                auto fit = module_->fn_index.find(inner->left->name);
                if (fit != module_->fn_index.end()) {
                  const Function* fn = module_->find_function(fit->second);
                  if (fn && is_any_ref_ts(fn->return_type.spec)) arg_returns_ref = true;
                }
              }
            }
            if (arg_returns_ref) {
              // Call already returns a pointer (reference ABI).
              c.args.push_back(lower_expr(&ctx, arg));
            } else {
              ExprId arg_val = lower_expr(&ctx, inner);
              TypeSpec storage_ts = reference_storage_ts(param_ts);
              UnaryExpr addr_e{};
              addr_e.op = UnaryOp::AddrOf;
              addr_e.operand = arg_val;
              c.args.push_back(append_expr(arg, addr_e, storage_ts));
            }
          } else {
            c.args.push_back(lower_expr(&ctx, n->children[i]));
          }
        }
        ExprId call_id = append_expr(n, c, fn_ts);
        ExprStmt es{};
        es.expr = call_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
      }
    }
  }

  // C++ implicit default constructor: T var; where T has a zero-arg ctor.
  if (!n->is_ctor_init && !n->init && decl_ts.tag &&
      decl_ts.base == TB_STRUCT && decl_ts.ptr_level == 0 &&
      decl_ts.array_rank == 0) {
    auto cit = struct_constructors_.find(decl_ts.tag);
    if (cit != struct_constructors_.end()) {
      // Find a zero-arg constructor.
      const CtorOverload* default_ctor = nullptr;
      for (const auto& ov : cit->second) {
        if (ov.method_node->n_params == 0) {
          default_ctor = &ov;
          break;
        }
      }
      if (default_ctor) {
        if (default_ctor->method_node->is_deleted) {
          std::string diag = "error: call to deleted default constructor '";
          diag += decl_ts.tag;
          diag += "'";
          throw std::runtime_error(diag);
        }
        CallExpr c{};
        DeclRef callee_ref{};
        callee_ref.name = default_ctor->mangled_name;
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        c.callee = append_expr(n, callee_ref, callee_ts);
        // First arg: &var (this pointer).
        DeclRef var_ref{};
        var_ref.name = n->name ? n->name : "<anon_local>";
        var_ref.local = lid;
        ExprId var_id = append_expr(n, var_ref, decl_ts, ValueCategory::LValue);
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = var_id;
        TypeSpec ptr_ts = decl_ts;
        ptr_ts.ptr_level++;
        c.args.push_back(append_expr(n, addr, ptr_ts));
        ExprId call_id = append_expr(n, c, fn_ts);
        ExprStmt es{};
        es.expr = call_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
      }
    }
  }

  // C++ copy initialization: T var = expr; -> call copy/move constructor.
  if (is_struct_copy_init) {
    auto cit = struct_constructors_.find(decl_ts.tag);
    if (cit != struct_constructors_.end() && !cit->second.empty()) {
      // Score constructors: prefer T&& for rvalue, const T& for lvalue.
      bool init_is_lvalue = is_ast_lvalue(n->init);
      const CtorOverload* best = nullptr;
      int best_score = -1;
      for (const auto& ov : cit->second) {
        if (ov.method_node->n_params != 1) continue;
        TypeSpec param_ts = ov.method_node->params[0]->type;
        resolve_typedef_to_struct(param_ts);
        if (!param_ts.tag || strcmp(param_ts.tag, decl_ts.tag) != 0) continue;
        if (!param_ts.is_lvalue_ref && !param_ts.is_rvalue_ref) continue;
        int score = 0;
        if (param_ts.is_rvalue_ref && !init_is_lvalue) {
          score = 4;  // rvalue ref matches rvalue arg
        } else if (param_ts.is_rvalue_ref && init_is_lvalue) {
          continue;  // T&& cannot bind lvalue; skip
        } else if (param_ts.is_lvalue_ref && init_is_lvalue) {
          score = 3;  // const T& matches lvalue
        } else if (param_ts.is_lvalue_ref && !init_is_lvalue) {
          score = 1;  // const T& can bind rvalue (lower priority)
        }
        if (score > best_score) {
          best_score = score;
          best = &ov;
        }
      }
      if (best) {
        if (best->method_node->is_deleted) {
          std::string diag = "error: call to deleted constructor '";
          diag += decl_ts.tag;
          diag += "'";
          throw std::runtime_error(diag);
        }
        CallExpr c{};
        DeclRef callee_ref{};
        callee_ref.name = best->mangled_name;
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        c.callee = append_expr(n, callee_ref, callee_ts);
        // First arg: &var (this pointer).
        DeclRef var_ref{};
        var_ref.name = n->name ? n->name : "<anon_local>";
        var_ref.local = lid;
        ExprId var_id = append_expr(n, var_ref, decl_ts, ValueCategory::LValue);
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = var_id;
        TypeSpec ptr_ts = decl_ts;
        ptr_ts.ptr_level++;
        c.args.push_back(append_expr(n, addr, ptr_ts));
        // Second arg: the init expression (passed as &expr for ref param).
        TypeSpec param_ts = best->method_node->params[0]->type;
        resolve_typedef_to_struct(param_ts);
        if (param_ts.is_lvalue_ref || param_ts.is_rvalue_ref) {
          // Reference param: pass address of init expression.
          ExprId init_val = lower_expr(&ctx, n->init);
          UnaryExpr addr_e{};
          addr_e.op = UnaryOp::AddrOf;
          addr_e.operand = init_val;
          TypeSpec storage_ts = decl_ts;
          storage_ts.ptr_level++;
          c.args.push_back(append_expr(n->init, addr_e, storage_ts));
        } else {
          c.args.push_back(lower_expr(&ctx, n->init));
        }
        ExprId call_id = append_expr(n, c, fn_ts);
        ExprStmt es{};
        es.expr = call_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
      }
    }
  }

  // Track struct locals with destructors for implicit scope-exit calls.
  // Also track structs whose members have destructors (even without explicit dtor).
  if (decl_ts.tag && decl_ts.base == TB_STRUCT && decl_ts.ptr_level == 0 &&
      decl_ts.array_rank == 0 && !n->type.is_lvalue_ref && !n->type.is_rvalue_ref) {
    auto dit = struct_destructors_.find(decl_ts.tag);
    if (dit != struct_destructors_.end()) {
      if (dit->second.method_node->is_deleted) {
        std::string diag = "error: variable '";
        diag += n->name ? n->name : "<anon>";
        diag += "' of type '";
        diag += decl_ts.tag;
        diag += "' has a deleted destructor";
        throw std::runtime_error(diag);
      }
      ctx.dtor_stack.push_back({lid, std::string(decl_ts.tag)});
    } else if (struct_has_member_dtors(decl_ts.tag)) {
      ctx.dtor_stack.push_back({lid, std::string(decl_ts.tag)});
    }
  }

  auto emit_scalar_array_zero_fill = [&](const TypeSpec& array_ts) {
    if (array_ts.array_rank != 1 || array_ts.array_size <= 0) return;
    TypeSpec elem_ts = array_ts;
    elem_ts.array_rank--;
    elem_ts.array_size = -1;
    if ((elem_ts.base == TB_STRUCT || elem_ts.base == TB_UNION) &&
        elem_ts.ptr_level == 0 && elem_ts.array_rank == 0) {
      return;
    }
    for (long long idx = 0; idx < array_ts.array_size; ++idx) {
      DeclRef dr{};
      dr.name = n->name ? n->name : "<anon_local>";
      dr.local = lid;
      ExprId dr_id = append_expr(n, dr, array_ts, ValueCategory::LValue);
      TypeSpec idx_ts{};
      idx_ts.base = TB_INT;
      ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
      IndexExpr ie{};
      ie.base = dr_id;
      ie.index = idx_id;
      ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
      ExprId zero_id = append_expr(n, IntLiteral{0, false}, idx_ts);
      AssignExpr ae{};
      ae.lhs = ie_id;
      ae.rhs = zero_id;
      ExprId ae_id = append_expr(n, ae, elem_ts);
      ExprStmt es{};
      es.expr = ae_id;
      append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
    }
  };
  // For array init lists, emit element-by-element assignments.
  if (is_array_with_init_list && use_array_init_fast_path) {
    emit_scalar_array_zero_fill(decl_ts);
    long long next_idx = 0;
    for (int ci = 0; ci < n->init->n_children; ++ci) {
      const Node* item = n->init->children[ci];
      if (!item) {
        ++next_idx;
        continue;
      }
      long long idx = next_idx;
      const Node* val_node = item;
      if (item->kind == NK_INIT_ITEM) {
        if (item->is_designated && item->is_index_desig) idx = item->desig_val;
        val_node = init_item_value_node(item);
      }
      next_idx = idx + 1;
      if (!val_node) continue;
      // Build: x[idx] = val
      TypeSpec elem_ts = decl_ts;
      elem_ts.array_rank--;
      elem_ts.array_size = -1;
      // DeclRef to the local
      DeclRef dr{};
      dr.name = n->name ? n->name : "<anon_local>";
      dr.local = lid;
      ExprId dr_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);
      // IntLiteral for index
      TypeSpec idx_ts{};
      idx_ts.base = TB_INT;
      ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
      // IndexExpr
      IndexExpr ie{};
      ie.base = dr_id;
      ie.index = idx_id;
      ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
      const bool elem_is_agg =
          (elem_ts.base == TB_STRUCT || elem_ts.base == TB_UNION) &&
          elem_ts.ptr_level == 0 && elem_ts.array_rank == 0;
      if (elem_is_agg && elem_ts.tag) {
        const Node* scalar_node = val_node;
        if (scalar_node && scalar_node->kind == NK_COMPOUND_LIT) {
          const Node* init_list =
              (scalar_node->left && scalar_node->left->kind == NK_INIT_LIST)
                  ? scalar_node->left
                  : nullptr;
          if (!init_list && scalar_node->n_children > 0) {
            const Node* c0 = scalar_node->children[0];
            if (c0 && c0->kind == NK_INIT_ITEM) {
              const Node* vv = c0->left ? c0->left : c0->right;
              if (!vv) vv = c0->init;
              if (vv) c0 = vv;
            }
            if (c0) scalar_node = c0;
          } else if (init_list && init_list->n_children > 0) {
            const Node* c0 = init_list->children[0];
            if (c0 && c0->kind == NK_INIT_ITEM) {
              const Node* vv = c0->left ? c0->left : c0->right;
              if (!vv) vv = c0->init;
              if (vv) c0 = vv;
            }
            if (c0) scalar_node = c0;
          }
        }
        scalar_node = unwrap_init_scalar_value(scalar_node);
        bool direct_agg = false;
        if (scalar_node) {
          const TypeSpec st = infer_generic_ctrl_type(&ctx, scalar_node);
          if (st.ptr_level == 0 && st.array_rank == 0 &&
              st.base == elem_ts.base) {
            const char* et = elem_ts.tag ? elem_ts.tag : "";
            const char* stg = st.tag ? st.tag : "";
            direct_agg = std::strcmp(et, stg) == 0;
          }
        }
        if (!direct_agg) {
          const auto sit = module_->struct_defs.find(elem_ts.tag);
          if (sit != module_->struct_defs.end() && !sit->second.fields.empty() &&
              scalar_node) {
            const HirStructField& fld = sit->second.fields[0];
            TypeSpec field_ts = field_type_of(fld);
            MemberExpr me{};
            me.base = ie_id;
            me.field = fld.name;
            me.is_arrow = false;
            ExprId me_id = append_expr(n, me, field_ts, ValueCategory::LValue);
            ExprId val_id = lower_expr(&ctx, scalar_node);
            AssignExpr ae{};
            ae.lhs = me_id;
            ae.rhs = val_id;
            ExprId ae_id = append_expr(n, ae, field_ts);
            ExprStmt es{};
            es.expr = ae_id;
            append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
            continue;
          }
        }
      }

      ExprId val_id = lower_expr(&ctx, val_node);
      AssignExpr ae{};
      ae.lhs = ie_id;
      ae.rhs = val_id;
      ExprId ae_id = append_expr(n, ae, elem_ts);
      ExprStmt es{};
      es.expr = ae_id;
      append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
    }
  }
  if (is_array_with_string_init && n->init && n->init->sval) {
    emit_scalar_array_zero_fill(decl_ts);
    const bool is_wide = n->init->sval[0] == 'L';
    const auto vals = decode_string_literal_values(n->init->sval, is_wide);
    const long long max_count =
        decl_ts.array_size > 0 ? decl_ts.array_size : static_cast<long long>(vals.size());
    const long long emit_n =
        std::min<long long>(max_count, static_cast<long long>(vals.size()));
    for (long long idx = 0; idx < emit_n; ++idx) {
      TypeSpec elem_ts = decl_ts;
      elem_ts.array_rank--;
      elem_ts.array_size = -1;
      DeclRef dr{};
      dr.name = n->name ? n->name : "<anon_local>";
      dr.local = lid;
      ExprId dr_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);
      TypeSpec idx_ts{};
      idx_ts.base = TB_INT;
      ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
      IndexExpr ie{};
      ie.base = dr_id;
      ie.index = idx_id;
      ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
      ExprId val_id =
          append_expr(n, IntLiteral{vals[static_cast<size_t>(idx)], false}, idx_ts);
      AssignExpr ae{};
      ae.lhs = ie_id;
      ae.rhs = val_id;
      ExprId ae_id = append_expr(n, ae, elem_ts);
      ExprStmt es{};
      es.expr = ae_id;
      append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
    }
  }
  if (((is_struct_with_init_list && decl_ts.tag) ||
       (is_array_with_init_list && !use_array_init_fast_path)) &&
      n->init) {
    auto is_agg = [](const TypeSpec& ts) {
      return (ts.base == TB_STRUCT || ts.base == TB_UNION) &&
             ts.ptr_level == 0 && ts.array_rank == 0;
    };
    auto append_assign = [&](ExprId lhs_id, const TypeSpec& lhs_ts, const Node* rhs_node) {
      if (!rhs_node) return;
      if (lhs_ts.array_rank == 1 && rhs_node->kind == NK_STR_LIT && rhs_node->sval) {
        TypeSpec elem_ts = lhs_ts;
        elem_ts.array_rank = 0;
        elem_ts.array_size = -1;
        if (elem_ts.ptr_level == 0 &&
            (elem_ts.base == TB_CHAR || elem_ts.base == TB_SCHAR || elem_ts.base == TB_UCHAR)) {
          const bool is_wide = rhs_node->sval[0] == 'L';
          const auto vals = decode_string_literal_values(rhs_node->sval, is_wide);
          const long long max_count =
              lhs_ts.array_size > 0 ? lhs_ts.array_size : static_cast<long long>(vals.size());
          const long long emit_n =
              std::min<long long>(max_count, static_cast<long long>(vals.size()));
          for (long long idx = 0; idx < emit_n; ++idx) {
            TypeSpec idx_ts{};
            idx_ts.base = TB_INT;
            ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
            IndexExpr ie{};
            ie.base = lhs_id;
            ie.index = idx_id;
            ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
            ExprId val_id =
                append_expr(n, IntLiteral{vals[static_cast<size_t>(idx)], false}, idx_ts);
            AssignExpr ae{};
            ae.lhs = ie_id;
            ae.rhs = val_id;
            ExprId ae_id = append_expr(n, ae, elem_ts);
            ExprStmt es{};
            es.expr = ae_id;
            append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
          }
          return;
        }
      }
      const Node* val_node = unwrap_init_scalar_value(rhs_node);
      ExprId val_id = lower_expr(&ctx, val_node);
      AssignExpr ae{};
      ae.lhs = lhs_id;
      ae.rhs = val_id;
      ExprId ae_id = append_expr(n, ae, lhs_ts);
      ExprStmt es{};
      es.expr = ae_id;
      append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
    };
    auto can_direct_assign_agg = [&](const TypeSpec& lhs_ts,
                                     const Node* rhs_node) -> bool {
      if (!rhs_node) return false;
      if (!is_agg(lhs_ts)) return false;
      const TypeSpec rhs_ts = infer_generic_ctrl_type(&ctx, rhs_node);
      if (rhs_ts.ptr_level != 0 || rhs_ts.array_rank != 0) return false;
      if (rhs_ts.base != lhs_ts.base) return false;
      if (rhs_ts.base == TB_STRUCT || rhs_ts.base == TB_UNION || rhs_ts.base == TB_ENUM) {
        const char* lt = lhs_ts.tag ? lhs_ts.tag : "";
        const char* rt = rhs_ts.tag ? rhs_ts.tag : "";
        return std::strcmp(lt, rt) == 0;
      }
      return true;
    };
    auto is_direct_char_array_init = [&](const TypeSpec& lhs_ts,
                                         const Node* rhs_node) -> bool {
      if (!rhs_node) return false;
      if (lhs_ts.array_rank != 1 || lhs_ts.ptr_level != 0) return false;
      if (!(lhs_ts.base == TB_CHAR || lhs_ts.base == TB_SCHAR || lhs_ts.base == TB_UCHAR))
        return false;
      return rhs_node->kind == NK_STR_LIT;
    };

    std::function<void(const TypeSpec&, ExprId, const Node*, int&)> consume_from_list;
    consume_from_list =
        [&](const TypeSpec& cur_ts, ExprId cur_lhs, const Node* list_node, int& cursor) {
          if (!list_node || list_node->kind != NK_INIT_LIST) return;
          if (cursor < 0) cursor = 0;

          if (is_agg(cur_ts) && cur_ts.tag) {
            const auto sit = module_->struct_defs.find(cur_ts.tag);
            if (sit == module_->struct_defs.end()) return;
            const auto& sd = sit->second;
            size_t next_field = 0;
            while (cursor < list_node->n_children) {
              const Node* item = list_node->children[cursor];
              if (!item) {
                ++cursor;
                if (next_field < sd.fields.size()) ++next_field;
                continue;
              }
              const bool has_field_designator =
                  item->kind == NK_INIT_ITEM && item->is_designated &&
                  !item->is_index_desig && item->desig_field;
              if (!has_field_designator && next_field >= sd.fields.size()) break;
              size_t fi = next_field;
              if (has_field_designator) {
                for (size_t k = 0; k < sd.fields.size(); ++k) {
                  if (sd.fields[k].name == item->desig_field) {
                    fi = k;
                    break;
                  }
                }
              }
              if (fi >= sd.fields.size()) {
                ++cursor;
                continue;
              }

              const HirStructField& fld = sd.fields[fi];
              TypeSpec field_ts = field_type_of(fld);
              MemberExpr me{};
              me.base = cur_lhs;
              me.field = fld.name;
              me.is_arrow = false;
              ExprId me_id = append_expr(n, me, field_ts, ValueCategory::LValue);

              const Node* val_node = init_item_value_node(item);
              if (is_agg(field_ts) || field_ts.array_rank > 0) {
                if (val_node && val_node->kind == NK_INIT_LIST) {
                  int sub_cursor = 0;
                  consume_from_list(field_ts, me_id, val_node, sub_cursor);
                  ++cursor;
                } else if (is_direct_char_array_init(field_ts, val_node)) {
                  append_assign(me_id, field_ts, val_node);
                  ++cursor;
                } else if (can_direct_assign_agg(field_ts, val_node)) {
                  append_assign(me_id, field_ts, val_node);
                  ++cursor;
                } else {
                  consume_from_list(field_ts, me_id, list_node, cursor);
                }
              } else {
                if (val_node && val_node->kind == NK_INIT_LIST) {
                  int sub_cursor = 0;
                  consume_from_list(field_ts, me_id, val_node, sub_cursor);
                  ++cursor;
                } else {
                  append_assign(me_id, field_ts, val_node);
                  ++cursor;
                }
              }
              next_field = fi + 1;
            }
            return;
          }

          if (cur_ts.array_rank > 0 || is_vector_ty(cur_ts)) {
            TypeSpec elem_ts = cur_ts;
            long long bound = 0;
            if (is_vector_ty(cur_ts)) {
              elem_ts = vector_element_type(cur_ts);
              bound = cur_ts.vector_lanes > 0 ? cur_ts.vector_lanes : (1LL << 30);
            } else {
              elem_ts.array_rank--;
              elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
              bound = cur_ts.array_size > 0 ? cur_ts.array_size : (1LL << 30);
            }
            long long next_idx = 0;
            while (cursor < list_node->n_children && next_idx < bound) {
              const Node* item = list_node->children[cursor];
              if (!item) {
                ++cursor;
                ++next_idx;
                continue;
              }
              long long idx = next_idx;
              if (item->kind == NK_INIT_ITEM && item->is_designated &&
                  item->is_index_desig) {
                idx = item->desig_val;
              }
              TypeSpec idx_ts{};
              idx_ts.base = TB_INT;
              ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
              IndexExpr ie{};
              ie.base = cur_lhs;
              ie.index = idx_id;
              ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);

              const Node* val_node = init_item_value_node(item);
              if (is_agg(elem_ts) || elem_ts.array_rank > 0) {
                if (val_node && val_node->kind == NK_INIT_LIST) {
                  int sub_cursor = 0;
                  consume_from_list(elem_ts, ie_id, val_node, sub_cursor);
                  ++cursor;
                } else if (can_direct_assign_agg(elem_ts, val_node)) {
                  append_assign(ie_id, elem_ts, val_node);
                  ++cursor;
                } else {
                  consume_from_list(elem_ts, ie_id, list_node, cursor);
                }
              } else {
                if (val_node && val_node->kind == NK_INIT_LIST) {
                  int sub_cursor = 0;
                  consume_from_list(elem_ts, ie_id, val_node, sub_cursor);
                  ++cursor;
                } else {
                  append_assign(ie_id, elem_ts, val_node);
                  ++cursor;
                }
              }
              next_idx = idx + 1;
            }
            return;
          }

          if (cursor < list_node->n_children) {
            const Node* item = list_node->children[cursor];
            const Node* val_node = init_item_value_node(item);
            if (val_node && val_node->kind == NK_INIT_LIST) {
              int sub_cursor = 0;
              consume_from_list(cur_ts, cur_lhs, val_node, sub_cursor);
            } else {
              append_assign(cur_lhs, cur_ts, val_node);
            }
            ++cursor;
          }
        };

    DeclRef dr{};
    dr.name = n->name ? n->name : "<anon_local>";
    dr.local = lid;
    ExprId base_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);
    int cursor = 0;
    consume_from_list(decl_ts, base_id, n->init, cursor);
  }
}

#if 0
std::optional<ExprId> Lowerer::try_lower_consteval_call_expr(FunctionCtx* ctx,
                                                             const Node* n) {
  if (!(n->kind == NK_CALL && n->left && n->left->kind == NK_VAR && n->left->name))
    return std::nullopt;
  const Node* ce_fn_def = ct_state_->find_consteval_def(n->left->name);
  if (!ce_fn_def) return std::nullopt;

  ConstEvalEnv arg_env{&enum_consts_, &const_int_bindings_,
                       ctx ? &ctx->local_const_bindings : nullptr};
  TypeBindings tpl_bindings;
  NttpBindings ce_nttp_bindings;
  const Node* fn_def = ce_fn_def;
  if ((n->left->n_template_args > 0 || n->left->has_template_args) &&
      fn_def->n_template_params > 0) {
    int count = std::min(n->left->n_template_args, fn_def->n_template_params);
    for (int i = 0; i < count; ++i) {
      if (!fn_def->template_param_names[i]) continue;
      if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) {
        if (n->left->template_arg_is_value && n->left->template_arg_is_value[i]) {
          if (n->left->template_arg_nttp_names && n->left->template_arg_nttp_names[i] &&
              ctx && !ctx->nttp_bindings.empty()) {
            auto it = ctx->nttp_bindings.find(n->left->template_arg_nttp_names[i]);
            if (it != ctx->nttp_bindings.end()) {
              ce_nttp_bindings[fn_def->template_param_names[i]] = it->second;
            }
          } else {
            ce_nttp_bindings[fn_def->template_param_names[i]] =
                n->left->template_arg_values[i];
          }
        }
        continue;
      }
      TypeSpec arg_ts = n->left->template_arg_types[i];
      if (arg_ts.base == TB_TYPEDEF && arg_ts.tag && ctx && !ctx->tpl_bindings.empty()) {
        auto resolved = ctx->tpl_bindings.find(arg_ts.tag);
        if (resolved != ctx->tpl_bindings.end()) arg_ts = resolved->second;
      }
      tpl_bindings[fn_def->template_param_names[i]] = arg_ts;
    }
    if (fn_def->template_param_has_default) {
      for (int i = count; i < fn_def->n_template_params; ++i) {
        if (!fn_def->template_param_names[i]) continue;
        if (!fn_def->template_param_has_default[i]) continue;
        if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) {
          ce_nttp_bindings[fn_def->template_param_names[i]] =
              fn_def->template_param_default_values[i];
        } else {
          tpl_bindings[fn_def->template_param_names[i]] =
              fn_def->template_param_default_types[i];
        }
      }
    }
    arg_env.type_bindings = &tpl_bindings;
    if (!ce_nttp_bindings.empty()) arg_env.nttp_bindings = &ce_nttp_bindings;
  }
  std::vector<ConstValue> args;
  bool all_const = true;
  for (int i = 0; i < n->n_children; ++i) {
    auto r = evaluate_constant_expr(n->children[i], arg_env);
    if (r.ok()) {
      args.push_back(*r.value);
    } else {
      all_const = false;
      break;
    }
  }
  if (!all_const) {
    std::string diag = "error: call to consteval function '";
    diag += n->left->name;
    diag += "' with non-constant arguments";
    throw std::runtime_error(diag);
  }

  PendingConstevalExpr pce;
  pce.fn_name = n->left->name;
  for (const auto& cv : args) pce.const_args.push_back(cv.as_int());
  pce.tpl_bindings = tpl_bindings;
  pce.nttp_bindings = ce_nttp_bindings;
  pce.call_span = make_span(n);
  pce.unlocked_by_deferred_instantiation = lowering_deferred_instantiation_;
  TypeSpec ts = n->type;
  return append_expr(n, std::move(pce), ts);
}

std::string Lowerer::resolve_ref_overload(const std::string& base_name,
                                          const Node* call_node) {
  auto ovit = ref_overload_set_.find(base_name);
  if (ovit == ref_overload_set_.end()) return {};
  const auto& overloads = ovit->second;
  const std::string* best_name = nullptr;
  int best_score = -1;
  for (const auto& ov : overloads) {
    bool viable = true;
    int score = 0;
    for (int i = 0;
         i < call_node->n_children &&
         i < static_cast<int>(ov.param_is_rvalue_ref.size());
         ++i) {
      bool arg_is_lvalue = is_ast_lvalue(call_node->children[i]);
      if (ov.param_is_lvalue_ref[static_cast<size_t>(i)] && !arg_is_lvalue) {
        viable = false;
        break;
      }
      if (ov.param_is_rvalue_ref[static_cast<size_t>(i)] && arg_is_lvalue) {
        viable = false;
        break;
      }
      if (ov.param_is_rvalue_ref[static_cast<size_t>(i)] && !arg_is_lvalue) {
        score += 2;
      } else if (ov.param_is_lvalue_ref[static_cast<size_t>(i)] && arg_is_lvalue) {
        score += 2;
      } else {
        score += 1;
      }
    }
    if (viable && score > best_score) {
      best_name = &ov.mangled_name;
      best_score = score;
    }
  }
  return best_name ? *best_name : base_name;
}

const Node* Lowerer::find_pending_method_by_mangled(
    const std::string& mangled_name) const {
  for (const auto& pm : pending_methods_) {
    if (pm.mangled == mangled_name) return pm.method_node;
  }
  return nullptr;
}

bool Lowerer::describe_initializer_list_struct(const TypeSpec& ts,
                                               TypeSpec* elem_ts,
                                               TypeSpec* data_ptr_ts,
                                               TypeSpec* len_ts) const {
  if (ts.base != TB_STRUCT || ts.ptr_level != 0 || !ts.tag) return false;
  auto sit = module_->struct_defs.find(ts.tag);
  if (sit == module_->struct_defs.end()) return false;

  const HirStructField* data_field = nullptr;
  const HirStructField* len_field = nullptr;
  for (const auto& field : sit->second.fields) {
    if (field.name == "_M_array") {
      data_field = &field;
    } else if (field.name == "_M_len") {
      len_field = &field;
    }
  }
  if (!data_field || !len_field) return false;
  if (data_field->elem_type.ptr_level <= 0) return false;

  if (data_ptr_ts) *data_ptr_ts = data_field->elem_type;
  if (elem_ts) {
    *elem_ts = data_field->elem_type;
    elem_ts->ptr_level--;
  }
  if (len_ts) *len_ts = len_field->elem_type;
  return true;
}

ExprId Lowerer::materialize_initializer_list_arg(FunctionCtx* ctx,
                                                 const Node* list_node,
                                                 const TypeSpec& param_ts) {
  TypeSpec elem_ts{};
  TypeSpec data_ptr_ts{};
  TypeSpec len_ts{};
  if (!describe_initializer_list_struct(param_ts, &elem_ts, &data_ptr_ts, &len_ts)) {
    return append_expr(list_node, IntLiteral{0, false}, param_ts);
  }

  ExprId data_ptr_id{};
  if (list_node && list_node->n_children > 0) {
    TypeSpec array_ts = elem_ts;
    array_ts.array_rank = 1;
    array_ts.array_size = list_node->n_children;
    array_ts.array_dims[0] = list_node->n_children;
    for (int i = 1; i < 8; ++i) array_ts.array_dims[i] = -1;

    Node array_tmp{};
    array_tmp.kind = NK_COMPOUND_LIT;
    array_tmp.type = array_ts;
    array_tmp.left = const_cast<Node*>(list_node);
    array_tmp.line = list_node->line;

    ExprId array_id = lower_expr(ctx, &array_tmp);

    TypeSpec idx_ts{};
    idx_ts.base = TB_INT;
    ExprId zero_idx = append_expr(list_node, IntLiteral{0, false}, idx_ts);
    IndexExpr first_elem{};
    first_elem.base = array_id;
    first_elem.index = zero_idx;
    ExprId first_elem_id =
        append_expr(list_node, first_elem, elem_ts, ValueCategory::LValue);

    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = first_elem_id;
    data_ptr_id = append_expr(list_node, addr, data_ptr_ts);
  } else {
    data_ptr_id = append_expr(list_node, IntLiteral{0, false}, data_ptr_ts);
  }

  LocalDecl tmp{};
  tmp.id = next_local_id();
  tmp.name = "__init_list_arg_" + std::to_string(tmp.id.value);
  tmp.type = qtype_from(param_ts, ValueCategory::LValue);
  tmp.storage = StorageClass::Auto;
  tmp.span = make_span(list_node);
  const std::string tmp_name = tmp.name;
  TypeSpec int_ts{};
  int_ts.base = TB_INT;
  tmp.init = append_expr(list_node, IntLiteral{0, false}, int_ts);
  const LocalId tmp_lid = tmp.id;
  ctx->locals[tmp_name] = tmp.id;
  ctx->local_types[tmp.id.value] = param_ts;
  append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(list_node)});

  DeclRef tmp_ref{};
  tmp_ref.name = tmp_name;
  tmp_ref.local = tmp_lid;
  ExprId tmp_id = append_expr(list_node, tmp_ref, param_ts, ValueCategory::LValue);

  auto assign_field = [&](const char* field_name, const TypeSpec& field_ts, ExprId rhs_id) {
    MemberExpr lhs{};
    lhs.base = tmp_id;
    lhs.field = field_name;
    lhs.is_arrow = false;
    ExprId lhs_id = append_expr(list_node, lhs, field_ts, ValueCategory::LValue);
    AssignExpr assign{};
    assign.op = AssignOp::Set;
    assign.lhs = lhs_id;
    assign.rhs = rhs_id;
    ExprId assign_id = append_expr(list_node, assign, field_ts);
    append_stmt(*ctx, Stmt{StmtPayload{ExprStmt{assign_id}}, make_span(list_node)});
  };

  assign_field("_M_array", data_ptr_ts, data_ptr_id);
  ExprId len_id = append_expr(
      list_node, IntLiteral{list_node ? list_node->n_children : 0, false}, len_ts);
  assign_field("_M_len", len_ts, len_id);
  return tmp_id;
}
#endif

#if 0
bool Lowerer::is_string_scalar(const GlobalInit& init) const {
  const auto* scalar = std::get_if<InitScalar>(&init);
  if (!scalar) return false;
  const Expr& e = module_->expr_pool[scalar->expr.value];
  return std::holds_alternative<StringLiteral>(e.payload);
}

long long Lowerer::flat_scalar_count(const TypeSpec& ts) const {
  if (is_vector_ty(ts)) return ts.vector_lanes > 0 ? ts.vector_lanes : 1;
  if (ts.array_rank > 0) {
    if (ts.array_size <= 0) return 1;
    TypeSpec elem_ts = ts;
    elem_ts.array_rank--;
    elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
    return ts.array_size * flat_scalar_count(elem_ts);
  }
  if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0 && ts.tag) {
    const auto it = module_->struct_defs.find(ts.tag);
    if (it == module_->struct_defs.end()) return 1;
    const auto& sd = it->second;
    if (sd.fields.empty()) return 1;
    if (sd.is_union) return flat_scalar_count(field_type_of(sd.fields.front()));
    long long count = 0;
    for (const auto& f : sd.fields) count += flat_scalar_count(field_type_of(f));
    return count > 0 ? count : 1;
  }
  return 1;
}

long long Lowerer::deduce_array_size_from_init(const GlobalInit& init) const {
  if (const auto* list = std::get_if<InitList>(&init)) {
    long long max_idx = -1;
    long long next = 0;
    for (const auto& item : list->items) {
      long long idx = next;
      if (item.index_designator && *item.index_designator >= 0) idx = *item.index_designator;
      if (idx > max_idx) max_idx = idx;
      next = idx + 1;
    }
    return max_idx + 1;
  }
  if (is_string_scalar(init)) {
    const auto& scalar = std::get<InitScalar>(init);
    const Expr& e = module_->expr_pool[scalar.expr.value];
    const auto& sl = std::get<StringLiteral>(e.payload);
    if (sl.is_wide) {
      return static_cast<long long>(decode_string_literal_values(sl.raw.c_str(), true).size());
    }
    return static_cast<long long>(bytes_from_string_literal(sl).size()) + 1;
  }
  return -1;
}

TypeSpec Lowerer::resolve_array_ts(const TypeSpec& ts, const GlobalInit& init) const {
  if (ts.array_rank <= 0 || ts.array_size >= 0) return ts;
  long long deduced = deduce_array_size_from_init(init);
  if (deduced < 0) return ts;
  TypeSpec elem_ts = ts;
  elem_ts.array_rank--;
  elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
  const bool elem_is_agg = is_vector_ty(elem_ts) ||
                           elem_ts.array_rank > 0 ||
                           ((elem_ts.base == TB_STRUCT || elem_ts.base == TB_UNION) &&
                            elem_ts.ptr_level == 0);
  if (elem_is_agg) {
    if (const auto* list = std::get_if<InitList>(&init)) {
      bool all_scalar = true;
      for (const auto& item : list->items) {
        if (!std::holds_alternative<InitScalar>(item.value)) {
          all_scalar = false;
          break;
        }
      }
      if (all_scalar) {
        const long long fc = flat_scalar_count(elem_ts);
        if (fc > 1) deduced = (deduced + fc - 1) / fc;
      }
    }
  }
  TypeSpec out = ts;
  out.array_size = deduced;
  out.array_dims[0] = deduced;
  return out;
}

bool Lowerer::is_direct_char_array_init(const TypeSpec& ts, const GlobalInit& init) const {
  if (ts.array_rank != 1 || ts.ptr_level != 0) return false;
  if (!is_char_like(ts.base)) return false;
  return is_string_scalar(init);
}

bool Lowerer::union_allows_init_normalization(const TypeSpec& ts) const {
  if (ts.base != TB_UNION || ts.ptr_level != 0 || !ts.tag || !ts.tag[0]) return false;
  const auto it = module_->struct_defs.find(ts.tag);
  if (it == module_->struct_defs.end()) return false;
  const auto& sd = it->second;
  if (!sd.is_union || sd.fields.empty()) return false;
  for (const auto& field : sd.fields) {
    TypeSpec field_ts = field_init_type_of(field);
    if (field_ts.array_rank > 0 || is_vector_ty(field_ts)) continue;
    if (field_ts.base == TB_STRUCT && field_ts.ptr_level == 0 &&
        !struct_allows_init_normalization(field_ts)) {
      return false;
    }
    if (field_ts.base == TB_UNION && field_ts.ptr_level == 0 &&
        !union_allows_init_normalization(field_ts)) {
      return false;
    }
  }
  return true;
}

bool Lowerer::struct_allows_init_normalization(const TypeSpec& ts) const {
  if (ts.base != TB_STRUCT || ts.ptr_level != 0 || !ts.tag || !ts.tag[0]) return false;
  const auto it = module_->struct_defs.find(ts.tag);
  if (it == module_->struct_defs.end()) return false;
  const auto& sd = it->second;
  if (sd.is_union) return false;
  for (size_t fi = 0; fi < sd.fields.size(); ++fi) {
    const auto& field = sd.fields[fi];
    if (field.is_flexible_array) {
      if (fi + 1 != sd.fields.size()) return false;
      continue;
    }
    TypeSpec field_ts = field_init_type_of(field);
    if (field_ts.array_rank > 0 || is_vector_ty(field_ts)) continue;
    if (field_ts.base == TB_STRUCT && field_ts.ptr_level == 0 &&
        !struct_allows_init_normalization(field_ts)) {
      return false;
    }
    if (field_ts.base == TB_UNION && field_ts.ptr_level == 0 &&
        !union_allows_init_normalization(field_ts)) {
      return false;
    }
  }
  return true;
}

GlobalInit Lowerer::normalize_global_init(const TypeSpec& ts, const GlobalInit& init) {
  std::function<GlobalInit(const TypeSpec&, const InitList&, size_t&)> consume_from_flat;
  auto has_designators = [](const InitList& list) {
    return std::any_of(
        list.items.begin(), list.items.end(),
        [](const InitListItem& item) {
          return item.field_designator.has_value() || item.index_designator.has_value();
        });
  };
  auto find_aggregate_field_index =
      [&](const HirStructDef& sd, const InitListItem& item, size_t next_idx)
      -> std::optional<size_t> {
    size_t idx = next_idx;
    if (item.field_designator) {
      const auto fit = std::find_if(
          sd.fields.begin(), sd.fields.end(),
          [&](const HirStructField& field) { return field.name == *item.field_designator; });
      if (fit == sd.fields.end()) return std::nullopt;
      idx = static_cast<size_t>(std::distance(sd.fields.begin(), fit));
    } else if (item.index_designator && *item.index_designator >= 0) {
      idx = static_cast<size_t>(*item.index_designator);
    }
    if (idx >= sd.fields.size()) return std::nullopt;
    return idx;
  };
  auto make_field_mapped_item =
      [&](const HirStructDef& sd, size_t idx, const TypeSpec& target_ts,
          const GlobalInit& child) -> std::optional<InitListItem> {
    auto item = make_init_item(child);
    if (!item) return std::nullopt;
    if (!sd.fields[idx].name.empty()) item->field_designator = sd.fields[idx].name;
    else item->index_designator = static_cast<long long>(idx);
    if (target_ts.array_rank > 0 && target_ts.array_size >= 0) {
      item->resolved_array_bound = target_ts.array_size;
    }
    return item;
  };
  auto make_indexed_item =
      [&](long long idx, const TypeSpec& target_ts, const GlobalInit& child)
      -> std::optional<InitListItem> {
    auto item = make_init_item(child);
    if (!item) return std::nullopt;
    item->index_designator = idx;
    item->field_designator.reset();
    if (target_ts.array_rank > 0 && target_ts.array_size >= 0) {
      item->resolved_array_bound = target_ts.array_size;
    }
    return item;
  };

  auto normalize_scalar_like = [&](const TypeSpec& cur_ts, const GlobalInit& cur_init)
      -> GlobalInit {
    if (const auto* scalar = std::get_if<InitScalar>(&cur_init)) return GlobalInit(*scalar);
    if (const auto* list = std::get_if<InitList>(&cur_init)) {
      if (!list->items.empty()) return normalize_global_init(cur_ts, child_init_of(list->items.front()));
    }
    return std::monostate{};
  };

  consume_from_flat = [&](const TypeSpec& cur_ts, const InitList& list, size_t& cursor)
      -> GlobalInit {
    if (cursor >= list.items.size()) return std::monostate{};
    const auto& item = list.items[cursor];
    if (std::holds_alternative<std::shared_ptr<InitList>>(item.value)) {
      auto sub = std::get<std::shared_ptr<InitList>>(item.value);
      ++cursor;
      return normalize_global_init(cur_ts, GlobalInit(*sub));
    }
    if (is_scalar_init_type(cur_ts)) {
      ++cursor;
      if (const auto* scalar = std::get_if<InitScalar>(&item.value)) return GlobalInit(*scalar);
      return std::monostate{};
    }
    if (is_vector_ty(cur_ts) || cur_ts.array_rank > 0) {
      TypeSpec elem_ts = cur_ts;
      long long bound = 0;
      if (is_vector_ty(cur_ts)) {
        elem_ts = vector_element_type(cur_ts);
        bound = cur_ts.vector_lanes > 0 ? cur_ts.vector_lanes : 0;
      } else {
        for (int di = 0; di < elem_ts.array_rank - 1; ++di)
          elem_ts.array_dims[di] = elem_ts.array_dims[di + 1];
        elem_ts.array_dims[elem_ts.array_rank - 1] = -1;
        elem_ts.array_rank--;
        elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
        bound = resolve_array_ts(cur_ts, GlobalInit(list)).array_size;
      }
      if (!is_vector_ty(cur_ts) && is_direct_char_array_init(cur_ts, child_init_of(item))) {
        ++cursor;
        return child_init_of(item);
      }
      InitList out{};
      for (long long i = 0; i < bound && cursor < list.items.size(); ++i) {
        auto child = consume_from_flat(elem_ts, list, cursor);
        if (auto it = make_init_item(child)) out.items.push_back(std::move(*it));
      }
      return out;
    }
    if ((cur_ts.base == TB_STRUCT || cur_ts.base == TB_UNION) &&
        cur_ts.ptr_level == 0 && cur_ts.tag) {
      const auto sit = module_->struct_defs.find(cur_ts.tag);
      if (sit == module_->struct_defs.end()) {
        ++cursor;
        return std::monostate{};
      }
      const auto& sd = sit->second;
      if (sd.is_union) {
        InitList out{};
        if (!sd.fields.empty()) {
          TypeSpec field_ts = field_init_type_of(sd.fields.front());
          auto child = consume_from_flat(field_ts, list, cursor);
          field_ts = resolve_array_ts(field_ts, child);
          child = normalize_global_init(field_ts, child);
          if (auto it = make_field_mapped_item(sd, 0, field_ts, child)) {
            out.items.push_back(std::move(*it));
          }
        }
        return out;
      }
      InitList out{};
      for (size_t fi = 0; fi < sd.fields.size() && cursor < list.items.size(); ++fi) {
        TypeSpec field_ts = field_init_type_of(sd.fields[fi]);
        auto child = consume_from_flat(field_ts, list, cursor);
        field_ts = resolve_array_ts(field_ts, child);
        child = normalize_global_init(field_ts, child);
        if (auto it = make_field_mapped_item(sd, fi, field_ts, child)) {
          out.items.push_back(std::move(*it));
        }
      }
      return out;
    }
    ++cursor;
    return std::monostate{};
  };

  if (is_scalar_init_type(ts)) return normalize_scalar_like(ts, init);

  if (is_vector_ty(ts) || ts.array_rank > 0) {
    const auto* list = std::get_if<InitList>(&init);
    if (!list) return init;
    TypeSpec elem_ts = ts;
    long long bound = 0;
    if (is_vector_ty(ts)) {
      elem_ts = vector_element_type(ts);
      bound = ts.vector_lanes > 0 ? ts.vector_lanes : 0;
    } else {
      for (int di = 0; di < elem_ts.array_rank - 1; ++di)
        elem_ts.array_dims[di] = elem_ts.array_dims[di + 1];
      elem_ts.array_dims[elem_ts.array_rank - 1] = -1;
      elem_ts.array_rank--;
      elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
      bound = resolve_array_ts(ts, init).array_size;
    }
    if (!list->items.empty() && !is_vector_ty(ts) &&
        is_direct_char_array_init(ts, child_init_of(list->items.front()))) {
      return child_init_of(list->items.front());
    }

    if (!is_vector_ty(ts) && has_designators(*list)) {
      if (bound <= 0) return init;
      std::vector<std::optional<GlobalInit>> slots(static_cast<size_t>(bound));
      long long next_idx = 0;
      for (const auto& item : list->items) {
        long long idx = next_idx;
        if (item.index_designator && *item.index_designator >= 0) idx = *item.index_designator;
        if (idx >= 0 && idx < bound) {
          slots[static_cast<size_t>(idx)] = normalize_global_init(elem_ts, child_init_of(item));
        }
        next_idx = idx + 1;
      }
      InitList out{};
      for (long long idx = 0; idx < bound; ++idx) {
        const auto& slot = slots[static_cast<size_t>(idx)];
        if (!slot) continue;
        if (auto item = make_indexed_item(idx, elem_ts, *slot)) out.items.push_back(std::move(*item));
      }
      return out;
    }

    InitList out{};
    size_t cursor = 0;
    for (long long i = 0; i < bound && cursor < list->items.size(); ++i) {
      auto child = consume_from_flat(elem_ts, *list, cursor);
      if (!is_vector_ty(ts)) {
        if (auto it = make_indexed_item(i, elem_ts, child)) out.items.push_back(std::move(*it));
      } else if (auto it = make_init_item(child)) {
        out.items.push_back(std::move(*it));
      }
    }
    return out;
  }

  if (ts.base == TB_UNION && ts.ptr_level == 0 && ts.tag) {
    const auto sit = module_->struct_defs.find(ts.tag);
    if (sit == module_->struct_defs.end()) return init;
    const auto& sd = sit->second;
    if (!sd.is_union || sd.fields.empty()) return init;

    size_t idx = 0;
    GlobalInit child = std::monostate{};
    bool has_child = false;
    if (const auto* list = std::get_if<InitList>(&init)) {
      if (list->items.empty()) return init;
      const auto& item0 = list->items.front();
      const auto maybe_idx = find_aggregate_field_index(sd, item0, 0);
      if (maybe_idx && (item0.field_designator || item0.index_designator)) {
        idx = *maybe_idx;
        child = child_init_of(item0);
      } else {
        child = (list->items.size() == 1 &&
                 std::holds_alternative<std::shared_ptr<InitList>>(item0.value))
            ? GlobalInit(*std::get<std::shared_ptr<InitList>>(item0.value))
            : GlobalInit(*list);
      }
      has_child = true;
    } else {
      child = normalize_scalar_like(field_init_type_of(sd.fields.front()), init);
      has_child = !std::holds_alternative<std::monostate>(child);
    }
    if (!has_child) return init;
    TypeSpec field_ts = field_init_type_of(sd.fields[idx]);
    field_ts = resolve_array_ts(field_ts, child);
    auto normalized_child = normalize_global_init(field_ts, child);
    InitList out{};
    if (auto item = make_field_mapped_item(sd, idx, field_ts, normalized_child)) {
      out.items.push_back(std::move(*item));
    }
    return out;
  }

  if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0 && ts.tag) {
    const auto* list = std::get_if<InitList>(&init);
    const auto sit = module_->struct_defs.find(ts.tag);
    if (sit == module_->struct_defs.end()) {
      return list ? init : normalize_scalar_like(ts, init);
    }
    const auto& sd = sit->second;
    if (sd.is_union) return init;
    if (!struct_allows_init_normalization(ts)) {
      return list ? init : normalize_scalar_like(ts, init);
    }
    if (!list) {
      if (sd.fields.empty()) return std::monostate{};
      TypeSpec field_ts = field_init_type_of(sd.fields.front());
      field_ts = resolve_array_ts(field_ts, init);
      auto child = normalize_global_init(field_ts, init);
      InitList out{};
      if (auto item = make_field_mapped_item(sd, 0, field_ts, child)) {
        out.items.push_back(std::move(*item));
      }
      return out;
    }

    InitList out{};
    if (has_designators(*list)) {
      size_t next_idx = 0;
      for (const auto& item : list->items) {
        const auto maybe_idx = find_aggregate_field_index(sd, item, next_idx);
        if (!maybe_idx) continue;
        const size_t idx = *maybe_idx;
        TypeSpec field_ts = field_init_type_of(sd.fields[idx]);
        field_ts = resolve_array_ts(field_ts, child_init_of(item));
        auto child = normalize_global_init(field_ts, child_init_of(item));
        auto normalized_item = make_field_mapped_item(sd, idx, field_ts, child);
        if (!normalized_item) {
          next_idx = idx + 1;
          continue;
        }
        out.items.push_back(std::move(*normalized_item));
        next_idx = idx + 1;
      }
      return out;
    }

    size_t cursor = 0;
    for (size_t fi = 0; fi < sd.fields.size(); ++fi) {
      if (cursor >= list->items.size()) break;
      TypeSpec field_ts = field_init_type_of(sd.fields[fi]);
      auto child = consume_from_flat(field_ts, *list, cursor);
      field_ts = resolve_array_ts(field_ts, child);
      child = normalize_global_init(field_ts, child);
      if (auto it = make_field_mapped_item(sd, fi, field_ts, child))
        out.items.push_back(std::move(*it));
    }
    return out;
  }

  return init;
}
#endif


#if 0
void Lowerer::collect_weak_symbol_names(const std::vector<const Node*>& items) {
  for (const Node* item : items) {
    if (item->kind == NK_PRAGMA_WEAK && item->name) weak_symbols_.insert(item->name);
  }
}

void Lowerer::collect_enum_def(const Node* ed) {
  if (!ed || ed->kind != NK_ENUM_DEF || ed->n_enum_variants <= 0) return;
  if (!ed->enum_names || !ed->enum_vals) return;
  for (int i = 0; i < ed->n_enum_variants; ++i) {
    const char* name = ed->enum_names[i];
    if (!name || !name[0]) continue;
    enum_consts_[name] = ed->enum_vals[i];
    ct_state_->register_enum_const(name, ed->enum_vals[i]);
  }
}

void Lowerer::collect_initial_type_definitions(const std::vector<const Node*>& items) {
  for (const Node* item : items) {
    if (item->kind == NK_STRUCT_DEF) {
      lower_struct_def(item);
      if (item->name) struct_def_nodes_[item->name] = item;
      if (is_primary_template_struct_def(item) && item->name) {
        register_template_struct_primary(item->name, item);
      }
      if (item->template_origin_name && item->n_template_args > 0) {
        const Node* primary_tpl =
            find_template_struct_primary(item->template_origin_name);
        if (primary_tpl) register_template_struct_specialization(primary_tpl, item);
      }
    }
    if (item->kind == NK_ENUM_DEF) collect_enum_def(item);
  }
}

void Lowerer::collect_consteval_function_definitions(
    const std::vector<const Node*>& items) {
  for (const Node* item : items) {
    if (item->kind == NK_FUNCTION && item->is_consteval && item->name) {
      ct_state_->register_consteval_def(item->name, item);
    }
  }
}

void Lowerer::collect_template_function_definitions(
    const std::vector<const Node*>& items) {
  for (const Node* item : items) {
    if (item->kind == NK_FUNCTION && item->name && item->n_template_params > 0) {
      ct_state_->register_template_def(item->name, item);
    }
  }
}

void Lowerer::collect_function_template_specializations(
    const std::vector<const Node*>& items) {
  for (const Node* item : items) {
    if (item->kind == NK_FUNCTION && item->name && item->is_explicit_specialization &&
        item->n_template_args > 0) {
      const Node* tpl_def = ct_state_->find_template_def(item->name);
      if (tpl_def) ct_state_->register_function_specialization(tpl_def, item);
    }
  }
}

void Lowerer::collect_depth0_template_instantiations(
    const std::vector<const Node*>& items) {
  for (const Node* item : items) {
    if (item->kind == NK_FUNCTION && item->body && item->n_template_params == 0) {
      collect_template_instantiations(item->body, item);
    }
  }
}

std::vector<std::string> Lowerer::get_template_param_order_from_instances(
    const std::string& fn_name) {
  const Node* tpl_def = ct_state_->find_template_def(fn_name);
  if (tpl_def) return get_template_param_order(tpl_def);
  return {};
}

std::string Lowerer::record_seed(const std::string& fn_name,
                                 TypeBindings bindings,
                                 NttpBindings nttp_bindings,
                                 TemplateSeedOrigin origin) {
  const Node* primary_def = ct_state_->find_template_def(fn_name);
  auto param_order =
      get_template_param_order(primary_def, &bindings, &nttp_bindings);
  return registry_.record_seed(fn_name, std::move(bindings),
                               std::move(nttp_bindings), param_order,
                               origin, primary_def);
}

std::string Lowerer::resolve_template_call_name(
    const Node* call_var,
    const TypeBindings* enclosing_bindings,
    const NttpBindings* enclosing_nttp) {
  if (!call_var || !call_var->name ||
      (call_var->n_template_args <= 0 && !call_var->has_template_args)) {
    return call_var ? (call_var->name ? call_var->name : "") : "";
  }
  const Node* fn_def = ct_state_->find_template_def(call_var->name);
  if (!fn_def) return call_var->name;
  if (fn_def->is_consteval) return call_var->name;
  TypeBindings bindings = merge_explicit_and_deduced_type_bindings(
      nullptr, call_var, fn_def, enclosing_bindings);
  NttpBindings nttp_bindings =
      build_call_nttp_bindings(call_var, fn_def, enclosing_nttp);
  return mangle_template_name(call_var->name, bindings, nttp_bindings);
}

void Lowerer::collect_template_instantiations(const Node* n,
                                              const Node* enclosing_fn) {
  if (!n) return;
  if (n->kind == NK_CALL && n->left && n->left->kind == NK_VAR &&
      n->left->name &&
      (n->left->n_template_args > 0 || n->left->has_template_args)) {
    const Node* fn_def = ct_state_->find_template_def(n->left->name);
    if (fn_def && !fn_def->is_consteval && fn_def->n_template_params > 0) {
      if (enclosing_fn && enclosing_fn->name) {
        auto* enc_list = registry_.find_instances(enclosing_fn->name);
        if (enc_list) {
          for (const auto& enc_inst : *enc_list) {
            TypeBindings inner = merge_explicit_and_deduced_type_bindings(
                n, n->left, fn_def, &enc_inst.bindings, enclosing_fn);
            NttpBindings call_nttp = build_call_nttp_bindings(
                n->left, fn_def, &enc_inst.nttp_bindings);
            record_seed(n->left->name, std::move(inner), call_nttp,
                        TemplateSeedOrigin::EnclosingTemplateExpansion);
          }
          goto recurse;
        }
      }
      NttpBindings call_nttp = build_call_nttp_bindings(n->left, fn_def);
      if (has_forwarded_nttp(n->left)) goto recurse;
      TypeBindings bindings = merge_explicit_and_deduced_type_bindings(
          n, n->left, fn_def, nullptr, enclosing_fn);
      std::string mangled = record_seed(n->left->name, std::move(bindings),
                                        call_nttp,
                                        TemplateSeedOrigin::DirectCall);
      if (fn_def->n_template_params > n->left->n_template_args) {
        TypeBindings full_bindings = merge_explicit_and_deduced_type_bindings(
            n, n->left, fn_def, nullptr, enclosing_fn);
        deduced_template_calls_[n] = {
            mangled, std::move(full_bindings), std::move(call_nttp)};
      }
    }
  }
  if (n->kind == NK_CALL && n->left && n->left->kind == NK_VAR &&
      n->left->name && n->left->n_template_args == 0 &&
      !n->left->has_template_args) {
    const Node* fn_def = ct_state_->find_template_def(n->left->name);
    if (fn_def && !fn_def->is_consteval && fn_def->n_template_params > 0) {
      TypeBindings deduced =
          try_deduce_template_type_args(n, fn_def, enclosing_fn);
      if (deduction_covers_all_type_params(deduced, fn_def)) {
        fill_deduced_defaults(deduced, fn_def);
        NttpBindings nttp{};
        if (fn_def->template_param_has_default) {
          for (int i = 0; i < fn_def->n_template_params; ++i) {
            if (!fn_def->template_param_names[i]) continue;
            if (!fn_def->template_param_is_nttp ||
                !fn_def->template_param_is_nttp[i]) {
              continue;
            }
            if (fn_def->template_param_has_default[i]) {
              nttp[fn_def->template_param_names[i]] =
                  fn_def->template_param_default_values[i];
            }
          }
        }
        std::string mangled = record_seed(
            n->left->name, TypeBindings(deduced), nttp,
            TemplateSeedOrigin::DeducedCall);
        deduced_template_calls_[n] = {
            mangled, std::move(deduced), std::move(nttp)};
      }
    }
  }

recurse:
  if (n->left) collect_template_instantiations(n->left, enclosing_fn);
  if (n->right) collect_template_instantiations(n->right, enclosing_fn);
  if (n->cond) collect_template_instantiations(n->cond, enclosing_fn);
  if (n->then_) collect_template_instantiations(n->then_, enclosing_fn);
  if (n->else_) collect_template_instantiations(n->else_, enclosing_fn);
  if (n->body) collect_template_instantiations(n->body, enclosing_fn);
  if (n->init) collect_template_instantiations(n->init, enclosing_fn);
  if (n->update) collect_template_instantiations(n->update, enclosing_fn);
  for (int i = 0; i < n->n_children; ++i) {
    if (n->children[i]) collect_template_instantiations(n->children[i],
                                                        enclosing_fn);
  }
}

void Lowerer::collect_consteval_template_instantiations(
    const Node* n, const Node* enclosing_fn) {
  if (!n) return;
  if (n->kind == NK_CALL && n->left && n->left->kind == NK_VAR &&
      n->left->name &&
      (n->left->n_template_args > 0 || n->left->has_template_args)) {
    const Node* fn_def = ct_state_->find_template_def(n->left->name);
    if (fn_def && fn_def->is_consteval && fn_def->n_template_params > 0) {
      if (enclosing_fn && enclosing_fn->name) {
        auto* enc_list = registry_.find_instances(enclosing_fn->name);
        if (enc_list) {
          for (const auto& enc_inst : *enc_list) {
            TypeBindings inner =
                build_call_bindings(n->left, fn_def, &enc_inst.bindings);
            NttpBindings call_nttp = build_call_nttp_bindings(
                n->left, fn_def, &enc_inst.nttp_bindings);
            record_seed(n->left->name, std::move(inner), call_nttp,
                        TemplateSeedOrigin::ConstevalEnclosingExpansion);
          }
          goto recurse_ce;
        }
        if (enclosing_fn->n_template_params > 0) goto recurse_ce;
      }
      NttpBindings call_nttp = build_call_nttp_bindings(n->left, fn_def);
      if (has_forwarded_nttp(n->left)) goto recurse_ce;
      TypeBindings bindings = build_call_bindings(n->left, fn_def, nullptr);
      record_seed(n->left->name, std::move(bindings), call_nttp,
                  TemplateSeedOrigin::ConstevalSeed);
    }
  }

recurse_ce:
  if (n->left) collect_consteval_template_instantiations(n->left, enclosing_fn);
  if (n->right) {
    collect_consteval_template_instantiations(n->right, enclosing_fn);
  }
  if (n->cond) collect_consteval_template_instantiations(n->cond, enclosing_fn);
  if (n->then_) collect_consteval_template_instantiations(n->then_, enclosing_fn);
  if (n->else_) collect_consteval_template_instantiations(n->else_, enclosing_fn);
  if (n->body) collect_consteval_template_instantiations(n->body, enclosing_fn);
  if (n->init) collect_consteval_template_instantiations(n->init, enclosing_fn);
  if (n->update) {
    collect_consteval_template_instantiations(n->update, enclosing_fn);
  }
  for (int i = 0; i < n->n_children; ++i) {
    if (n->children[i]) {
      collect_consteval_template_instantiations(n->children[i], enclosing_fn);
    }
  }
}

void Lowerer::run_consteval_template_seed_fixpoint(
    const std::vector<const Node*>& items) {
  for (int pass = 0; pass < 8; ++pass) {
    size_t prev_size = registry_.total_instance_count();
    for (const Node* item : items) {
      if (item->kind == NK_FUNCTION && item->body && item->n_template_params > 0) {
        collect_consteval_template_instantiations(item->body, item);
      }
    }
    registry_.realize_seeds();
    if (registry_.total_instance_count() == prev_size) break;
  }
}

void Lowerer::finalize_template_seed_realization() {
  registry_.realize_seeds();
  if (!registry_.verify_parity()) {
    registry_.dump_parity(stderr);
    throw std::runtime_error(
        "InstantiationRegistry: seed/instance parity violation after "
        "realize_seeds()");
  }
}

void Lowerer::populate_hir_template_defs(Module& m) {
  ct_state_->for_each_template_def([&](const std::string& name, const Node* fn_def) {
    HirTemplateDef tdef;
    tdef.name = name;
    tdef.is_consteval = fn_def->is_consteval;
    tdef.span = make_span(fn_def);
    for (int i = 0; i < fn_def->n_template_params; ++i) {
      if (fn_def->template_param_names[i])
        tdef.template_params.emplace_back(fn_def->template_param_names[i]);
      tdef.param_is_nttp.push_back(
          fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]);
    }
    tdef.instances = ct_state_->instances_to_hir_metadata(
        name, tdef.template_params);
    m.template_defs[name] = std::move(tdef);
  });
}

void Lowerer::collect_ref_overloaded_free_functions(
    const std::vector<const Node*>& items) {
  std::unordered_map<std::string, const Node*> first_fn_decl;
  for (const Node* item : items) {
    if (item->kind != NK_FUNCTION || !item->name || item->is_consteval ||
        item->n_template_params > 0 || item->is_explicit_specialization) {
      continue;
    }
    std::string fn_name = item->name;
    auto prev_it = first_fn_decl.find(fn_name);
    if (prev_it == first_fn_decl.end()) {
      first_fn_decl[fn_name] = item;
      continue;
    }
    const Node* prev = prev_it->second;
    if (prev->n_params != item->n_params) continue;
    bool has_ref_diff = false;
    bool base_match = true;
    for (int pi = 0; pi < item->n_params; ++pi) {
      const TypeSpec& a = prev->params[pi]->type;
      const TypeSpec& b = item->params[pi]->type;
      if (a.is_lvalue_ref != b.is_lvalue_ref || a.is_rvalue_ref != b.is_rvalue_ref) {
        has_ref_diff = true;
      }
      if (a.base != b.base || a.ptr_level != b.ptr_level) {
        base_match = false;
        break;
      }
      if ((a.base == TB_STRUCT || a.base == TB_UNION) && a.tag && b.tag) {
        if (std::string(a.tag) != std::string(b.tag)) {
          base_match = false;
          break;
        }
      }
    }
    if (!has_ref_diff || !base_match) continue;
    auto& ovset = ref_overload_set_[fn_name];
    if (ovset.empty()) {
      Lowerer::RefOverloadEntry e0;
      e0.mangled_name = fn_name;
      for (int pi = 0; pi < prev->n_params; ++pi) {
        e0.param_is_rvalue_ref.push_back(prev->params[pi]->type.is_rvalue_ref);
        e0.param_is_lvalue_ref.push_back(prev->params[pi]->type.is_lvalue_ref);
      }
      ovset.push_back(std::move(e0));
      ref_overload_mangled_[prev] = fn_name;
    }
    Lowerer::RefOverloadEntry e1;
    e1.mangled_name = fn_name + "__rref_overload";
    for (int pi = 0; pi < item->n_params; ++pi) {
      e1.param_is_rvalue_ref.push_back(item->params[pi]->type.is_rvalue_ref);
      e1.param_is_lvalue_ref.push_back(item->params[pi]->type.is_lvalue_ref);
    }
    ovset.push_back(std::move(e1));
    ref_overload_mangled_[item] = fn_name + "__rref_overload";
  }
}
#endif

#if 0
ExprId Lowerer::hoist_compound_literal_to_global(const Node* addr_node,
                                                 const Node* clit) {
  GlobalVar cg{};
  cg.id = next_global_id();
  const std::string clit_name = "__clit_" + std::to_string(cg.id.value);
  cg.name = clit_name;
  cg.type = qtype_from(clit->type, ValueCategory::LValue);
  cg.linkage = {true, false, false};
  cg.is_const = false;
  cg.span = make_span(clit);
  if (clit->left && clit->left->kind == NK_INIT_LIST) {
    cg.init = lower_init_list(clit->left);
    cg.type.spec = resolve_array_ts(cg.type.spec, cg.init);
    cg.init = normalize_global_init(cg.type.spec, cg.init);
  }
  module_->global_index[clit_name] = cg.id;
  module_->globals.push_back(std::move(cg));

  TypeSpec ptr_ts = clit->type;
  ptr_ts.ptr_level++;
  DeclRef dr{};
  dr.name = clit_name;
  dr.global = module_->global_index[clit_name];
  ExprId dr_id = append_expr(clit, dr, clit->type, ValueCategory::LValue);
  UnaryExpr addr{};
  addr.op = UnaryOp::AddrOf;
  addr.operand = dr_id;
  return append_expr(addr_node, addr, ptr_ts);
}

bool Lowerer::is_ast_lvalue(const Node* n) {
  if (!n) return false;
  switch (n->kind) {
    case NK_VAR:
    case NK_INDEX:
    case NK_DEREF:
    case NK_MEMBER:
      return true;
    default:
      return false;
  }
}

ExprId Lowerer::lower_call_expr(FunctionCtx* ctx, const Node* n) {
  if (auto consteval_expr = try_lower_consteval_call_expr(ctx, n)) {
    return *consteval_expr;
  }

  if (ctx && n->left && n->left->kind == NK_VAR && n->left->name &&
      n->n_children == 0) {
    auto sit = module_->struct_defs.find(n->left->name);
    auto cit = struct_constructors_.find(n->left->name);
    if (sit != module_->struct_defs.end() &&
        (cit == struct_constructors_.end() || cit->second.empty())) {
      TypeSpec tmp_ts{};
      tmp_ts.base = TB_STRUCT;
      tmp_ts.tag = sit->second.tag.c_str();
      tmp_ts.array_size = -1;
      tmp_ts.inner_rank = -1;

      const LocalId tmp_lid = next_local_id();
      const std::string tmp_name = "__brace_tmp_" + std::to_string(tmp_lid.value);
      LocalDecl tmp{};
      tmp.id = tmp_lid;
      tmp.name = tmp_name;
      tmp.type = qtype_from(tmp_ts, ValueCategory::LValue);
      tmp.storage = StorageClass::Auto;
      tmp.span = make_span(n);
      ctx->locals[tmp.name] = tmp.id;
      ctx->local_types[tmp.id.value] = tmp_ts;
      append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});

      DeclRef tmp_ref{};
      tmp_ref.name = tmp_name;
      tmp_ref.local = tmp_lid;
      return append_expr(n, tmp_ref, tmp_ts, ValueCategory::LValue);
    }
  }

  if (ctx && n->left && n->left->kind == NK_VAR && n->left->name) {
    const std::string callee_name = n->left->name;
    auto cit = struct_constructors_.find(callee_name);
    auto sit = module_->struct_defs.find(callee_name);
    if (cit != struct_constructors_.end() && sit != module_->struct_defs.end()) {
      const CtorOverload* best = nullptr;
      if (cit->second.size() == 1) {
        if (cit->second[0].method_node->n_params == n->n_children)
          best = &cit->second[0];
      } else {
        int best_score = -1;
        for (const auto& ov : cit->second) {
          if (ov.method_node->n_params != n->n_children) continue;
          int score = 0;
          bool viable = true;
          for (int pi = 0; pi < ov.method_node->n_params && viable; ++pi) {
            TypeSpec param_ts = ov.method_node->params[pi]->type;
            resolve_typedef_to_struct(param_ts);
            TypeSpec arg_ts = infer_generic_ctrl_type(ctx, n->children[pi]);
            const bool arg_is_lvalue = is_ast_lvalue(n->children[pi]);
            TypeSpec param_base = param_ts;
            param_base.is_lvalue_ref = false;
            param_base.is_rvalue_ref = false;
            if (param_base.base == arg_ts.base &&
                param_base.ptr_level == arg_ts.ptr_level) {
              score += 2;
              if (param_ts.is_rvalue_ref && !arg_is_lvalue) {
                score += 4;
              } else if (param_ts.is_rvalue_ref && arg_is_lvalue) {
                viable = false;
              } else if (param_ts.is_lvalue_ref && arg_is_lvalue) {
                score += 3;
              } else if (param_ts.is_lvalue_ref && !arg_is_lvalue) {
                score += 1;
              }
            } else if (param_base.ptr_level == 0 && arg_ts.ptr_level == 0 &&
                       param_base.base != TB_STRUCT && arg_ts.base != TB_STRUCT &&
                       param_base.base != TB_VOID && arg_ts.base != TB_VOID) {
              score += 1;
            } else {
              viable = false;
            }
          }
          if (viable && score > best_score) {
            best_score = score;
            best = &ov;
          }
        }
      }

      if (best) {
        if (best->method_node->is_deleted) {
          std::string diag = "error: call to deleted constructor '";
          diag += callee_name;
          diag += "'";
          throw std::runtime_error(diag);
        }

        TypeSpec tmp_ts{};
        tmp_ts.base = TB_STRUCT;
        tmp_ts.tag = sit->second.tag.c_str();
        tmp_ts.array_size = -1;
        tmp_ts.inner_rank = -1;

        const LocalId tmp_lid = next_local_id();
        const std::string tmp_name = "__ctor_tmp_" + std::to_string(tmp_lid.value);
        LocalDecl tmp{};
        tmp.id = tmp_lid;
        tmp.name = tmp_name;
        tmp.type = qtype_from(tmp_ts, ValueCategory::LValue);
        tmp.storage = StorageClass::Auto;
        tmp.span = make_span(n);
        ctx->locals[tmp.name] = tmp.id;
        ctx->local_types[tmp.id.value] = tmp_ts;
        append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});

        CallExpr ctor_call{};
        DeclRef callee_ref{};
        callee_ref.name = best->mangled_name;
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        ctor_call.callee = append_expr(n, callee_ref, callee_ts);

        DeclRef tmp_ref{};
        tmp_ref.name = tmp_name;
        tmp_ref.local = tmp_lid;
        ExprId tmp_id = append_expr(n, tmp_ref, tmp_ts, ValueCategory::LValue);
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = tmp_id;
        TypeSpec ptr_ts = tmp_ts;
        ptr_ts.ptr_level++;
        ctor_call.args.push_back(append_expr(n, addr, ptr_ts));

        for (int i = 0; i < n->n_children; ++i) {
          TypeSpec param_ts = best->method_node->params[i]->type;
          resolve_typedef_to_struct(param_ts);
          if (param_ts.is_rvalue_ref || param_ts.is_lvalue_ref) {
            const Node* arg = n->children[i];
            const Node* inner = arg;
            if (inner->kind == NK_CAST && inner->left) inner = inner->left;
            ExprId arg_val = lower_expr(ctx, inner);
            TypeSpec storage_ts = reference_storage_ts(param_ts);
            UnaryExpr addr_e{};
            addr_e.op = UnaryOp::AddrOf;
            addr_e.operand = arg_val;
            ctor_call.args.push_back(append_expr(arg, addr_e, storage_ts));
          } else {
            ctor_call.args.push_back(lower_expr(ctx, n->children[i]));
          }
        }

        ExprId call_id = append_expr(n, ctor_call, fn_ts);
        ExprStmt es{};
        es.expr = call_id;
        append_stmt(*ctx, Stmt{StmtPayload{es}, make_span(n)});
        return tmp_id;
      }
    }
  }

  if (ctx && n->left && n->left->kind == NK_VAR && n->left->name &&
      n->left->has_template_args &&
      find_template_struct_primary(n->left->name)) {
    ExprId tmp_expr = lower_expr(ctx, n->left);
    if (n->n_children == 0) return tmp_expr;
    const TypeSpec& tmp_ts = module_->expr_pool[tmp_expr.value].type.spec;
    if (tmp_ts.base == TB_STRUCT && tmp_ts.ptr_level == 0 && tmp_ts.tag) {
      auto resolved = find_struct_method_mangled(tmp_ts.tag, "operator_call", false);
      if (!resolved)
        resolved = find_struct_method_mangled(tmp_ts.tag, "operator_call", true);
      if (resolved) {
        ExprId this_obj = tmp_expr;
        if (module_->expr_pool[tmp_expr.value].type.category != ValueCategory::LValue) {
          LocalDecl tmp{};
          tmp.id = next_local_id();
          std::string tmp_name = "__call_tmp_" + std::to_string(tmp.id.value);
          tmp.name = tmp_name;
          tmp.type = qtype_from(tmp_ts, ValueCategory::RValue);
          const TypeSpec init_ts = module_->expr_pool[tmp_expr.value].type.spec;
          if (init_ts.base == tmp_ts.base && init_ts.ptr_level == tmp_ts.ptr_level &&
              init_ts.tag == tmp_ts.tag) {
            tmp.init = tmp_expr;
          }
          append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});

          DeclRef tmp_ref{};
          tmp_ref.name = tmp_name;
          tmp_ref.local = LocalId{tmp.id.value};
          this_obj = append_expr(n, tmp_ref, tmp_ts, ValueCategory::LValue);
        }
        std::string resolved_mangled = *resolved;
        CallExpr oc{};
        DeclRef dr{};
        dr.name = resolved_mangled;
        auto fit = module_->fn_index.find(dr.name);
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        if (fit != module_->fn_index.end() &&
            fit->second.value < module_->functions.size())
          fn_ts = module_->functions[fit->second.value].return_type.spec;
        if (fn_ts.base == TB_VOID) {
          if (auto rit = find_struct_method_return_type(
                  tmp_ts.tag, "operator_call", false))
            fn_ts = *rit;
        }
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        oc.callee = append_expr(n, dr, callee_ts);
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = this_obj;
        TypeSpec ptr_ts = tmp_ts;
        ptr_ts.ptr_level++;
        oc.args.push_back(append_expr(n, addr, ptr_ts));
        for (int i = 0; i < n->n_children; ++i)
          oc.args.push_back(lower_expr(ctx, n->children[i]));
        return append_expr(n, oc, fn_ts);
      }
    }
    return tmp_expr;
  }

  if (n->left) {
    TypeSpec callee_ts = infer_generic_ctrl_type(ctx, n->left);
    if (callee_ts.base == TB_STRUCT && callee_ts.ptr_level == 0 && callee_ts.tag) {
      std::vector<const Node*> arg_nodes;
      for (int i = 0; i < n->n_children; ++i)
        arg_nodes.push_back(n->children[i]);
      ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_call", arg_nodes);
      if (op.valid()) return op;
    }
  }

  CallExpr c{};
  auto maybe_lower_ref_arg = [&](const Node* arg_node, const TypeSpec* param_ts) -> ExprId {
    if (ctx && arg_node && arg_node->kind == NK_INIT_LIST && param_ts &&
        !param_ts->is_lvalue_ref && !param_ts->is_rvalue_ref) {
      TypeSpec direct_ts = *param_ts;
      direct_ts.is_lvalue_ref = false;
      direct_ts.is_rvalue_ref = false;
      resolve_typedef_to_struct(direct_ts);
      if (direct_ts.base == TB_STRUCT && direct_ts.ptr_level == 0) {
        if (describe_initializer_list_struct(direct_ts, nullptr, nullptr, nullptr))
          return materialize_initializer_list_arg(ctx, arg_node, direct_ts);
      }
    }
    if (!param_ts || (!param_ts->is_lvalue_ref && !param_ts->is_rvalue_ref))
      return lower_expr(ctx, arg_node);
    TypeSpec storage_ts = reference_storage_ts(*param_ts);
    if (param_ts->is_rvalue_ref) {
      if (arg_node && arg_node->kind == NK_CAST && arg_node->type.is_rvalue_ref &&
          arg_node->left) {
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = lower_expr(ctx, arg_node->left);
        return append_expr(arg_node, addr, storage_ts);
      }
      ExprId arg_val = lower_expr(ctx, arg_node);
      TypeSpec val_ts = reference_value_ts(*param_ts);
      resolve_typedef_to_struct(val_ts);
      LocalDecl tmp{};
      tmp.id = next_local_id();
      tmp.name = "__rref_arg_tmp";
      tmp.type = qtype_from(val_ts, ValueCategory::LValue);
      tmp.init = arg_val;
      const LocalId tmp_lid = tmp.id;
      ctx->locals[tmp.name] = tmp.id;
      ctx->local_types[tmp.id.value] = val_ts;
      append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(arg_node)});
      DeclRef tmp_ref{};
      tmp_ref.name = "__rref_arg_tmp";
      tmp_ref.local = tmp_lid;
      ExprId var_id = append_expr(arg_node, tmp_ref, val_ts, ValueCategory::LValue);
      UnaryExpr addr{};
      addr.op = UnaryOp::AddrOf;
      addr.operand = var_id;
      return append_expr(arg_node, addr, storage_ts);
    }
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = lower_expr(ctx, arg_node);
    return append_expr(arg_node, addr, storage_ts);
  };
  auto direct_callee_fn = [&](const std::string& name) -> const Function* {
    auto fit = module_->fn_index.find(name);
    if (fit == module_->fn_index.end()) return nullptr;
    return module_->find_function(fit->second);
  };
  auto try_expand_pack_call_arg =
      [&](const Node* arg_node, const TypeSpec* construct_param_ts) -> bool {
    if (!ctx || !arg_node || arg_node->kind != NK_PACK_EXPANSION || !arg_node->left)
      return false;
    const Node* pattern = arg_node->left;
    if (pattern->kind != NK_CALL || pattern->n_children != 1 || !pattern->left)
      return false;
    if (pattern->left->kind != NK_VAR || !pattern->left->name ||
        std::strcmp(pattern->left->name, "forward") != 0)
      return false;
    if (pattern->left->n_template_args != 1 || !pattern->left->template_arg_types ||
        pattern->left->template_arg_types[0].base != TB_TYPEDEF ||
        !pattern->left->template_arg_types[0].tag)
      return false;
    const Node* forwarded_arg = pattern->children[0];
    if (!forwarded_arg || forwarded_arg->kind != NK_VAR || !forwarded_arg->name)
      return false;
    auto pit = ctx->pack_params.find(forwarded_arg->name);
    if (pit == ctx->pack_params.end() || pit->second.empty()) return false;

    for (const auto& elem : pit->second) {
      const Node* tpl_fn = ct_state_->find_template_def("forward");
      TypeBindings forward_bindings;
      if (tpl_fn && tpl_fn->n_template_params > 0 && tpl_fn->template_param_names[0])
        forward_bindings[tpl_fn->template_param_names[0]] = elem.type;
      else
        forward_bindings["T"] = elem.type;
      const std::string callee_name =
          mangle_template_name("forward", forward_bindings);

      DeclRef callee_ref{};
      callee_ref.name = callee_name;
      TypeSpec callee_ts = pattern->left->type;
      if (const Function* callee_fn = direct_callee_fn(callee_name)) {
        callee_ts = callee_fn->return_type.spec;
        callee_ts.ptr_level++;
      }

      CallExpr expanded_call{};
      expanded_call.callee = append_expr(pattern->left, callee_ref, callee_ts);
      TemplateCallInfo tci;
      tci.source_template = "forward";
      tci.template_args.push_back(elem.type);
      expanded_call.template_info = std::move(tci);

      DeclRef param_ref{};
      param_ref.name = elem.element_name;
      param_ref.param_index = elem.param_index;
      ExprId param_expr = append_expr(forwarded_arg, param_ref, elem.type, ValueCategory::LValue);
      UnaryExpr addr{};
      addr.op = UnaryOp::AddrOf;
      addr.operand = param_expr;
      TypeSpec param_storage_ts = elem.type;
      param_storage_ts.ptr_level += 1;
      expanded_call.args.push_back(append_expr(forwarded_arg, addr, param_storage_ts));

      TypeSpec ret_ts = elem.type;
      ret_ts.is_lvalue_ref = false;
      ret_ts.is_rvalue_ref = true;
      ExprId expanded_id = append_expr(pattern, expanded_call, ret_ts);
      if (!construct_param_ts || (!construct_param_ts->is_lvalue_ref &&
                                  !construct_param_ts->is_rvalue_ref)) {
        c.args.push_back(expanded_id);
      } else {
        TypeSpec storage_ts = reference_storage_ts(*construct_param_ts);
        UnaryExpr outer_addr{};
        outer_addr.op = UnaryOp::AddrOf;
        outer_addr.operand = expanded_id;
        c.args.push_back(append_expr(pattern, outer_addr, storage_ts));
      }
    }
    return true;
  };
  if (n->left && n->left->kind == NK_MEMBER && n->left->name) {
    const Node* base_node = n->left->left;
    const char* method_name = n->left->name;
    TypeSpec base_ts = infer_generic_ctrl_type(ctx, base_node);
    if (n->left->is_arrow && base_ts.ptr_level > 0)
      base_ts.ptr_level--;
    const char* tag = base_ts.tag;
    if (tag) {
      std::string base_key = std::string(tag) + "::" + method_name;
      std::string const_key = base_key + "_const";
      decltype(struct_methods_)::iterator mit;
      if (base_ts.is_const) {
        mit = struct_methods_.find(const_key);
        if (mit == struct_methods_.end())
          mit = struct_methods_.find(base_key);
      } else {
        mit = struct_methods_.find(base_key);
        if (mit == struct_methods_.end())
          mit = struct_methods_.find(const_key);
      }
      if (mit != struct_methods_.end()) {
        DeclRef dr{};
        std::string resolved_method = mit->second;
        auto ovit = ref_overload_set_.find(mit->first);
        if (ovit != ref_overload_set_.end() && !ovit->second.empty()) {
          resolved_method = resolve_ref_overload(mit->first, n);
        }
        dr.name = resolved_method;
        auto fit = module_->fn_index.find(dr.name);
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        if (fit != module_->fn_index.end() &&
            fit->second.value < module_->functions.size()) {
          fn_ts = module_->functions[fit->second.value].return_type.spec;
        }
        fn_ts.ptr_level++;
        c.callee = append_expr(n->left, dr, fn_ts);
        ExprId base_id = lower_expr(ctx, base_node);
        if (n->left->is_arrow) {
          c.args.push_back(base_id);
        } else {
          UnaryExpr addr{};
          addr.op = UnaryOp::AddrOf;
          addr.operand = base_id;
          TypeSpec ptr_ts = base_ts;
          ptr_ts.ptr_level++;
          c.args.push_back(append_expr(base_node, addr, ptr_ts));
        }
        const Node* callee_method = find_pending_method_by_mangled(resolved_method);
        for (int i = 0; i < n->n_children; ++i) {
          const TypeSpec* param_ts = nullptr;
          if (callee_method && i < callee_method->n_params &&
              callee_method->params[i]) {
            param_ts = &callee_method->params[i]->type;
          }
          if (try_expand_pack_call_arg(n->children[i], param_ts)) continue;
          c.args.push_back(maybe_lower_ref_arg(n->children[i], param_ts));
        }
        TypeSpec ts = n->type;
        if (ts.base == TB_VOID && ts.ptr_level == 0) {
          auto mfit = module_->fn_index.find(resolved_method);
          if (mfit != module_->fn_index.end() &&
              mfit->second.value < module_->functions.size())
            ts = module_->functions[mfit->second.value].return_type.spec;
        }
        return append_expr(n, c, ts);
      }
    }
  }

  std::string resolved_callee_name;
  if (n->left && n->left->kind == NK_VAR && n->left->name &&
      (n->left->n_template_args > 0 || n->left->has_template_args) &&
      ct_state_->has_template_def(n->left->name) &&
      !ct_state_->has_consteval_def(n->left->name)) {
    const TypeBindings* enc = (ctx && !ctx->tpl_bindings.empty())
                                  ? &ctx->tpl_bindings : nullptr;
    const NttpBindings* enc_nttp = (ctx && !ctx->nttp_bindings.empty())
                                       ? &ctx->nttp_bindings : nullptr;
    const Node* tpl_fn = ct_state_->find_template_def(n->left->name);
    TypeBindings bindings;
    NttpBindings nttp_bindings;
    if (auto ded_it = deduced_template_calls_.find(n);
        ded_it != deduced_template_calls_.end()) {
      resolved_callee_name = ded_it->second.mangled_name;
      bindings = ded_it->second.bindings;
      nttp_bindings = ded_it->second.nttp_bindings;
    } else {
      bindings = merge_explicit_and_deduced_type_bindings(n, n->left, tpl_fn, enc);
      nttp_bindings = build_call_nttp_bindings(n->left, tpl_fn, enc_nttp);
      resolved_callee_name =
          mangle_template_name(n->left->name, bindings, nttp_bindings);
      if (tpl_fn) {
        const auto param_order = get_template_param_order(tpl_fn, &bindings, &nttp_bindings);
        const SpecializationKey spec_key = nttp_bindings.empty()
            ? make_specialization_key(n->left->name, param_order, bindings)
            : make_specialization_key(n->left->name, param_order, bindings, nttp_bindings);
        if (const auto* inst_list = registry_.find_instances(n->left->name)) {
          for (const auto& inst : *inst_list) {
            if (inst.spec_key == spec_key) {
              resolved_callee_name = inst.mangled_name;
              break;
            }
          }
        }
      }
    }
    DeclRef dr{};
    dr.name = resolved_callee_name;
    auto fit = module_->fn_index.find(dr.name);
    if (fit != module_->fn_index.end() &&
        fit->second.value < module_->functions.size()) {
      TypeSpec fn_ts = module_->functions[fit->second.value].return_type.spec;
      fn_ts.ptr_level++;
      c.callee = append_expr(n->left, dr, fn_ts);
    } else {
      c.callee = append_expr(n->left, dr, n->left->type);
    }
    TemplateCallInfo tci;
    tci.source_template = n->left->name;
    if (tpl_fn) {
      for (const auto& pname : get_template_param_order(tpl_fn, &bindings, &nttp_bindings)) {
        auto bit = bindings.find(pname);
        if (bit != bindings.end()) tci.template_args.push_back(bit->second);
      }
      tci.nttp_args = std::move(nttp_bindings);
    }
    c.template_info = std::move(tci);
  } else if (auto ded_it = deduced_template_calls_.find(n);
             ded_it != deduced_template_calls_.end()) {
    resolved_callee_name = ded_it->second.mangled_name;
    DeclRef dr{};
    dr.name = resolved_callee_name;
    auto fit = module_->fn_index.find(dr.name);
    if (fit != module_->fn_index.end() &&
        fit->second.value < module_->functions.size()) {
      TypeSpec fn_ts = module_->functions[fit->second.value].return_type.spec;
      fn_ts.ptr_level++;
      c.callee = append_expr(n->left, dr, fn_ts);
    } else {
      c.callee = append_expr(n->left, dr, n->left->type);
    }
    TemplateCallInfo tci;
    tci.source_template = n->left->name;
    const Node* tpl_fn = ct_state_->find_template_def(n->left->name);
    if (tpl_fn) {
      for (const auto& pname : get_template_param_order(
               tpl_fn, &ded_it->second.bindings, &ded_it->second.nttp_bindings)) {
        auto bit = ded_it->second.bindings.find(pname);
        if (bit != ded_it->second.bindings.end())
          tci.template_args.push_back(bit->second);
      }
      tci.nttp_args = ded_it->second.nttp_bindings;
    }
    c.template_info = std::move(tci);
  } else {
    if (n->left && n->left->kind == NK_VAR && n->left->name &&
        ref_overload_set_.count(n->left->name)) {
      resolved_callee_name = resolve_ref_overload(n->left->name, n);
      DeclRef dr{};
      dr.name = resolved_callee_name;
      auto fit = module_->fn_index.find(dr.name);
      if (fit != module_->fn_index.end() &&
          fit->second.value < module_->functions.size()) {
        TypeSpec fn_ts = module_->functions[fit->second.value].return_type.spec;
        fn_ts.ptr_level++;
        c.callee = append_expr(n->left, dr, fn_ts);
      } else {
        c.callee = append_expr(n->left, dr, n->left->type);
      }
    } else {
      bool resolved_as_method = false;
      if (ctx && !ctx->method_struct_tag.empty() &&
          n->left && n->left->kind == NK_VAR && n->left->name) {
        const std::string callee_name = n->left->name;
        if (!direct_callee_fn(callee_name)) {
          std::string base_key = ctx->method_struct_tag + "::" + callee_name;
          std::string const_key = base_key + "_const";
          auto mit = struct_methods_.find(base_key);
          if (mit == struct_methods_.end())
            mit = struct_methods_.find(const_key);
          if (mit != struct_methods_.end()) {
            resolved_callee_name = mit->second;
            DeclRef dr{};
            dr.name = resolved_callee_name;
            auto fit = module_->fn_index.find(dr.name);
            TypeSpec fn_ts{};
            fn_ts.base = TB_VOID;
            if (fit != module_->fn_index.end() &&
                fit->second.value < module_->functions.size()) {
              fn_ts = module_->functions[fit->second.value].return_type.spec;
            }
            fn_ts.ptr_level++;
            c.callee = append_expr(n->left, dr, fn_ts);
            auto pit = ctx->params.find("this");
            if (pit != ctx->params.end()) {
              DeclRef this_ref{};
              this_ref.name = "this";
              this_ref.param_index = pit->second;
              TypeSpec this_ts{};
              this_ts.base = TB_STRUCT;
              auto sit = module_->struct_defs.find(ctx->method_struct_tag);
              this_ts.tag = sit != module_->struct_defs.end()
                                ? sit->second.tag.c_str()
                                : ctx->method_struct_tag.c_str();
              this_ts.ptr_level = 1;
              c.args.push_back(append_expr(n->left, this_ref, this_ts, ValueCategory::LValue));
            }
            resolved_as_method = true;
          }
        }
      }
      if (!resolved_as_method)
        c.callee = lower_expr(ctx, n->left);
    }
  }
  c.builtin_id = n->builtin_id;
  const Function* callee_fn = !resolved_callee_name.empty()
                                  ? direct_callee_fn(resolved_callee_name)
                                  : (n->left && n->left->kind == NK_VAR && n->left->name
                                         ? direct_callee_fn(n->left->name)
                                         : nullptr);
  for (int i = 0; i < n->n_children; ++i) {
    const TypeSpec* param_ts =
        (callee_fn && static_cast<size_t>(i) < callee_fn->params.size())
            ? &callee_fn->params[static_cast<size_t>(i)].type.spec
            : nullptr;
    c.args.push_back(maybe_lower_ref_arg(n->children[i], param_ts));
  }
  TypeSpec ts = n->type;
  if (n->builtin_id != BuiltinId::Unknown) {
    bool known = false;
    TypeSpec builtin_ts = classify_known_builtin_return_type(n->builtin_id, &known);
    if (known) ts = builtin_ts;
  } else if (auto inferred = infer_call_result_type_from_callee(ctx, n->left)) {
    ts = *inferred;
  } else if (n->left && n->left->kind == NK_VAR && n->left->name) {
    bool known = false;
    TypeSpec kts = classify_known_call_return_type(n->left->name, &known);
    if (known) ts = kts;
  }
  return append_expr(n, c, ts);
}

ExprId Lowerer::try_lower_operator_call(FunctionCtx* ctx,
                                        const Node* result_node,
                                        const Node* obj_node,
                                        const char* op_method_name,
                                        const std::vector<const Node*>& arg_nodes,
                                        const std::vector<ExprId>& extra_args) {
  TypeSpec obj_ts = infer_generic_ctrl_type(ctx, obj_node);
  if (obj_ts.ptr_level != 0 || obj_ts.base != TB_STRUCT || !obj_ts.tag) {
    return ExprId::invalid();
  }

  std::optional<std::string> resolved =
      find_struct_method_mangled(obj_ts.tag, op_method_name, obj_ts.is_const);
  if (!resolved) return ExprId::invalid();

  std::string resolved_mangled = *resolved;
  std::string base_key = std::string(obj_ts.tag) + "::" + op_method_name;
  auto ovit = ref_overload_set_.find(base_key);
  if (ovit != ref_overload_set_.end() && !ovit->second.empty()) {
    const auto& overloads = ovit->second;
    const std::string* best_name = nullptr;
    int best_score = -1;
    for (const auto& ov : overloads) {
      bool viable = true;
      int score = 0;
      for (size_t i = 0; i < arg_nodes.size() && i < ov.param_is_rvalue_ref.size(); ++i) {
        bool arg_is_lvalue = is_ast_lvalue(arg_nodes[i]);
        if (ov.param_is_lvalue_ref[i] && !arg_is_lvalue) {
          viable = false;
          break;
        }
        if (ov.param_is_rvalue_ref[i] && arg_is_lvalue) {
          viable = false;
          break;
        }
        if (ov.param_is_rvalue_ref[i] && !arg_is_lvalue) score += 2;
        else if (ov.param_is_lvalue_ref[i] && arg_is_lvalue) score += 2;
        else score += 1;
      }
      if (viable && score > best_score) {
        best_name = &ov.mangled_name;
        best_score = score;
      }
    }
    if (best_name) resolved_mangled = *best_name;
  }

  CallExpr c{};
  DeclRef dr{};
  dr.name = resolved_mangled;
  auto fit = module_->fn_index.find(dr.name);
  TypeSpec fn_ts{};
  fn_ts.base = TB_VOID;
  if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
    fn_ts = module_->functions[fit->second.value].return_type.spec;
  }
  if (fn_ts.base == TB_VOID) {
    if (auto rit = find_struct_method_return_type(obj_ts.tag, op_method_name, obj_ts.is_const)) {
      fn_ts = *rit;
    }
  }
  TypeSpec callee_ts = fn_ts;
  callee_ts.ptr_level++;
  c.callee = append_expr(result_node, dr, callee_ts);

  ExprId obj_id = lower_expr(ctx, obj_node);
  if (ctx && module_->expr_pool[obj_id.value].type.category != ValueCategory::LValue) {
    LocalDecl tmp{};
    tmp.id = next_local_id();
    std::string tmp_name = "__op_call_tmp_" + std::to_string(tmp.id.value);
    tmp.name = tmp_name;
    tmp.type = qtype_from(obj_ts, ValueCategory::RValue);
    const TypeSpec init_ts = module_->expr_pool[obj_id.value].type.spec;
    if (init_ts.base == obj_ts.base && init_ts.ptr_level == obj_ts.ptr_level &&
        init_ts.tag == obj_ts.tag) {
      tmp.init = obj_id;
    }
    const LocalId tmp_lid = tmp.id;
    append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(obj_node)});

    DeclRef tmp_ref{};
    tmp_ref.name = tmp_name;
    tmp_ref.local = tmp_lid;
    obj_id = append_expr(obj_node, tmp_ref, obj_ts, ValueCategory::LValue);
  }
  UnaryExpr addr{};
  addr.op = UnaryOp::AddrOf;
  addr.operand = obj_id;
  TypeSpec ptr_ts = obj_ts;
  ptr_ts.ptr_level++;
  c.args.push_back(append_expr(obj_node, addr, ptr_ts));

  const Function* callee_fn = nullptr;
  if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
    callee_fn = &module_->functions[fit->second.value];
  }
  const Node* method_ast = nullptr;
  if (!callee_fn) {
    for (const auto& pm : pending_methods_) {
      if (pm.mangled == resolved_mangled) {
        method_ast = pm.method_node;
        break;
      }
    }
  }

  auto lower_operator_ref_arg = [&](const Node* arg_node, const TypeSpec* param_ts) -> ExprId {
    if (!param_ts || (!param_ts->is_lvalue_ref && !param_ts->is_rvalue_ref)) {
      return lower_expr(ctx, arg_node);
    }
    TypeSpec storage_ts = reference_storage_ts(*param_ts);
    if (param_ts->is_rvalue_ref) {
      if (arg_node && arg_node->kind == NK_CAST && arg_node->type.is_rvalue_ref &&
          arg_node->left) {
        UnaryExpr addr_e{};
        addr_e.op = UnaryOp::AddrOf;
        addr_e.operand = lower_expr(ctx, arg_node->left);
        return append_expr(arg_node, addr_e, storage_ts);
      }
      ExprId arg_val = lower_expr(ctx, arg_node);
      TypeSpec val_ts = reference_value_ts(*param_ts);
      resolve_typedef_to_struct(val_ts);
      LocalDecl tmp{};
      tmp.id = next_local_id();
      tmp.name = "__rref_arg_tmp";
      tmp.type = qtype_from(val_ts, ValueCategory::LValue);
      tmp.init = arg_val;
      const LocalId tmp_lid = tmp.id;
      ctx->locals[tmp.name] = tmp.id;
      ctx->local_types[tmp.id.value] = val_ts;
      append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(arg_node)});
      DeclRef tmp_ref{};
      tmp_ref.name = "__rref_arg_tmp";
      tmp_ref.local = tmp_lid;
      ExprId var_id = append_expr(arg_node, tmp_ref, val_ts, ValueCategory::LValue);
      UnaryExpr addr_e{};
      addr_e.op = UnaryOp::AddrOf;
      addr_e.operand = var_id;
      return append_expr(arg_node, addr_e, storage_ts);
    }
    UnaryExpr addr_e{};
    addr_e.op = UnaryOp::AddrOf;
    addr_e.operand = lower_expr(ctx, arg_node);
    return append_expr(arg_node, addr_e, storage_ts);
  };

  for (size_t i = 0; i < arg_nodes.size(); ++i) {
    const TypeSpec* param_ts = (callee_fn && (i + 1) < callee_fn->params.size())
                                   ? &callee_fn->params[i + 1].type.spec
                                   : nullptr;
    TypeSpec ast_param_ts{};
    if (!param_ts && method_ast && static_cast<int>(i) < method_ast->n_params) {
      ast_param_ts = method_ast->params[i]->type;
      param_ts = &ast_param_ts;
    }
    if (param_ts && (param_ts->is_rvalue_ref || param_ts->is_lvalue_ref)) {
      c.args.push_back(lower_operator_ref_arg(arg_nodes[i], param_ts));
    } else {
      c.args.push_back(lower_expr(ctx, arg_nodes[i]));
    }
  }
  for (auto ea : extra_args) c.args.push_back(ea);

  return append_expr(result_node, c, fn_ts);
}

ExprId Lowerer::maybe_bool_convert(FunctionCtx* ctx, ExprId expr, const Node* n) {
  if (!expr.valid() || !n) return expr;
  TypeSpec ts = infer_generic_ctrl_type(ctx, n);
  if (ts.ptr_level != 0 || ts.base != TB_STRUCT || !ts.tag) return expr;
  std::string base_key = std::string(ts.tag) + "::operator_bool";
  std::string const_key = base_key + "_const";
  auto mit = struct_methods_.find(base_key);
  if (mit == struct_methods_.end()) mit = struct_methods_.find(const_key);
  if (mit == struct_methods_.end()) return expr;

  CallExpr c{};
  DeclRef dr{};
  dr.name = mit->second;
  auto fit = module_->fn_index.find(dr.name);
  TypeSpec fn_ts{};
  fn_ts.base = TB_BOOL;
  if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
    fn_ts = module_->functions[fit->second.value].return_type.spec;
  }
  TypeSpec callee_ts = fn_ts;
  callee_ts.ptr_level++;
  c.callee = append_expr(n, dr, callee_ts);

  UnaryExpr addr{};
  addr.op = UnaryOp::AddrOf;
  addr.operand = expr;
  TypeSpec ptr_ts = ts;
  ptr_ts.ptr_level++;
  c.args.push_back(append_expr(n, addr, ptr_ts));

  return append_expr(n, c, fn_ts);
}

TypeSpec Lowerer::builtin_query_result_type() const {
  TypeSpec ts{};
  ts.base = TB_ULONG;
  return ts;
}

TypeSpec Lowerer::resolve_builtin_query_type(FunctionCtx* ctx, TypeSpec target) const {
  if (!ctx || ctx->tpl_bindings.empty() ||
      target.base != TB_TYPEDEF || !target.tag) {
    return target;
  }
  auto it = ctx->tpl_bindings.find(target.tag);
  if (it == ctx->tpl_bindings.end()) return target;
  const TypeSpec& concrete = it->second;
  target.base = concrete.base;
  target.tag = concrete.tag;
  if (concrete.ptr_level > 0) target.ptr_level += concrete.ptr_level;
  return target;
}

ExprId Lowerer::lower_builtin_sizeof_type(FunctionCtx* ctx, const Node* n) {
  const LayoutQueries queries(*module_);
  const TypeSpec sizeof_target = resolve_builtin_query_type(ctx, n->type);
  const TypeSpec result_ts = builtin_query_result_type();
  if (ctx && sizeof_target.array_rank > 0 && n->type.array_size_expr &&
      (sizeof_target.array_size <= 0 || sizeof_target.array_dims[0] <= 0)) {
    TypeSpec elem_ts = sizeof_target;
    elem_ts.array_rank--;
    if (elem_ts.array_rank > 0) {
      for (int i = 0; i < elem_ts.array_rank; ++i) {
        elem_ts.array_dims[i] = elem_ts.array_dims[i + 1];
      }
    }
    elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;

    const ExprId count_id = lower_expr(ctx, n->type.array_size_expr);
    const ExprId elem_size_id = append_expr(
        n, IntLiteral{static_cast<long long>(queries.type_size_bytes(elem_ts)), false},
        result_ts);
    BinaryExpr mul{};
    mul.op = BinaryOp::Mul;
    mul.lhs = count_id;
    mul.rhs = elem_size_id;
    return append_expr(n, mul, result_ts);
  }

  const int size = queries.type_size_bytes(sizeof_target);
  return append_expr(n, IntLiteral{static_cast<long long>(size), false}, result_ts);
}

ExprId Lowerer::lower_builtin_alignof_type(FunctionCtx* ctx, const Node* n) {
  const LayoutQueries queries(*module_);
  const TypeSpec alignof_target = resolve_builtin_query_type(ctx, n->type);
  const int align = queries.type_align_bytes(alignof_target);
  return append_expr(
      n, IntLiteral{static_cast<long long>(align), false}, builtin_query_result_type());
}

int Lowerer::builtin_alignof_expr_bytes(FunctionCtx* ctx, const Node* expr) {
  const LayoutQueries queries(*module_);
  const TypeSpec expr_ts = infer_generic_ctrl_type(ctx, expr);
  int align = 0;
  if (expr && expr->kind == NK_VAR && expr->name) {
    const std::string fn_name(expr->name);
    auto fit = module_->fn_index.find(fn_name);
    if (fit != module_->fn_index.end() &&
        fit->second.value < module_->functions.size()) {
      int fn_align = module_->functions[fit->second.value].attrs.align_bytes;
      if (fn_align > 0) align = fn_align;
    }
  }
  if (align != 0) return align;
  if (expr_ts.align_bytes > 0) return expr_ts.align_bytes;
  return queries.type_align_bytes(expr_ts);
}

ExprId Lowerer::lower_builtin_alignof_expr(FunctionCtx* ctx, const Node* n) {
  const int align = builtin_alignof_expr_bytes(ctx, n->left);
  return append_expr(
      n, IntLiteral{static_cast<long long>(align), false}, builtin_query_result_type());
}
#endif


void Lowerer::lower_function(const Node* fn_node,
                             const std::string* name_override,
                             const TypeBindings* tpl_override,
                             const NttpBindings* nttp_override) {
  Function fn{};
  fn.id = next_fn_id();
  fn.name = name_override ? *name_override
                          : (fn_node->name ? fn_node->name : "<anon_fn>");
  fn.ns_qual = make_ns_qual(fn_node);
  {
    TypeSpec ret_ts = fn_node->type;
    if ((fn_node->n_ret_fn_ptr_params > 0 || fn_node->ret_fn_ptr_variadic) &&
        !ret_ts.is_fn_ptr) {
      ret_ts.is_fn_ptr = true;
      ret_ts.ptr_level = std::max(ret_ts.ptr_level, 1);
    }
    ret_ts = prepare_callable_return_type(
        ret_ts, tpl_override, nttp_override, fn_node,
        std::string("function-return:") + fn.name, false);
    fn.return_type = qtype_from(ret_ts);
  }
  if (fn_node->type.is_fn_ptr ||
      fn_node->n_ret_fn_ptr_params > 0 ||
      fn_node->ret_fn_ptr_variadic) {
    auto fn_ct = sema::canonicalize_declarator_type(fn_node);
    const auto* fn_fsig = sema::get_function_sig(fn_ct);
    if (fn_fsig && fn_fsig->return_type &&
        sema::is_callable_type(*fn_fsig->return_type)) {
      const auto* ret_fsig = sema::get_function_sig(*fn_fsig->return_type);
      if (ret_fsig) {
        FnPtrSig ret_sig{};
        ret_sig.canonical_sig = fn_fsig->return_type;
        if (ret_fsig->return_type) {
          TypeSpec ret_ts = sema::typespec_from_canonical(*ret_fsig->return_type);
          if ((fn_node->n_ret_fn_ptr_params > 0 || fn_node->ret_fn_ptr_variadic) &&
              !ret_ts.is_fn_ptr) {
            ret_ts.is_fn_ptr = true;
            ret_ts.ptr_level = std::max(ret_ts.ptr_level, 1);
          }
          ret_sig.return_type = qtype_from(ret_ts);
        }
        ret_sig.variadic = ret_fsig->is_variadic;
        ret_sig.unspecified_params = ret_fsig->unspecified_params;
        for (const auto& param : ret_fsig->params) {
          ret_sig.params.push_back(
              qtype_from(sema::typespec_from_canonical(param),
                         ValueCategory::LValue));
        }
        if (ret_sig.params.empty() &&
            (fn_node->n_ret_fn_ptr_params > 0 || fn_node->ret_fn_ptr_variadic)) {
          ret_sig.variadic = fn_node->ret_fn_ptr_variadic;
          ret_sig.unspecified_params = false;
          for (int i = 0; i < fn_node->n_ret_fn_ptr_params; ++i) {
            const Node* param = fn_node->ret_fn_ptr_params[i];
            ret_sig.params.push_back(
                qtype_from(param->type, ValueCategory::LValue));
          }
        }
        fn.ret_fn_ptr_sig = std::move(ret_sig);
      }
    }
    if (!fn.ret_fn_ptr_sig &&
        (fn_node->n_ret_fn_ptr_params > 0 || fn_node->ret_fn_ptr_variadic)) {
      FnPtrSig ret_sig{};
      ret_sig.return_type = qtype_from(fn.return_type.spec);
      ret_sig.variadic = fn_node->ret_fn_ptr_variadic;
      ret_sig.unspecified_params = false;
      for (int i = 0; i < fn_node->n_ret_fn_ptr_params; ++i) {
        const Node* param = fn_node->ret_fn_ptr_params[i];
        ret_sig.params.push_back(qtype_from(param->type, ValueCategory::LValue));
      }
      fn.ret_fn_ptr_sig = std::move(ret_sig);
    }
  }
  fn.linkage = {fn_node->is_static,
                fn_node->is_extern || fn_node->body == nullptr,
                fn_node->is_inline,
                weak_symbols_.count(fn.name) > 0,
                static_cast<Visibility>(fn_node->visibility)};
  fn.attrs.variadic = fn_node->variadic;
  fn.attrs.no_inline = fn_node->type.is_noinline;
  fn.attrs.always_inline = fn_node->type.is_always_inline;
  if (fn_node->type.align_bytes > 0) {
    fn.attrs.align_bytes = fn_node->type.align_bytes;
  }
  if (fn.attrs.align_bytes == 0) {
    auto fit = module_->fn_index.find(fn.name);
    if (fit != module_->fn_index.end() &&
        fit->second.value < module_->functions.size()) {
      int prev_align = module_->functions[fit->second.value].attrs.align_bytes;
      if (prev_align > 0) {
        fn.attrs.align_bytes = prev_align;
      }
    }
  }
  fn.span = make_span(fn_node);

  FunctionCtx ctx{};
  ctx.fn = &fn;
  if (tpl_override) {
    ctx.tpl_bindings = *tpl_override;
  }
  if (nttp_override) {
    ctx.nttp_bindings = *nttp_override;
  }
  append_callable_params(
      fn, ctx, fn_node, tpl_override, nttp_override, "function-param:", false,
      true);

  if (maybe_register_bodyless_callable(&fn, fn_node->body != nullptr)) {
    return;
  }

  const BlockId entry = begin_callable_body_lowering(fn, ctx);

  for (int i = 0; i < fn_node->n_params; ++i) {
    const Node* p = fn_node->params[i];
    if (!p) continue;
    if (p->type.array_rank <= 0 || !p->type.array_size_expr) continue;
    if (p->type.array_size > 0 && p->type.array_dims[0] > 0) continue;
    ExprStmt side_effect{};
    side_effect.expr = lower_expr(&ctx, p->type.array_size_expr);
    append_stmt(ctx, Stmt{StmtPayload{side_effect}, make_span(p)});
  }

  lower_stmt_node(ctx, fn_node->body);
  finish_lowered_callable(&fn, entry);
}

void Lowerer::lower_struct_method(const std::string& mangled_name,
                                  const std::string& struct_tag,
                                  const Node* method_node,
                                  const TypeBindings* tpl_bindings,
                                  const NttpBindings* nttp_bindings) {
  if (method_node->is_deleted) return;
  Function fn{};
  fn.id = next_fn_id();
  fn.name = mangled_name;
  fn.ns_qual = make_ns_qual(method_node);
  {
    TypeSpec ret_ts = prepare_callable_return_type(
        method_node->type, tpl_bindings, nttp_bindings, method_node,
        std::string("method-return:") + mangled_name, true);
    fn.return_type = qtype_from(ret_ts);
  }
  fn.linkage = {true, false, false, false, Visibility::Default};
  fn.attrs.variadic = method_node->variadic;
  fn.span = make_span(method_node);

  FunctionCtx ctx{};
  ctx.fn = &fn;
  if (tpl_bindings) ctx.tpl_bindings = *tpl_bindings;
  if (nttp_bindings) ctx.nttp_bindings = *nttp_bindings;

  {
    Param this_param{};
    this_param.name = "this";
    TypeSpec this_ts{};
    this_ts.base = TB_STRUCT;
    auto sit = module_->struct_defs.find(struct_tag);
    this_ts.tag = sit != module_->struct_defs.end() ? sit->second.tag.c_str()
                                                    : struct_tag.c_str();
    this_ts.ptr_level = 1;
    this_param.type = qtype_from(this_ts, ValueCategory::LValue);
    this_param.span = make_span(method_node);
    ctx.params[this_param.name] = static_cast<uint32_t>(fn.params.size());
    fn.params.push_back(std::move(this_param));
  }

  append_callable_params(
      fn, ctx, method_node, tpl_bindings, nttp_bindings, "method-param:", true,
      false);
  ctx.method_struct_tag = struct_tag;

  if (maybe_register_bodyless_callable(
          &fn, method_node->body != nullptr || method_node->is_defaulted)) {
    return;
  }

  const BlockId entry = begin_callable_body_lowering(fn, ctx);

  if (method_node->is_defaulted) {
    emit_defaulted_method_body(ctx, fn, struct_tag, method_node);
    if (method_node->is_destructor) {
      DeclRef this_ref{};
      this_ref.name = "this";
      auto pit = ctx.params.find("this");
      if (pit != ctx.params.end()) this_ref.param_index = pit->second;
      TypeSpec this_ts{};
      this_ts.base = TB_STRUCT;
      auto sit2 = module_->struct_defs.find(struct_tag);
      this_ts.tag = sit2 != module_->struct_defs.end()
                        ? sit2->second.tag.c_str()
                        : struct_tag.c_str();
      this_ts.ptr_level = 1;
      ExprId this_ptr = append_expr(
          method_node, this_ref, this_ts, ValueCategory::LValue);
      emit_member_dtor_calls(ctx, struct_tag, this_ptr, method_node);
    }
    if (method_node->is_destructor) {
      ReturnStmt rs{};
      append_stmt(ctx, Stmt{StmtPayload{rs}, make_span(method_node)});
    }
    finish_lowered_callable(&fn, entry);
    return;
  }

  if (method_node->is_constructor && method_node->n_ctor_inits > 0) {
    auto sit = module_->struct_defs.find(struct_tag);
    for (int i = 0; i < method_node->n_ctor_inits; ++i) {
      const char* mem_name = method_node->ctor_init_names[i];
      int nargs = method_node->ctor_init_nargs[i];
      Node** args = method_node->ctor_init_args[i];

      if (mem_name == struct_tag || (mem_name && struct_tag == mem_name)) {
        auto cit = struct_constructors_.find(struct_tag);
        if (cit == struct_constructors_.end() || cit->second.empty()) {
          throw std::runtime_error(
              "error: no constructors found for delegating constructor call to '" +
              struct_tag + "'");
        }
        const CtorOverload* best = nullptr;
        if (cit->second.size() == 1 &&
            cit->second[0].method_node->n_params == nargs) {
          best = &cit->second[0];
        } else {
          int best_score = -1;
          for (const auto& ov : cit->second) {
            if (ov.method_node->n_params != nargs) continue;
            if (ov.mangled_name == mangled_name) continue;
            int score = 0;
            bool viable = true;
            for (int pi = 0; pi < nargs && viable; ++pi) {
              TypeSpec param_ts = ov.method_node->params[pi]->type;
              resolve_typedef_to_struct(param_ts);
              TypeSpec arg_ts = infer_generic_ctrl_type(&ctx, args[pi]);
              bool arg_is_lvalue = is_ast_lvalue(args[pi]);
              TypeSpec param_base = param_ts;
              param_base.is_lvalue_ref = false;
              param_base.is_rvalue_ref = false;
              if (param_base.base == arg_ts.base &&
                  param_base.ptr_level == arg_ts.ptr_level) {
                score += 2;
                if (param_ts.is_rvalue_ref && !arg_is_lvalue) score += 4;
                else if (param_ts.is_rvalue_ref && arg_is_lvalue) viable = false;
                else if (param_ts.is_lvalue_ref && arg_is_lvalue) score += 3;
                else if (param_ts.is_lvalue_ref && !arg_is_lvalue) score += 1;
              } else if (param_base.ptr_level == 0 && arg_ts.ptr_level == 0 &&
                         param_base.base != TB_STRUCT &&
                         arg_ts.base != TB_STRUCT) {
                score += 1;
              } else {
                viable = false;
              }
            }
            if (viable && score > best_score) {
              best_score = score;
              best = &ov;
            }
          }
        }
        if (!best) {
          throw std::runtime_error(
              "error: no matching constructor for delegating constructor call to '" +
              struct_tag + "'");
        }
        if (best->method_node->is_deleted) {
          throw std::runtime_error(
              "error: call to deleted constructor '" + struct_tag + "'");
        }
        DeclRef this_ref{};
        this_ref.name = "this";
        auto pit = ctx.params.find("this");
        if (pit != ctx.params.end()) this_ref.param_index = pit->second;
        TypeSpec this_ts{};
        this_ts.base = TB_STRUCT;
        this_ts.tag = struct_tag.c_str();
        this_ts.ptr_level = 1;
        ExprId this_id = append_expr(
            method_node, this_ref, this_ts, ValueCategory::LValue);

        CallExpr c{};
        DeclRef callee_ref{};
        callee_ref.name = best->mangled_name;
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        c.callee = append_expr(method_node, callee_ref, callee_ts);
        c.args.push_back(this_id);
        for (int ai = 0; ai < nargs; ++ai) {
          TypeSpec param_ts = best->method_node->params[ai]->type;
          resolve_typedef_to_struct(param_ts);
          if (param_ts.is_rvalue_ref || param_ts.is_lvalue_ref) {
            ExprId arg_val = lower_expr(&ctx, args[ai]);
            TypeSpec storage_ts = reference_storage_ts(param_ts);
            UnaryExpr addr_e{};
            addr_e.op = UnaryOp::AddrOf;
            addr_e.operand = arg_val;
            c.args.push_back(append_expr(args[ai], addr_e, storage_ts));
          } else {
            c.args.push_back(lower_expr(&ctx, args[ai]));
          }
        }
        ExprId call_id = append_expr(method_node, c, fn_ts);
        ExprStmt es{};
        es.expr = call_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
        continue;
      }

      DeclRef this_ref{};
      this_ref.name = "this";
      auto pit = ctx.params.find("this");
      if (pit != ctx.params.end()) this_ref.param_index = pit->second;
      TypeSpec this_ts{};
      this_ts.base = TB_STRUCT;
      this_ts.tag = sit != module_->struct_defs.end() ? sit->second.tag.c_str()
                                                      : struct_tag.c_str();
      this_ts.ptr_level = 1;
      ExprId this_id = append_expr(
          method_node, this_ref, this_ts, ValueCategory::LValue);
      TypeSpec field_ts{};
      if (sit != module_->struct_defs.end()) {
        for (const auto& fld : sit->second.fields) {
          if (fld.name == mem_name) {
            field_ts = fld.elem_type;
            if (fld.array_first_dim >= 0) {
              field_ts.array_rank = 1;
              field_ts.array_size = fld.array_first_dim;
            }
            break;
          }
        }
      }
      MemberExpr me{};
      me.base = this_id;
      me.field = mem_name;
      me.is_arrow = true;
      ExprId lhs_id = append_expr(method_node, me, field_ts, ValueCategory::LValue);

      bool did_ctor_call = false;
      if (field_ts.base == TB_STRUCT && field_ts.tag && field_ts.ptr_level == 0) {
        auto cit = struct_constructors_.find(field_ts.tag);
        if (cit != struct_constructors_.end() && !cit->second.empty()) {
          const CtorOverload* best = nullptr;
          if (cit->second.size() == 1 &&
              cit->second[0].method_node->n_params == nargs) {
            best = &cit->second[0];
          } else {
            int best_score = -1;
            for (const auto& ov : cit->second) {
              if (ov.method_node->n_params != nargs) continue;
              int score = 0;
              bool viable = true;
              for (int pi = 0; pi < nargs && viable; ++pi) {
                TypeSpec param_ts = ov.method_node->params[pi]->type;
                resolve_typedef_to_struct(param_ts);
                TypeSpec arg_ts = infer_generic_ctrl_type(&ctx, args[pi]);
                bool arg_is_lvalue = is_ast_lvalue(args[pi]);
                TypeSpec param_base = param_ts;
                param_base.is_lvalue_ref = false;
                param_base.is_rvalue_ref = false;
                if (param_base.base == arg_ts.base &&
                    param_base.ptr_level == arg_ts.ptr_level) {
                  score += 2;
                  if (param_ts.is_rvalue_ref && !arg_is_lvalue) score += 4;
                  else if (param_ts.is_rvalue_ref && arg_is_lvalue) viable = false;
                  else if (param_ts.is_lvalue_ref && arg_is_lvalue) score += 3;
                  else if (param_ts.is_lvalue_ref && !arg_is_lvalue) score += 1;
                } else if (param_base.ptr_level == 0 && arg_ts.ptr_level == 0 &&
                           param_base.base != TB_STRUCT &&
                           arg_ts.base != TB_STRUCT) {
                  score += 1;
                } else {
                  viable = false;
                }
              }
              if (viable && score > best_score) {
                best_score = score;
                best = &ov;
              }
            }
          }
          if (best) {
            if (best->method_node->is_deleted) {
              std::string diag = "error: call to deleted constructor '";
              diag += field_ts.tag;
              diag += "'";
              throw std::runtime_error(diag);
            }
            CallExpr c{};
            DeclRef callee_ref{};
            callee_ref.name = best->mangled_name;
            TypeSpec fn_ts{};
            fn_ts.base = TB_VOID;
            TypeSpec callee_ts = fn_ts;
            callee_ts.ptr_level++;
            c.callee = append_expr(method_node, callee_ref, callee_ts);
            UnaryExpr addr{};
            addr.op = UnaryOp::AddrOf;
            addr.operand = lhs_id;
            TypeSpec ptr_ts = field_ts;
            ptr_ts.ptr_level++;
            c.args.push_back(append_expr(method_node, addr, ptr_ts));
            for (int ai = 0; ai < nargs; ++ai) {
              TypeSpec param_ts = best->method_node->params[ai]->type;
              resolve_typedef_to_struct(param_ts);
              if (param_ts.is_rvalue_ref || param_ts.is_lvalue_ref) {
                ExprId arg_val = lower_expr(&ctx, args[ai]);
                TypeSpec storage_ts = reference_storage_ts(param_ts);
                UnaryExpr addr_e{};
                addr_e.op = UnaryOp::AddrOf;
                addr_e.operand = arg_val;
                c.args.push_back(append_expr(args[ai], addr_e, storage_ts));
              } else {
                c.args.push_back(lower_expr(&ctx, args[ai]));
              }
            }
            ExprId call_id = append_expr(method_node, c, fn_ts);
            ExprStmt es{};
            es.expr = call_id;
            append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
            did_ctor_call = true;
          }
        }
      }

      if (!did_ctor_call) {
        if (nargs != 1) {
          std::string diag = "error: initializer for scalar member '";
          diag += mem_name;
          diag += "' must have exactly one argument";
          throw std::runtime_error(diag);
        }
        ExprId rhs_id = lower_expr(&ctx, args[0]);
        AssignExpr ae{};
        ae.op = AssignOp::Set;
        ae.lhs = lhs_id;
        ae.rhs = rhs_id;
        ExprId ae_id = append_expr(method_node, ae, field_ts);
        ExprStmt es{};
        es.expr = ae_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
      }
    }
  }

  lower_stmt_node(ctx, method_node->body);

  if (method_node->is_destructor) {
    DeclRef this_ref{};
    this_ref.name = "this";
    auto pit = ctx.params.find("this");
    if (pit != ctx.params.end()) this_ref.param_index = pit->second;
    TypeSpec this_ts{};
    this_ts.base = TB_STRUCT;
    auto sit2 = module_->struct_defs.find(struct_tag);
    this_ts.tag = sit2 != module_->struct_defs.end()
                      ? sit2->second.tag.c_str()
                      : struct_tag.c_str();
    this_ts.ptr_level = 1;
    ExprId this_ptr = append_expr(
        method_node, this_ref, this_ts, ValueCategory::LValue);
    emit_member_dtor_calls(ctx, struct_tag, this_ptr, method_node);
  }

  finish_lowered_callable(&fn, entry);
}

}  // namespace c4c::hir
