#include "impl/hir_impl.hpp"
#include "consteval.hpp"
#include "impl/lowerer.hpp"

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
NamespaceQualifier make_ns_qual(const Node* n, TextTable* texts) {
  NamespaceQualifier q;
  if (!n) return q;
  q.is_global_qualified = n->is_global_qualified;
  q.context_id = n->namespace_context_id;
  for (int i = 0; i < n->n_qualifier_segments; ++i) {
    if (n->qualifier_segments[i]) {
      q.segments.emplace_back(n->qualifier_segments[i]);
      q.segment_text_ids.push_back(
          texts ? texts->intern(n->qualifier_segments[i]) : kInvalidText);
    }
  }
  return q;
}

TextId make_text_id(std::string_view text, TextTable* texts) {
  if (!texts || text.empty()) return kInvalidText;
  return texts->intern(text);
}

TextId make_unqualified_text_id(const Node* n, TextTable* texts) {
  if (!n || !texts) return kInvalidText;
  if (n->unqualified_name && n->unqualified_name[0]) {
    return make_text_id(n->unqualified_name, texts);
  }
  if (n->name && n->name[0]) {
    return make_text_id(n->name, texts);
  }
  return kInvalidText;
}

NamespaceQualifier make_type_ns_qual(const TypeSpec& ts) {
  NamespaceQualifier q;
  q.context_id = ts.namespace_context_id;
  q.is_global_qualified = ts.is_global_qualified;
  if (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) {
    q.segment_text_ids.assign(
        ts.qualifier_text_ids,
        ts.qualifier_text_ids + ts.n_qualifier_segments);
  }
  return q;
}

std::optional<HirRecordOwnerKey> make_record_owner_key_for_type(
    const TypeSpec& ts,
    TextTable* texts) {
  if (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) {
    NamespaceQualifier ns_qual;
    ns_qual.context_id = ts.record_def->namespace_context_id;
    ns_qual.is_global_qualified = ts.record_def->is_global_qualified;
    if (ts.record_def->qualifier_text_ids &&
        ts.record_def->n_qualifier_segments > 0) {
      ns_qual.segment_text_ids.assign(
          ts.record_def->qualifier_text_ids,
          ts.record_def->qualifier_text_ids + ts.record_def->n_qualifier_segments);
    } else if (texts && ts.record_def->qualifier_segments &&
               ts.record_def->n_qualifier_segments > 0) {
      for (int i = 0; i < ts.record_def->n_qualifier_segments; ++i) {
        ns_qual.segment_text_ids.push_back(
            make_text_id(ts.record_def->qualifier_segments[i], texts));
      }
    }
    TextId declaration_text_id = ts.record_def->unqualified_text_id;
    if (declaration_text_id == kInvalidText) {
      declaration_text_id = make_unqualified_text_id(ts.record_def, texts);
    }
    HirRecordOwnerKey key = make_hir_record_owner_key(ns_qual, declaration_text_id);
    if (hir_record_owner_key_has_complete_metadata(key)) return key;
  }
  if (ts.namespace_context_id >= 0 && ts.tag_text_id != kInvalidText) {
    HirRecordOwnerKey key =
        make_hir_record_owner_key(make_type_ns_qual(ts), ts.tag_text_id);
    if (hir_record_owner_key_has_complete_metadata(key)) return key;
  }
  return std::nullopt;
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
    const std::optional<HirRecordOwnerKey> a_key =
        make_record_owner_key_for_type(a, nullptr);
    const std::optional<HirRecordOwnerKey> b_key =
        make_record_owner_key_for_type(b, nullptr);
    if (a_key && b_key) return *a_key == *b_key;
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

  const HirStructDef* find_struct_def_for_layout_type(const TypeSpec& ts) const {
    if ((ts.base != TB_STRUCT && ts.base != TB_UNION) || ts.ptr_level != 0) {
      return nullptr;
    }
    if (const std::optional<HirRecordOwnerKey> owner_key =
            make_record_owner_key_for_type(
                ts, module_.link_name_texts ? module_.link_name_texts.get() : nullptr)) {
      return module_.find_struct_def_by_owner_structured(*owner_key);
    }
    if (!ts.tag || !ts.tag[0]) return nullptr;
    const auto it = module_.struct_defs.find(ts.tag);
    return it == module_.struct_defs.end() ? nullptr : &it->second;
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
      if (const HirStructDef* def = find_struct_def_for_layout_type(ts)) {
        return def->size_bytes;
      }
      return 4;
    }
    return sizeof_type_spec(ts);
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
      if (const HirStructDef* def = find_struct_def_for_layout_type(ts)) {
        natural = std::max(1, def->align_bytes);
      } else {
        natural = 4;
      }
    } else {
      natural = std::max(1, static_cast<int>(alignof_type_spec(ts)));
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

  for (size_t base_index = 0; base_index < def.base_tags.size(); ++base_index) {
    const auto& base_tag = def.base_tags[base_index];
    if (base_tag.empty()) continue;
    TypeSpec base_ts{};
    base_ts.base = TB_STRUCT;
    base_ts.tag = base_tag.c_str();
    if (base_index < def.base_tag_text_ids.size()) {
      base_ts.tag_text_id = def.base_tag_text_ids[base_index];
      base_ts.namespace_context_id = def.ns_qual.context_id;
      base_ts.is_global_qualified = def.ns_qual.is_global_qualified;
    }

    int base_align = queries.type_align_bytes(base_ts);
    if (pack > 0 && base_align > pack) base_align = pack;
    offset = align_to(offset, base_align);
    offset += queries.type_size_bytes(base_ts);
    def.align_bytes = std::max(def.align_bytes, base_align);
  }

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

  if (def.fields.empty() && def.base_tags.empty()) {
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

}  // namespace c4c::hir
