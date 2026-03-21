#include "ast_to_hir.hpp"
#include "consteval.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <functional>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "type_utils.hpp"
#include "../parser/parser_internal.hpp"

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

struct QualifiedMethodRef {
  std::string struct_tag;
  std::string method_name;
  std::string key;
};

std::optional<QualifiedMethodRef> try_parse_qualified_struct_method_name(
    const Node* fn, bool include_const_suffix = true) {
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

int struct_size_bytes(const hir::Module& module, const char* tag) {
  if (!tag || !tag[0]) return 4;
  const auto it = module.struct_defs.find(tag);
  if (it == module.struct_defs.end()) return 4;
  return it->second.size_bytes;
}

int struct_align_bytes(const hir::Module& module, const char* tag) {
  if (!tag || !tag[0]) return 4;
  const auto it = module.struct_defs.find(tag);
  if (it == module.struct_defs.end()) return 4;
  return std::max(1, it->second.align_bytes);
}

int type_size_bytes(const hir::Module& module, const TypeSpec& ts) {
  if (ts.array_rank > 0) {
    if (ts.array_size == 0) return 0;
    if (ts.array_size > 0) {
      TypeSpec elem = ts;
      elem.array_rank--;
      if (elem.array_rank > 0) {
        for (int i = 0; i < elem.array_rank; ++i) elem.array_dims[i] = elem.array_dims[i + 1];
      }
      elem.array_size = (elem.array_rank > 0) ? elem.array_dims[0] : -1;
      return static_cast<int>(ts.array_size) * type_size_bytes(module, elem);
    }
    return 8;
  }
  if (ts.is_vector && ts.vector_bytes > 0) return static_cast<int>(ts.vector_bytes);
  if (ts.ptr_level > 0 && ts.is_ptr_to_array) return 8;
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return 8;
  if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0) {
    return struct_size_bytes(module, ts.tag);
  }
  return sizeof_base(ts.base);
}

int type_align_bytes(const hir::Module& module, const TypeSpec& ts) {
  int natural = 1;
  if (ts.array_rank > 0) {
    TypeSpec elem = ts;
    elem.array_rank--;
    if (elem.array_rank > 0) {
      for (int i = 0; i < elem.array_rank; ++i) elem.array_dims[i] = elem.array_dims[i + 1];
    }
    elem.array_size = (elem.array_rank > 0) ? elem.array_dims[0] : -1;
    natural = type_align_bytes(module, elem);
  } else if (ts.is_vector && ts.vector_bytes > 0) {
    natural = static_cast<int>(ts.vector_bytes);
  } else if (ts.ptr_level > 0 || ts.is_fn_ptr) {
    natural = 8;
  } else if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0) {
    natural = struct_align_bytes(module, ts.tag);
  } else {
    natural = std::max(1, static_cast<int>(align_base(ts.base, ts.ptr_level)));
  }
  if (ts.align_bytes > 0) natural = std::max(natural, ts.align_bytes);
  return natural;
}

int field_size_bytes(const hir::Module& module, const HirStructField& f) {
  if (f.size_bytes > 0 || f.is_flexible_array) return f.size_bytes;
  return type_size_bytes(module, f.elem_type);
}

int field_align_bytes(const hir::Module& module, const HirStructField& f) {
  if (f.align_bytes > 0) return f.align_bytes;
  return type_align_bytes(module, f.elem_type);
}

void compute_struct_layout(hir::Module* module, HirStructDef& def) {
  if (!module) return;

  const int pack = def.pack_align;  // 0 = default, >0 = cap alignment

  for (auto& field : def.fields) {
    if (field.bit_width >= 0) {
      field.align_bytes = std::max(1, field.storage_unit_bits / 8);
      field.size_bytes = std::max(1, field.storage_unit_bits / 8);
    } else {
      TypeSpec field_ts = field.elem_type;
      if (field.array_first_dim >= 0) {
        field_ts.array_rank = std::max(field_ts.array_rank, 1);
        field_ts.array_size = field.array_first_dim;
        field_ts.array_dims[0] = field.array_first_dim;
      }
      field.align_bytes = type_align_bytes(*module, field_ts);
      field.size_bytes = type_size_bytes(*module, field_ts);
    }
    // Apply #pragma pack: cap field alignment to pack value
    if (pack > 0 && field.align_bytes > pack) {
      field.align_bytes = pack;
    }
  }

  if (def.is_union) {
    def.align_bytes = 1;
    def.size_bytes = 0;
    for (auto& field : def.fields) {
      field.offset_bytes = 0;
      int fa = field_align_bytes(*module, field);
      if (pack > 0 && fa > pack) fa = pack;
      def.align_bytes = std::max(def.align_bytes, fa);
      def.size_bytes = std::max(def.size_bytes, field_size_bytes(*module, field));
    }
    // __attribute__((aligned(N))) boosts struct alignment
    if (def.struct_align > 0)
      def.align_bytes = std::max(def.align_bytes, def.struct_align);
    def.size_bytes = align_to(def.size_bytes, def.align_bytes);
    return;
  }

  def.align_bytes = 1;
  int offset = 0;
  int last_llvm_idx = -1;
  int last_offset = 0;
  for (auto& field : def.fields) {
    if (field.llvm_idx != last_llvm_idx) {
      int field_align = field_align_bytes(*module, field);
      if (pack > 0 && field_align > pack) field_align = pack;
      offset = align_to(offset, field_align);
      last_offset = offset;
      offset += field_size_bytes(*module, field);
      last_llvm_idx = field.llvm_idx;
      def.align_bytes = std::max(def.align_bytes, field_align);
    }
    field.offset_bytes = last_offset;
  }
  // __attribute__((aligned(N))) boosts struct alignment
  if (def.struct_align > 0)
    def.align_bytes = std::max(def.align_bytes, def.struct_align);
  def.size_bytes = align_to(offset, def.align_bytes);
}

class Lowerer {
 public:
  const sema::ResolvedTypeTable* resolved_types_ = nullptr;

  /// Engine-owned compile-time state, shared with the pipeline.
  std::shared_ptr<CompileTimeState> ct_state() const { return ct_state_; }

  void lower_initial_program(const Node* root, Module& m) {
    if (!root || root->kind != NK_PROGRAM) {
      throw std::runtime_error("build_initial_hir: root is not NK_PROGRAM");
    }

    module_ = &m;

    // Recursively flatten top-level items (may be nested NK_BLOCKs from
    // nested namespace blocks).
    std::vector<const Node*> items;
    std::function<void(const Node*)> flatten = [&](const Node* n) {
      if (!n) return;
      if (n->kind == NK_BLOCK) {
        for (int j = 0; j < n->n_children; ++j)
          flatten(n->children[j]);
      } else {
        items.push_back(n);
      }
    };
    for (int i = 0; i < root->n_children; ++i)
      flatten(root->children[i]);

    // Phase 0: collect #pragma weak symbol names
    for (const Node* item : items) {
      if (item->kind == NK_PRAGMA_WEAK && item->name)
        weak_symbols_.insert(item->name);
    }

    // Phase 1: collect type definitions used by later lowering
    for (const Node* item : items) {
      if (item->kind == NK_STRUCT_DEF) {
        lower_struct_def(item);
        // Also collect template struct defs for deferred instantiation.
        if (item->n_template_params > 0 && item->name)
          template_struct_defs_[item->name] = item;
      }
      if (item->kind == NK_ENUM_DEF) collect_enum_def(item);
    }

    // Phase 1.5: collect consteval function definitions for compile-time evaluation
    for (const Node* item : items) {
      if (item->kind == NK_FUNCTION && item->is_consteval && item->name) {
        ct_state_->register_consteval_def(item->name, item);
      }
    }

    // Phase 1.6: collect template instantiation info from call sites.
    // For each template function, collect all unique sets of concrete template
    // arg types, so template functions that contain consteval calls with
    // template-dependent args can be lowered once per instantiation.
    for (const Node* item : items) {
      if (item->kind == NK_FUNCTION && item->name && item->n_template_params > 0) {
        ct_state_->register_template_def(item->name, item);
      }
    }
    // Collect explicit template specializations (template<> T foo<Args>(...)).
    for (const Node* item : items) {
      if (item->kind == NK_FUNCTION && item->name && item->is_explicit_specialization
          && item->n_template_args > 0) {
        const Node* tpl_def = ct_state_->find_template_def(item->name);
        if (tpl_def) {
          std::string mangled = mangle_specialization(item, tpl_def);
          if (!mangled.empty())
            registry_.register_specialization(mangled, item);
        }
      }
    }
    // Collect only depth-0 instantiations: direct template calls from
    // non-template functions (e.g., main() calling twice<int>()).
    // Nested template calls (e.g., add<T>() inside twice<T>()) will be
    // discovered and instantiated by the HIR compile-time reduction pass.
    for (const Node* item : items) {
      if (item->kind == NK_FUNCTION && item->body && item->n_template_params == 0)
        collect_template_instantiations(item->body, item);
    }
    // Realize seeds collected from non-template function bodies so that
    // instances are available for the consteval fixpoint loop below.
    registry_.realize_seeds();

    // Also collect consteval template calls from template function bodies,
    // since consteval is still evaluated eagerly during lowering and needs
    // all instantiation bindings to be available at lowering time.
    // This uses the fixpoint loop because consteval templates may chain.
    for (int pass = 0; pass < 8; ++pass) {
      size_t prev_size = registry_.total_instance_count();
      for (const Node* item : items) {
        if (item->kind == NK_FUNCTION && item->body && item->n_template_params > 0)
          collect_consteval_template_instantiations(item->body, item);
      }
      // Realize seeds discovered in this pass so enclosing-expansion
      // paths in the next pass can find_instances() for them.
      registry_.realize_seeds();
      if (registry_.total_instance_count() == prev_size) break;
    }

    // Final realize — catch any stragglers (e.g. seeds added by paths
    // that bypass the loops above).
    registry_.realize_seeds();

    // Verify seed/instance parity after all realization is done.
    if (!registry_.verify_parity()) {
      registry_.dump_parity(stderr);
      throw std::runtime_error(
          "InstantiationRegistry: seed/instance parity violation after "
          "realize_seeds()");
    }

    // Phase 1.7b: populate HirTemplateDef metadata for all template functions.
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

    // Phase 1.9: detect ref-overloaded free functions (e.g. f(T&) vs f(T&&)).
    // Build overload sets so Phase 2 can mangle them to unique names.
    {
      // Track first occurrence of each function name and its param ref pattern.
      std::unordered_map<std::string, const Node*> first_fn_decl;
      for (const Node* item : items) {
        if (item->kind != NK_FUNCTION || !item->name || item->is_consteval
            || item->n_template_params > 0 || item->is_explicit_specialization)
          continue;
        std::string fn_name = item->name;
        auto prev_it = first_fn_decl.find(fn_name);
        if (prev_it == first_fn_decl.end()) {
          first_fn_decl[fn_name] = item;
          continue;
        }
        // Check if the param lists differ only in ref-qualifiers.
        const Node* prev = prev_it->second;
        if (prev->n_params != item->n_params) continue;
        bool has_ref_diff = false;
        bool base_match = true;
        for (int pi = 0; pi < item->n_params; ++pi) {
          const TypeSpec& a = prev->params[pi]->type;
          const TypeSpec& b = item->params[pi]->type;
          if (a.is_lvalue_ref != b.is_lvalue_ref || a.is_rvalue_ref != b.is_rvalue_ref)
            has_ref_diff = true;
          // Compare base types (ignoring ref/const qualifiers).
          if (a.base != b.base || a.ptr_level != b.ptr_level) { base_match = false; break; }
          if ((a.base == TB_STRUCT || a.base == TB_UNION) && a.tag && b.tag) {
            if (std::string(a.tag) != std::string(b.tag)) { base_match = false; break; }
          }
        }
        if (!has_ref_diff || !base_match) continue;
        // This is a ref-overload pair. Register both in the overload set.
        auto& ovset = ref_overload_set_[fn_name];
        if (ovset.empty()) {
          // Register the first overload.
          RefOverloadEntry e0;
          e0.mangled_name = fn_name;
          for (int pi = 0; pi < prev->n_params; ++pi) {
            e0.param_is_rvalue_ref.push_back(prev->params[pi]->type.is_rvalue_ref);
            e0.param_is_lvalue_ref.push_back(prev->params[pi]->type.is_lvalue_ref);
          }
          ovset.push_back(std::move(e0));
          ref_overload_mangled_[prev] = fn_name;
        }
        // Register the new overload with a mangled name.
        RefOverloadEntry e1;
        e1.mangled_name = fn_name + "__rref_overload";
        for (int pi = 0; pi < item->n_params; ++pi) {
          e1.param_is_rvalue_ref.push_back(item->params[pi]->type.is_rvalue_ref);
          e1.param_is_lvalue_ref.push_back(item->params[pi]->type.is_lvalue_ref);
        }
        ovset.push_back(std::move(e1));
        ref_overload_mangled_[item] = fn_name + "__rref_overload";
      }
    }

    // Phase 1.95: attach out-of-class struct method definitions to the
    // pending method entries collected from their in-class declarations.
    for (const Node* item : items) {
      if (item->kind != NK_FUNCTION || !item->body) continue;
      auto method_ref = try_parse_qualified_struct_method_name(item);
      if (!method_ref.has_value()) continue;
      if (!m.struct_defs.count(method_ref->struct_tag)) continue;
      auto mit = struct_methods_.find(method_ref->key);
      if (mit == struct_methods_.end()) continue;
      for (auto& pm : pending_methods_) {
        if (pm.mangled == mit->second) {
          pm.method_node = item;
          break;
        }
      }
    }

    // Phase 2: lower functions and globals
    for (const Node* item : items) {
      if (item->kind == NK_FUNCTION) {
        auto method_ref = try_parse_qualified_struct_method_name(item);
        if (method_ref.has_value() &&
            m.struct_defs.count(method_ref->struct_tag) &&
            struct_methods_.count(method_ref->key)) {
          continue;
        }
        if (item->is_consteval && item->n_template_params == 0) {
          // Non-template consteval function: add as declaration-only to HIR
          // for analysis/debugging.  The body is not lowered (it's evaluated
          // by the AST-level consteval interpreter).  Marked consteval_only
          // so the materialization pass won't emit it.
          Function ce_fn{};
          ce_fn.id = next_fn_id();
          ce_fn.name = item->name ? item->name : "<anon_consteval>";
          ce_fn.ns_qual = make_ns_qual(item);
          ce_fn.return_type = qtype_from(item->type);
          ce_fn.consteval_only = true;
          ce_fn.span = make_span(item);
          for (int i = 0; i < item->n_params; ++i) {
            const Node* p = item->params[i];
            if (!p) continue;
            Param param{};
            param.name = p->name ? p->name : "<anon_param>";
            param.type = qtype_from(p->type, ValueCategory::LValue);
            param.span = make_span(p);
            ce_fn.params.push_back(std::move(param));
          }
          m.fn_index[ce_fn.name] = ce_fn.id;
          m.functions.push_back(std::move(ce_fn));
          continue;
        }
        if (item->is_consteval) continue;  // consteval template: handled by AST interpreter
        if (item->n_template_params > 0 && item->name) {
          // Template function: lower once per collected instantiation.
          auto* inst_list = registry_.find_instances(item->name);
          if (inst_list && !inst_list->empty()) {
            for (const auto& inst : *inst_list) {
              // Check for explicit specialization matching this instantiation.
              const Node* spec_node = registry_.find_specialization(inst.mangled_name);
              if (spec_node) {
                // Use the specialization body (no template bindings needed).
                lower_function(spec_node, &inst.mangled_name);
              } else {
                lower_function(item, &inst.mangled_name, &inst.bindings,
                               inst.nttp_bindings.empty() ? nullptr : &inst.nttp_bindings);
              }
              if (!m.functions.empty()) {
                m.functions.back().template_origin = item->name ? item->name : "";
                m.functions.back().spec_key = inst.spec_key;
              }
            }
          } else {
            // No explicit-arg call sites found.
            // If the function is called from a non-template function without
            // explicit template args (implicit deduction), it still needs
            // generic lowering.  Only skip if we know it will be deferred
            // (it's referenced only from other template functions with
            // explicit template args).
            if (!is_referenced_without_template_args(item->name, items))
              continue;  // Will be instantiated by the HIR compile-time pass.
            lower_function(item);
          }
        } else if (!item->is_explicit_specialization) {
          auto ovit = ref_overload_mangled_.find(item);
          if (ovit != ref_overload_mangled_.end()) {
            lower_function(item, &ovit->second);
          } else {
            lower_function(item);
          }
        }
      } else if (item->kind == NK_GLOBAL_VAR) {
        lower_global(item);
      }
    }

    // Phase 2.5: lower struct methods collected during Phase 1.
    for (const auto& pm : pending_methods_) {
      lower_struct_method(pm.mangled, pm.struct_tag, pm.method_node,
                          pm.tpl_bindings.empty() ? nullptr : &pm.tpl_bindings,
                          pm.nttp_bindings.empty() ? nullptr : &pm.nttp_bindings);
    }

    return;
  }

  bool instantiate_deferred_template(const std::string& tpl_name,
                                     const TypeBindings& bindings,
                                     const NttpBindings& nttp_bindings,
                                     const std::string& mangled) {
    // NOTE: consteval pre-check and post-instantiation metadata
    // (template_origin, spec_key) are now handled by the engine's
    // TemplateInstantiationStep.  This callback is a pure lowering
    // operation.
    const Node* fn_def = ct_state_->find_template_def(tpl_name);
    if (!fn_def) return false;
    const Node* spec_node = registry_.find_specialization(mangled);
    if (spec_node) {
      lower_function(spec_node, &mangled);
    } else {
      lowering_deferred_instantiation_ = true;
      const NttpBindings* nttp_ptr = nttp_bindings.empty() ? nullptr : &nttp_bindings;
      lower_function(fn_def, &mangled, &bindings, nttp_ptr);
      lowering_deferred_instantiation_ = false;
    }
    return true;
  }

 private:
  struct SwitchCtx {
    BlockId parent_block{};
    std::vector<std::pair<long long, BlockId>> cases;
    std::vector<std::tuple<long long, long long, BlockId>> case_ranges;
    std::optional<BlockId> default_block;
  };

  struct FunctionCtx {
    Function* fn = nullptr;
    std::unordered_map<std::string, LocalId> locals;
    std::unordered_map<uint32_t, TypeSpec> local_types;
    std::unordered_map<std::string, FnPtrSig> local_fn_ptr_sigs;
    std::unordered_map<std::string, FnPtrSig> param_fn_ptr_sigs;
    std::unordered_map<std::string, GlobalId> static_globals;
    std::unordered_map<std::string, uint32_t> params;
    BlockId current_block{};
    std::vector<BlockId> break_stack;
    std::vector<BlockId> continue_stack;
    std::unordered_map<std::string, BlockId> label_blocks;
    std::vector<SwitchCtx> switch_stack;
    std::unordered_map<std::string, long long> local_const_bindings;
    TypeBindings tpl_bindings;  // template param → concrete type for enclosing template fn
    NttpBindings nttp_bindings; // non-type template param → constant value
    std::string method_struct_tag; // non-empty when lowering a struct method body
    // Destructor tracking: records locals that need destructor calls at scope exit.
    struct DtorLocal {
      LocalId local_id;
      std::string struct_tag;
    };
    std::vector<DtorLocal> dtor_stack;
  };

  static bool is_lvalue_ref_ts(const TypeSpec& ts) {
    return ts.is_lvalue_ref;
  }

  static bool is_any_ref_ts(const TypeSpec& ts) {
    return ts.is_lvalue_ref || ts.is_rvalue_ref;
  }

  static TypeSpec reference_storage_ts(TypeSpec ts) {
    if (ts.is_lvalue_ref || ts.is_rvalue_ref) ts.ptr_level += 1;
    return ts;
  }

  static TypeSpec reference_value_ts(TypeSpec ts) {
    if (!ts.is_lvalue_ref && !ts.is_rvalue_ref) return ts;
    ts.is_lvalue_ref = false;
    ts.is_rvalue_ref = false;
    if (ts.ptr_level > 0) ts.ptr_level -= 1;
    return ts;
  }

  // Resolve TB_TYPEDEF to TB_STRUCT/TB_UNION when the tag matches a known
  // struct definition.  Handles the injected-class-name case where the parser
  // cannot fully resolve the typedef because the struct is still incomplete
  // during body parsing.
  void resolve_typedef_to_struct(TypeSpec& ts) const {
    if (ts.base != TB_TYPEDEF || !ts.tag) return;
    auto sit = module_->struct_defs.find(ts.tag);
    if (sit != module_->struct_defs.end()) {
      ts.base = TB_STRUCT;
      ts.tag = sit->second.tag.c_str();
    }
  }

  FunctionId next_fn_id() { return module_->alloc_function_id(); }
  GlobalId next_global_id() { return module_->alloc_global_id(); }
  LocalId next_local_id() { return module_->alloc_local_id(); }
  BlockId next_block_id() { return module_->alloc_block_id(); }
  ExprId next_expr_id() { return module_->alloc_expr_id(); }

  static bool contains_stmt_expr(const Node* n) {
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

  QualType qtype_from(const TypeSpec& t, ValueCategory c = ValueCategory::RValue) {
    QualType qt{};
    qt.spec = t;
    qt.category = c;
    return qt;
  }

  std::optional<FnPtrSig> fn_ptr_sig_from_decl_node(const Node* n) {
    if (!n) return std::nullopt;

    // Canonical path: extract fn_ptr sig from sema's resolved type table.
    if (resolved_types_) {
      auto ct = resolved_types_->lookup(n);
      if (ct && sema::is_callable_type(*ct)) {
        const auto* fsig = sema::get_function_sig(*ct);
        if (fsig) {
          FnPtrSig sig{};
          sig.canonical_sig = ct;
          // Derive return type from canonical sig.
          if (fsig->return_type) {
            TypeSpec ret_ts = sema::typespec_from_canonical(*fsig->return_type);
            // Fix for nested fn_ptr: canonical type doesn't capture nested fn_ptr
            // return structure. Use parser's ret_fn_ptr_params to correct it.
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
            sig.params.push_back(qtype_from(sema::typespec_from_canonical(param), ValueCategory::LValue));
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

    // Legacy path removed (Phase D slice 4).
    // All fn_ptr declarations should be in ResolvedTypeTable after sema.
    // If canonical lookup fails, the node is not a callable fn_ptr.
    return std::nullopt;
  }

  std::optional<TypeSpec> infer_call_result_type_from_callee(FunctionCtx* ctx, const Node* callee) {
    if (!callee) return std::nullopt;
    if (callee->kind == NK_DEREF) return infer_call_result_type_from_callee(ctx, callee->left);
    if (callee->kind != NK_VAR || !callee->name || !callee->name[0]) return std::nullopt;
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

  std::optional<TypeSpec> storage_type_for_declref(FunctionCtx* ctx, const DeclRef& r) {
    if (r.local && ctx) {
      auto it = ctx->local_types.find(r.local->value);
      if (it != ctx->local_types.end()) return it->second;
    }
    if (r.param_index && ctx && ctx->fn && *r.param_index < ctx->fn->params.size())
      return ctx->fn->params[*r.param_index].type.spec;
    if (r.global) {
      if (const GlobalVar* gv = module_->find_global(*r.global)) return gv->type.spec;
    }
    return std::nullopt;
  }

  TypeSpec infer_generic_ctrl_type(FunctionCtx* ctx, const Node* n) {
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
          // Check for operator_deref method — return its return type.
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
        if (ctx && !ctx->tpl_bindings.empty() && ts.tpl_struct_origin)
          resolve_pending_tpl_struct(ts, ctx->tpl_bindings, ctx->nttp_bindings);
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
        TypeSpec ts{}; ts.base = TB_INT; return ts;
      }
      case NK_CALL:
      case NK_BUILTIN_CALL: {
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
            std::string key = std::string(callee_ts.tag) + "::operator_call";
            auto rit = struct_method_ret_types_.find(key);
            if (rit != struct_method_ret_types_.end())
              return reference_value_ts(rit->second);
          }
        }
        break;
      }
      default:
        break;
    }
    return n->type;
  }

  Block& ensure_block(FunctionCtx& ctx, BlockId id) {
    for (auto& b : ctx.fn->blocks) {
      if (b.id.value == id.value) return b;
    }
    ctx.fn->blocks.push_back(Block{id, {}, false});
    return ctx.fn->blocks.back();
  }

  BlockId create_block(FunctionCtx& ctx) {
    const BlockId id = next_block_id();
    ctx.fn->blocks.push_back(Block{id, {}, false});
    return id;
  }

  void append_stmt(FunctionCtx& ctx, Stmt stmt) {
    Block& b = ensure_block(ctx, ctx.current_block);
    b.stmts.push_back(std::move(stmt));
  }

  ExprId append_expr(const Node* src, ExprPayload payload, const TypeSpec& ts,
                     ValueCategory c = ValueCategory::RValue) {
    Expr e{};
    e.id = next_expr_id();
    e.span = make_span(src);
    e.type = qtype_from(ts, c);
    e.payload = std::move(payload);
    module_->expr_pool.push_back(std::move(e));
    return module_->expr_pool.back().id;
  }

  void lower_struct_def(const Node* sd) {
    if (!sd || sd->kind != NK_STRUCT_DEF) return;
    // Skip template struct definitions — only instantiated structs are lowered.
    // An instantiated struct has n_template_args > 0 (concrete bindings stored).
    if (sd->n_template_params > 0 && sd->n_template_args == 0) return;
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

    int llvm_idx = 0;
    // Bitfield packing state (for structs only; unions always use offset 0)
    int bf_unit_start_bit = -1;  // bit position where current storage unit starts (-1 = none)
    int bf_unit_bits = 0;        // size of current storage unit in bits
    int bf_current_bit = 0;      // current bit position within the storage unit

    for (int i = 0; i < num_fields; ++i) {
      const Node* f = get_field(i);
      if (!f) continue;

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
        ft.array_rank = 0;
        ft.array_size = -1;
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

  void collect_enum_def(const Node* ed) {
    if (!ed || ed->kind != NK_ENUM_DEF || ed->n_enum_variants <= 0) return;
    if (!ed->enum_names || !ed->enum_vals) return;
    for (int i = 0; i < ed->n_enum_variants; ++i) {
      const char* name = ed->enum_names[i];
      if (!name || !name[0]) continue;
      enum_consts_[name] = ed->enum_vals[i];
      ct_state_->register_enum_const(name, ed->enum_vals[i]);
    }
  }

  // Resolve a pending template struct type using concrete template bindings.
  // If ts.tpl_struct_origin is non-null, this type references a template struct
  // with unresolved type params (e.g., Pair<T>).  Resolve it by:
  //   1. Looking up the template struct def
  //   2. Substituting type params from tpl_bindings
  //   3. Computing the concrete mangled name
  //   4. Instantiating the concrete struct in the HIR if needed
  //   5. Updating ts.tag to the concrete struct name
  void resolve_pending_tpl_struct(TypeSpec& ts,
                                  const TypeBindings& tpl_bindings,
                                  const NttpBindings& nttp_bindings) {
    if (!ts.tpl_struct_origin) return;
    const char* origin = ts.tpl_struct_origin;
    auto def_it = template_struct_defs_.find(origin);
    if (def_it == template_struct_defs_.end()) return;
    const Node* tpl_def = def_it->second;

    // Parse arg_refs: comma-separated list of arg values in param order.
    std::vector<std::string> arg_refs;
    if (ts.tpl_struct_arg_refs) {
      std::string refs(ts.tpl_struct_arg_refs);
      size_t pos = 0;
      while (pos < refs.size()) {
        size_t comma = refs.find(',', pos);
        if (comma == std::string::npos) {
          arg_refs.push_back(refs.substr(pos));
          break;
        }
        arg_refs.push_back(refs.substr(pos, comma - pos));
        pos = comma + 1;
      }
    }

    // Resolve each arg using tpl_bindings / nttp_bindings.
    std::vector<std::pair<std::string, TypeSpec>> resolved_type_bindings;
    std::vector<std::pair<std::string, long long>> resolved_nttp_bindings;
    int ai = 0;
    for (int pi = 0; pi < tpl_def->n_template_params && ai < (int)arg_refs.size(); ++pi, ++ai) {
      const char* param_name = tpl_def->template_param_names[pi];
      if (tpl_def->template_param_is_nttp[pi]) {
        // NTTP: try nttp_bindings first, then parse as number.
        auto nit = nttp_bindings.find(arg_refs[ai]);
        if (nit != nttp_bindings.end()) {
          resolved_nttp_bindings.push_back({param_name, nit->second});
        } else {
          try {
            resolved_nttp_bindings.push_back({param_name, std::stoll(arg_refs[ai])});
          } catch (...) {
            resolved_nttp_bindings.push_back({param_name, 0});
          }
        }
      } else {
        // Type param: look up in tpl_bindings.
        const std::string& ref = arg_refs[ai];
        if (ref.size() > 1 && ref[0] == '@') {
          // Nested pending template struct: @origin:inner_args
          size_t colon = ref.find(':', 1);
          std::string inner_origin = ref.substr(1, colon == std::string::npos ? std::string::npos : colon - 1);
          std::string inner_args = (colon != std::string::npos) ? ref.substr(colon + 1) : "";
          // Build a temporary TypeSpec for the nested pending struct and resolve it.
          TypeSpec nested_ts{};
          nested_ts.base = TB_STRUCT;
          nested_ts.array_size = -1;
          nested_ts.inner_rank = -1;
          nested_ts.tpl_struct_origin = inner_origin.c_str();
          nested_ts.tpl_struct_arg_refs = inner_args.c_str();
          resolve_pending_tpl_struct(nested_ts, tpl_bindings, nttp_bindings);
          resolved_type_bindings.push_back({param_name, nested_ts});
        } else {
          auto tit = tpl_bindings.find(ref);
          if (tit != tpl_bindings.end()) {
            resolved_type_bindings.push_back({param_name, tit->second});
          } else {
            // Unresolved — shouldn't happen if called with full bindings.
            TypeSpec fallback{};
            fallback.base = TB_INT;
            fallback.array_size = -1;
            fallback.inner_rank = -1;
            resolved_type_bindings.push_back({param_name, fallback});
          }
        }
      }
    }

    // Compute concrete mangled name using the same logic as the parser.
    std::string mangled(origin);
    auto append_type_suffix = [&](const TypeSpec& pts) {
      switch (pts.base) {
        case TB_INT: mangled += "int"; break;
        case TB_UINT: mangled += "uint"; break;
        case TB_CHAR: mangled += "char"; break;
        case TB_SCHAR: mangled += "schar"; break;
        case TB_UCHAR: mangled += "uchar"; break;
        case TB_SHORT: mangled += "short"; break;
        case TB_USHORT: mangled += "ushort"; break;
        case TB_LONG: mangled += "long"; break;
        case TB_ULONG: mangled += "ulong"; break;
        case TB_LONGLONG: mangled += "llong"; break;
        case TB_ULONGLONG: mangled += "ullong"; break;
        case TB_FLOAT: mangled += "float"; break;
        case TB_DOUBLE: mangled += "double"; break;
        case TB_LONGDOUBLE: mangled += "ldouble"; break;
        case TB_VOID: mangled += "void"; break;
        case TB_BOOL: mangled += "bool"; break;
        case TB_INT128: mangled += "i128"; break;
        case TB_UINT128: mangled += "u128"; break;
        case TB_STRUCT:
          mangled += "struct_";
          mangled += pts.tag ? pts.tag : "anon";
          break;
        case TB_UNION:
          mangled += "union_";
          mangled += pts.tag ? pts.tag : "anon";
          break;
        case TB_ENUM:
          mangled += "enum_";
          mangled += pts.tag ? pts.tag : "anon";
          break;
        case TB_TYPEDEF:
          mangled += pts.tag ? pts.tag : "typedef";
          break;
        default: mangled += "T"; break;
      }
      for (int p = 0; p < pts.ptr_level; ++p) mangled += "_ptr";
      if (pts.is_lvalue_ref) mangled += "_ref";
      if (pts.is_rvalue_ref) mangled += "_rref";
    };
    int ti = 0, ni = 0;
    for (int pi = 0; pi < tpl_def->n_template_params; ++pi) {
      mangled += "_";
      mangled += tpl_def->template_param_names[pi];
      mangled += "_";
      if (tpl_def->template_param_is_nttp[pi]) {
        if (ni < (int)resolved_nttp_bindings.size())
          mangled += std::to_string(resolved_nttp_bindings[ni++].second);
        else
          mangled += "0";
      } else {
        if (ti < (int)resolved_type_bindings.size())
          append_type_suffix(resolved_type_bindings[ti++].second);
        else
          mangled += "T";
      }
    }

    // Instantiate the concrete struct if not already done.
    if (!module_->struct_defs.count(mangled) &&
        !instantiated_tpl_structs_.count(mangled)) {
      instantiated_tpl_structs_.insert(mangled);

      HirStructDef def;
      def.tag = mangled;
      def.ns_qual = make_ns_qual(tpl_def);
      def.is_union = tpl_def->is_union;
      def.pack_align = tpl_def->pack_align;
      def.struct_align = tpl_def->struct_align;

      int num_fields = tpl_def->n_fields > 0 ? tpl_def->n_fields : 0;
      int llvm_idx = 0;
      for (int fi = 0; fi < num_fields; ++fi) {
        const Node* orig_f = tpl_def->fields[fi];
        if (!orig_f || !orig_f->name) continue;

        TypeSpec ft = orig_f->type;
        // Substitute type params in field type.
        for (const auto& [pname, pts] : resolved_type_bindings) {
          if (ft.base == TB_TYPEDEF && ft.tag && std::string(ft.tag) == pname) {
            ft.base = pts.base;
            ft.tag = pts.tag;
            ft.ptr_level += pts.ptr_level;
            ft.is_const |= pts.is_const;
            ft.is_volatile |= pts.is_volatile;
          }
        }
        // Substitute NTTP values in array dimensions.
        if (ft.array_size_expr) {
          Node* ase = ft.array_size_expr;
          if (ase->kind == NK_VAR && ase->name) {
            for (const auto& [npname, nval] : resolved_nttp_bindings) {
              if (std::string(ase->name) == npname) {
                if (ft.array_rank > 0) {
                  ft.array_dims[0] = nval;
                  ft.array_size = nval;
                }
                ft.array_size_expr = nullptr;
                break;
              }
            }
          }
        }

        HirStructField hf;
        hf.name = orig_f->name;
        if (ft.array_rank > 0) {
          hf.is_flexible_array = (ft.array_size < 0);
          hf.array_first_dim = (ft.array_size >= 0) ? ft.array_size : 0;
          ft.array_rank = 0;
          ft.array_size = -1;
        }
        hf.elem_type = ft;
        hf.is_anon_member = orig_f->is_anon_field;
        hf.llvm_idx = tpl_def->is_union ? 0 : llvm_idx;
        def.fields.push_back(std::move(hf));
        if (!tpl_def->is_union) ++llvm_idx;
      }

      compute_struct_layout(module_, def);
      module_->struct_def_order.push_back(mangled);
      module_->struct_defs[mangled] = std::move(def);

      // Register and immediately lower methods from the template struct.
      TypeBindings method_tpl_bindings;
      for (const auto& [pname, pts] : resolved_type_bindings)
        method_tpl_bindings[pname] = pts;
      NttpBindings method_nttp_bindings;
      for (const auto& [npname, nval] : resolved_nttp_bindings)
        method_nttp_bindings[npname] = nval;
      for (int mi = 0; mi < tpl_def->n_children; ++mi) {
        const Node* method = tpl_def->children[mi];
        if (!method || method->kind != NK_FUNCTION || !method->name) continue;
        const char* csuf = method->is_const_method ? "_const" : "";
        std::string mmangled = mangled + "__" + method->name + csuf;
        std::string mkey = mangled + "::" + method->name + csuf;
        if (!struct_methods_.count(mkey)) {
          struct_methods_[mkey] = mmangled;
          struct_method_ret_types_[mkey] = method->type;
          lower_struct_method(mmangled, mangled, method,
                              &method_tpl_bindings, &method_nttp_bindings);
        }
      }
    }

    // Update the TypeSpec to point to the concrete struct.
    ts.tag = module_->struct_defs.count(mangled) ?
        module_->struct_defs.at(mangled).tag.c_str() : nullptr;
    ts.tpl_struct_origin = nullptr;
    ts.tpl_struct_arg_refs = nullptr;
  }

  void lower_function(const Node* fn_node,
                      const std::string* name_override = nullptr,
                      const TypeBindings* tpl_override = nullptr,
                      const NttpBindings* nttp_override = nullptr) {
    Function fn{};
    fn.id = next_fn_id();
    fn.name = name_override ? *name_override
                            : (fn_node->name ? fn_node->name : "<anon_fn>");
    fn.ns_qual = make_ns_qual(fn_node);
    // Substitute template type parameters in the return type.
    {
      TypeSpec ret_ts = fn_node->type;
      if ((fn_node->n_ret_fn_ptr_params > 0 || fn_node->ret_fn_ptr_variadic) &&
          !ret_ts.is_fn_ptr) {
        ret_ts.is_fn_ptr = true;
        ret_ts.ptr_level = std::max(ret_ts.ptr_level, 1);
      }
      if (tpl_override && ret_ts.base == TB_TYPEDEF && ret_ts.tag) {
        auto it = tpl_override->find(ret_ts.tag);
        if (it != tpl_override->end()) {
          const TypeSpec& concrete = it->second;
          ret_ts.base = concrete.base;
          ret_ts.tag = concrete.tag;
        }
      }
      // Resolve pending template struct types (e.g., Pair<T> → Pair_T_int).
      if (tpl_override && ret_ts.tpl_struct_origin) {
        NttpBindings nttp_empty;
        resolve_pending_tpl_struct(ret_ts, *tpl_override,
            nttp_override ? *nttp_override : nttp_empty);
      }
      fn.return_type = qtype_from(ret_ts);
    }
    // Build fn_ptr_sig for the return type when the function returns a fn_ptr.
    // Uses canonical type to extract the return type's callable signature.
    if (fn_node->type.is_fn_ptr ||
        fn_node->n_ret_fn_ptr_params > 0 ||
        fn_node->ret_fn_ptr_variadic) {
      auto fn_ct = sema::canonicalize_declarator_type(fn_node);
      const auto* fn_fsig = sema::get_function_sig(fn_ct);
      if (fn_fsig && fn_fsig->return_type && sema::is_callable_type(*fn_fsig->return_type)) {
        const auto* ret_fsig = sema::get_function_sig(*fn_fsig->return_type);
        if (ret_fsig) {
          FnPtrSig ret_sig{};
          ret_sig.canonical_sig = fn_fsig->return_type;
          if (ret_fsig->return_type) {
            TypeSpec ret_ts = sema::typespec_from_canonical(*ret_fsig->return_type);
            // Nested fn_ptr fixup: canonical type doesn't capture nested fn_ptr
            // return structure. Use parser's ret_fn_ptr_params to correct it.
            if ((fn_node->n_ret_fn_ptr_params > 0 || fn_node->ret_fn_ptr_variadic) &&
                !ret_ts.is_fn_ptr) {
              ret_ts.is_fn_ptr = true;
              ret_ts.ptr_level = std::max(ret_ts.ptr_level, 1);
            }
            ret_sig.return_type = qtype_from(ret_ts);
          }
          ret_sig.variadic = ret_fsig->is_variadic;
          ret_sig.unspecified_params = ret_fsig->unspecified_params;
          for (const auto& param : ret_fsig->params)
            ret_sig.params.push_back(qtype_from(sema::typespec_from_canonical(param), ValueCategory::LValue));
          if (ret_sig.params.empty() &&
              (fn_node->n_ret_fn_ptr_params > 0 || fn_node->ret_fn_ptr_variadic)) {
            ret_sig.variadic = fn_node->ret_fn_ptr_variadic;
            ret_sig.unspecified_params = false;
            for (int i = 0; i < fn_node->n_ret_fn_ptr_params; ++i) {
              const Node* param = fn_node->ret_fn_ptr_params[i];
              ret_sig.params.push_back(qtype_from(param->type, ValueCategory::LValue));
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
    fn.linkage = {fn_node->is_static, fn_node->is_extern || fn_node->body == nullptr, fn_node->is_inline,
                   weak_symbols_.count(fn.name) > 0, static_cast<Visibility>(fn_node->visibility)};
    fn.attrs.variadic = fn_node->variadic;
    fn.attrs.no_inline = fn_node->type.is_noinline;
    fn.attrs.always_inline = fn_node->type.is_always_inline;
    if (fn_node->type.align_bytes > 0)
      fn.attrs.align_bytes = fn_node->type.align_bytes;
    // Inherit alignment from prior declaration if definition doesn't repeat the attribute.
    if (fn.attrs.align_bytes == 0) {
      auto fit = module_->fn_index.find(fn.name);
      if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
        int prev_align = module_->functions[fit->second.value].attrs.align_bytes;
        if (prev_align > 0) fn.attrs.align_bytes = prev_align;
      }
    }
    fn.span = make_span(fn_node);

    FunctionCtx ctx{};
    ctx.fn = &fn;

    // Populate template bindings for template function instantiations.
    if (tpl_override)
      ctx.tpl_bindings = *tpl_override;
    if (nttp_override)
      ctx.nttp_bindings = *nttp_override;

    for (int i = 0; i < fn_node->n_params; ++i) {
      const Node* p = fn_node->params[i];
      if (!p) continue;
      Param param{};
      param.name = p->name ? p->name : "<anon_param>";
      // Substitute template type parameters in parameter types.
      {
        TypeSpec param_ts = p->type;
        if (tpl_override && param_ts.base == TB_TYPEDEF && param_ts.tag) {
          auto it = tpl_override->find(param_ts.tag);
          if (it != tpl_override->end()) {
            const TypeSpec& concrete = it->second;
            param_ts.base = concrete.base;
            param_ts.tag = concrete.tag;
          }
        }
        // Resolve pending template struct types in params.
        if (tpl_override && param_ts.tpl_struct_origin) {
          NttpBindings nttp_empty;
          resolve_pending_tpl_struct(param_ts, *tpl_override,
              nttp_override ? *nttp_override : nttp_empty);
        }
        param.type = qtype_from(reference_storage_ts(param_ts), ValueCategory::LValue);
      }
      param.fn_ptr_sig = fn_ptr_sig_from_decl_node(p);
      param.span = make_span(p);
      ctx.params[param.name] = static_cast<uint32_t>(fn.params.size());
      if (param.fn_ptr_sig) ctx.param_fn_ptr_sigs[param.name] = *param.fn_ptr_sig;
      fn.params.push_back(std::move(param));
    }

    if (!fn_node->body) {
      module_->fn_index[fn.name] = fn.id;
      module_->functions.push_back(std::move(fn));
      return;
    }

    // Pre-register the function so that self-references during body
    // lowering (e.g. recursive function using its own name as a value)
    // can look up the return type via fn_index.
    const bool had_prior_decl = module_->fn_index.count(fn.name) > 0;
    module_->fn_index[fn.name] = fn.id;
    if (fn.id.value == module_->functions.size()) {
      // Push a skeleton; will be replaced after body lowering.
      module_->functions.push_back(Function{fn.id, fn.name, fn.ns_qual, fn.return_type});
    }

    const BlockId entry = create_block(ctx);
    fn.entry = entry;
    ctx.current_block = entry;

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

    if (fn.blocks.empty()) {
      fn.blocks.push_back(Block{entry, {}, false});
    }

    // Replace the skeleton with the fully lowered function.
    module_->functions[fn.id.value] = std::move(fn);
  }

  // Lower a struct method as a standalone function with an implicit `this` pointer.
  void lower_struct_method(const std::string& mangled_name,
                           const std::string& struct_tag,
                           const Node* method_node,
                           const TypeBindings* tpl_bindings = nullptr,
                           const NttpBindings* nttp_bindings = nullptr) {
    // Skip deleted methods — they have no body and must not be lowered.
    if (method_node->is_deleted) return;
    Function fn{};
    fn.id = next_fn_id();
    fn.name = mangled_name;
    fn.ns_qual = make_ns_qual(method_node);
    // Substitute template type parameters in the return type.
    {
      TypeSpec ret_ts = method_node->type;
      if (tpl_bindings && ret_ts.base == TB_TYPEDEF && ret_ts.tag) {
        auto it = tpl_bindings->find(ret_ts.tag);
        if (it != tpl_bindings->end()) {
          ret_ts.base = it->second.base;
          ret_ts.tag = it->second.tag;
        }
      }
      // Resolve pending template struct types (e.g., Pair<T> → Pair_T_int).
      if (tpl_bindings && ret_ts.tpl_struct_origin) {
        NttpBindings nttp_empty;
        resolve_pending_tpl_struct(ret_ts, *tpl_bindings,
            nttp_bindings ? *nttp_bindings : nttp_empty);
      }
      resolve_typedef_to_struct(ret_ts);
      fn.return_type = qtype_from(ret_ts);
    }
    fn.linkage = {true, false, false, false, Visibility::Default};  // internal
    fn.attrs.variadic = method_node->variadic;
    fn.span = make_span(method_node);

    FunctionCtx ctx{};
    ctx.fn = &fn;
    if (tpl_bindings) ctx.tpl_bindings = *tpl_bindings;
    if (nttp_bindings) ctx.nttp_bindings = *nttp_bindings;

    // Add implicit `this` parameter (pointer to struct).
    {
      Param this_param{};
      this_param.name = "this";
      TypeSpec this_ts{};
      this_ts.base = TB_STRUCT;
      // Use the persistent tag pointer from the struct_def in the module.
      auto sit = module_->struct_defs.find(struct_tag);
      this_ts.tag = sit != module_->struct_defs.end()
                        ? sit->second.tag.c_str()
                        : struct_tag.c_str();
      this_ts.ptr_level = 1;
      this_param.type = qtype_from(this_ts, ValueCategory::LValue);
      this_param.span = make_span(method_node);
      ctx.params[this_param.name] = static_cast<uint32_t>(fn.params.size());
      fn.params.push_back(std::move(this_param));
    }

    // Add explicit parameters.
    for (int i = 0; i < method_node->n_params; ++i) {
      const Node* p = method_node->params[i];
      if (!p) continue;
      Param param{};
      param.name = p->name ? p->name : "<anon_param>";
      // Substitute template type parameters in parameter types.
      {
        TypeSpec param_ts = p->type;
        if (tpl_bindings && param_ts.base == TB_TYPEDEF && param_ts.tag) {
          auto it = tpl_bindings->find(param_ts.tag);
          if (it != tpl_bindings->end()) {
            param_ts.base = it->second.base;
            param_ts.tag = it->second.tag;
          }
        }
        // Resolve pending template struct types in params.
        if (tpl_bindings && param_ts.tpl_struct_origin) {
          NttpBindings nttp_empty;
          resolve_pending_tpl_struct(param_ts, *tpl_bindings,
              nttp_bindings ? *nttp_bindings : nttp_empty);
        }
        resolve_typedef_to_struct(param_ts);
        param.type = qtype_from(reference_storage_ts(param_ts), ValueCategory::LValue);
      }
      param.span = make_span(p);
      ctx.params[param.name] = static_cast<uint32_t>(fn.params.size());
      fn.params.push_back(std::move(param));
    }

    // Store the struct tag so field accesses in the body can resolve via `this`.
    ctx.method_struct_tag = struct_tag;

    if (!method_node->body && !method_node->is_defaulted) {
      module_->fn_index[fn.name] = fn.id;
      module_->functions.push_back(std::move(fn));
      return;
    }

    module_->fn_index[fn.name] = fn.id;
    if (fn.id.value == module_->functions.size()) {
      module_->functions.push_back(Function{fn.id, fn.name, fn.ns_qual, fn.return_type});
    }

    const BlockId entry = create_block(ctx);
    fn.entry = entry;
    ctx.current_block = entry;

    // Generate synthetic body for = default special member functions.
    if (method_node->is_defaulted) {
      emit_defaulted_method_body(ctx, fn, struct_tag, method_node);
      // For destructors: emit member destructor calls after the (empty) body.
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
        ExprId this_ptr = append_expr(method_node, this_ref, this_ts, ValueCategory::LValue);
        emit_member_dtor_calls(ctx, struct_tag, this_ptr, method_node);
      }
      // Emit ret void for destructors (after member dtor calls) and any other
      // defaulted methods that didn't already emit a return.
      if (method_node->is_destructor) {
        ReturnStmt rs{};
        append_stmt(ctx, Stmt{StmtPayload{rs}, make_span(method_node)});
      }
      if (fn.blocks.empty()) {
        fn.blocks.push_back(Block{entry, {}, false});
      }
      module_->functions[fn.id.value] = std::move(fn);
      return;
    }

    // Emit constructor initializer list assignments before body.
    if (method_node->is_constructor && method_node->n_ctor_inits > 0) {
      auto sit = module_->struct_defs.find(struct_tag);
      for (int i = 0; i < method_node->n_ctor_inits; ++i) {
        const char* mem_name = method_node->ctor_init_names[i];
        int nargs = method_node->ctor_init_nargs[i];
        Node** args = method_node->ctor_init_args[i];

        // Delegating constructor: init name matches struct tag.
        if (mem_name == struct_tag || (mem_name && struct_tag == mem_name)) {
          // Resolve and call target constructor with same 'this' pointer.
          auto cit = struct_constructors_.find(struct_tag);
          if (cit == struct_constructors_.end() || cit->second.empty()) {
            throw std::runtime_error("error: no constructors found for delegating constructor call to '" + struct_tag + "'");
          }
          const CtorOverload* best = nullptr;
          if (cit->second.size() == 1 && cit->second[0].method_node->n_params == nargs) {
            best = &cit->second[0];
          } else {
            int best_score = -1;
            for (const auto& ov : cit->second) {
              if (ov.method_node->n_params != nargs) continue;
              // Skip self (same mangled name as current ctor).
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
                           param_base.base != TB_STRUCT && arg_ts.base != TB_STRUCT) {
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
            throw std::runtime_error("error: no matching constructor for delegating constructor call to '" + struct_tag + "'");
          }
          if (best->method_node->is_deleted) {
            throw std::runtime_error("error: call to deleted constructor '" + struct_tag + "'");
          }
          // Emit: Tag__Tag(&this, args...)
          DeclRef this_ref{};
          this_ref.name = "this";
          auto pit = ctx.params.find("this");
          if (pit != ctx.params.end()) this_ref.param_index = pit->second;
          TypeSpec this_ts{};
          this_ts.base = TB_STRUCT;
          this_ts.tag = struct_tag.c_str();
          this_ts.ptr_level = 1;
          ExprId this_id = append_expr(method_node, this_ref, this_ts, ValueCategory::LValue);

          CallExpr c{};
          DeclRef callee_ref{};
          callee_ref.name = best->mangled_name;
          TypeSpec fn_ts{}; fn_ts.base = TB_VOID;
          TypeSpec callee_ts = fn_ts; callee_ts.ptr_level++;
          c.callee = append_expr(method_node, callee_ref, callee_ts);
          c.args.push_back(this_id);  // pass 'this' directly (already a ptr)
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
          ExprStmt es{}; es.expr = call_id;
          append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
          continue;  // skip normal member init processing
        }

        // Build this->member as lvalue.
        DeclRef this_ref{};
        this_ref.name = "this";
        auto pit = ctx.params.find("this");
        if (pit != ctx.params.end()) this_ref.param_index = pit->second;
        TypeSpec this_ts{};
        this_ts.base = TB_STRUCT;
        this_ts.tag = sit != module_->struct_defs.end()
                          ? sit->second.tag.c_str()
                          : struct_tag.c_str();
        this_ts.ptr_level = 1;
        ExprId this_id = append_expr(method_node, this_ref, this_ts, ValueCategory::LValue);
        // Find field type.
        TypeSpec field_ts{};
        if (sit != module_->struct_defs.end()) {
          for (const auto& fld : sit->second.fields) {
            if (fld.name == mem_name) {
              field_ts = fld.elem_type;
              // Restore array type if needed.
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

        // Check if member is a struct type with constructors — call constructor.
        bool did_ctor_call = false;
        if (field_ts.base == TB_STRUCT && field_ts.tag && field_ts.ptr_level == 0) {
          auto cit = struct_constructors_.find(field_ts.tag);
          if (cit != struct_constructors_.end() && !cit->second.empty()) {
            // Resolve best constructor overload.
            const CtorOverload* best = nullptr;
            if (cit->second.size() == 1 && cit->second[0].method_node->n_params == nargs) {
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
                             param_base.base != TB_STRUCT && arg_ts.base != TB_STRUCT) {
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
              // Emit: MemberTag__MemberTag(&this->member, args...)
              CallExpr c{};
              DeclRef callee_ref{};
              callee_ref.name = best->mangled_name;
              TypeSpec fn_ts{}; fn_ts.base = TB_VOID;
              TypeSpec callee_ts = fn_ts; callee_ts.ptr_level++;
              c.callee = append_expr(method_node, callee_ref, callee_ts);
              // First arg: &(this->member) as the constructor's this pointer.
              UnaryExpr addr{};
              addr.op = UnaryOp::AddrOf;
              addr.operand = lhs_id;
              TypeSpec ptr_ts = field_ts; ptr_ts.ptr_level++;
              c.args.push_back(append_expr(method_node, addr, ptr_ts));
              // Explicit args — handle reference params.
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
              ExprStmt es{}; es.expr = call_id;
              append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
              did_ctor_call = true;
            }
          }
        }

        if (!did_ctor_call) {
          // Scalar member: simple assignment this->member = expr.
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
          ExprStmt es{}; es.expr = ae_id;
          append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
        }
      }
    }

    lower_stmt_node(ctx, method_node->body);

    // For destructors: emit member destructor calls after the user body.
    if (method_node->is_destructor) {
      // Build `this` parameter reference as the pointer to pass to member dtors.
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
      ExprId this_ptr = append_expr(method_node, this_ref, this_ts, ValueCategory::LValue);
      emit_member_dtor_calls(ctx, struct_tag, this_ptr, method_node);
    }

    if (fn.blocks.empty()) {
      fn.blocks.push_back(Block{entry, {}, false});
    }

    module_->functions[fn.id.value] = std::move(fn);
  }

  // Hoist a compound literal to an anonymous global variable.
  // Returns the ExprId of an AddrOf(DeclRef{clit_name}) expression.
  ExprId hoist_compound_literal_to_global(const Node* addr_node, const Node* clit) {
    GlobalVar cg{};
    cg.id = next_global_id();
    const std::string clit_name = "__clit_" + std::to_string(cg.id.value);
    cg.name = clit_name;
    cg.type = qtype_from(clit->type, ValueCategory::LValue);
    cg.linkage = {true, false, false};  // internal linkage
    cg.is_const = false;
    cg.span = make_span(clit);
    // Lower the compound literal's init list (stored in clit->left).
    if (clit->left && clit->left->kind == NK_INIT_LIST) {
      cg.init = lower_init_list(clit->left);
      cg.type.spec = resolve_array_ts(cg.type.spec, cg.init);
      cg.init = normalize_global_init(cg.type.spec, cg.init);
    }
    module_->global_index[clit_name] = cg.id;
    module_->globals.push_back(std::move(cg));

    // Build a scalar init: AddrOf(DeclRef{clit_name})
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

  void lower_global(const Node* gv) {
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
    g.type = qtype_from(gv->type, ValueCategory::LValue);
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

  void lower_local_decl_stmt(FunctionCtx& ctx, const Node* n) {
    // Local function prototype (e.g. `int f1(char *);` inside a function body):
    // if the name is already registered as a known function, skip creating a
    // local alloca — later references will resolve directly to the global function.
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
      if (!ctx.tpl_bindings.empty() && decl_ts.tpl_struct_origin)
        resolve_pending_tpl_struct(decl_ts, ctx.tpl_bindings, ctx.nttp_bindings);
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
      TypeSpec int_ts{}; int_ts.base = TB_INT;
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

    // C++ constructor invocation: Type var(args) → call StructTag__StructTag(&var, args...)
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
          TypeSpec fn_ts{}; fn_ts.base = TB_VOID;
          TypeSpec callee_ts = fn_ts; callee_ts.ptr_level++;
          c.callee = append_expr(n, callee_ref, callee_ts);
          // First arg: &var (this pointer).
          DeclRef var_ref{};
          var_ref.name = n->name ? n->name : "<anon_local>";
          var_ref.local = lid;
          ExprId var_id = append_expr(n, var_ref, decl_ts, ValueCategory::LValue);
          UnaryExpr addr{};
          addr.op = UnaryOp::AddrOf;
          addr.operand = var_id;
          TypeSpec ptr_ts = decl_ts; ptr_ts.ptr_level++;
          c.args.push_back(append_expr(n, addr, ptr_ts));
          // Explicit args — handle reference params.
          for (int i = 0; i < n->n_children; ++i) {
            TypeSpec param_ts = best->method_node->params[i]->type;
            resolve_typedef_to_struct(param_ts);
            if (param_ts.is_rvalue_ref || param_ts.is_lvalue_ref) {
              // Reference parameter: pass address of the argument.
              const Node* arg = n->children[i];
              const Node* inner = arg;
              // Unwrap static_cast<T&&>(x) to get x.
              if (inner->kind == NK_CAST && inner->left)
                inner = inner->left;
              // Check if the argument is a function call returning a reference type.
              // In that case, the call already returns a pointer — no AddrOf needed.
              bool arg_returns_ref = false;
              if (inner->kind == NK_CALL && inner->left &&
                  inner->left->kind == NK_VAR && inner->left->name) {
                TypeSpec call_ret = infer_generic_ctrl_type(&ctx, arg);
                // infer_generic_ctrl_type strips refs, so check original return type.
                auto dit = deduced_template_calls_.find(inner);
                if (dit != deduced_template_calls_.end()) {
                  auto fit = module_->fn_index.find(dit->second.mangled_name);
                  if (fit != module_->fn_index.end()) {
                    const Function* fn = module_->find_function(fit->second);
                    if (fn && is_any_ref_ts(fn->return_type.spec))
                      arg_returns_ref = true;
                  }
                }
                if (!arg_returns_ref) {
                  auto fit = module_->fn_index.find(inner->left->name);
                  if (fit != module_->fn_index.end()) {
                    const Function* fn = module_->find_function(fit->second);
                    if (fn && is_any_ref_ts(fn->return_type.spec))
                      arg_returns_ref = true;
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
          ExprStmt es{}; es.expr = call_id;
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
          TypeSpec fn_ts{}; fn_ts.base = TB_VOID;
          TypeSpec callee_ts = fn_ts; callee_ts.ptr_level++;
          c.callee = append_expr(n, callee_ref, callee_ts);
          // First arg: &var (this pointer).
          DeclRef var_ref{};
          var_ref.name = n->name ? n->name : "<anon_local>";
          var_ref.local = lid;
          ExprId var_id = append_expr(n, var_ref, decl_ts, ValueCategory::LValue);
          UnaryExpr addr{};
          addr.op = UnaryOp::AddrOf;
          addr.operand = var_id;
          TypeSpec ptr_ts = decl_ts; ptr_ts.ptr_level++;
          c.args.push_back(append_expr(n, addr, ptr_ts));
          ExprId call_id = append_expr(n, c, fn_ts);
          ExprStmt es{}; es.expr = call_id;
          append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
        }
      }
    }

    // C++ copy initialization: T var = expr; → call copy/move constructor.
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
            continue;  // T&& cannot bind lvalue — skip
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
          TypeSpec fn_ts{}; fn_ts.base = TB_VOID;
          TypeSpec callee_ts = fn_ts; callee_ts.ptr_level++;
          c.callee = append_expr(n, callee_ref, callee_ts);
          // First arg: &var (this pointer).
          DeclRef var_ref{};
          var_ref.name = n->name ? n->name : "<anon_local>";
          var_ref.local = lid;
          ExprId var_id = append_expr(n, var_ref, decl_ts, ValueCategory::LValue);
          UnaryExpr addr{};
          addr.op = UnaryOp::AddrOf;
          addr.operand = var_id;
          TypeSpec ptr_ts = decl_ts; ptr_ts.ptr_level++;
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
            TypeSpec storage_ts = decl_ts; storage_ts.ptr_level++;
            c.args.push_back(append_expr(n->init, addr_e, storage_ts));
          } else {
            c.args.push_back(lower_expr(&ctx, n->init));
          }
          ExprId call_id = append_expr(n, c, fn_ts);
          ExprStmt es{}; es.expr = call_id;
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
        TypeSpec idx_ts{}; idx_ts.base = TB_INT;
        ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
        IndexExpr ie{}; ie.base = dr_id; ie.index = idx_id;
        ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
        ExprId zero_id = append_expr(n, IntLiteral{0, false}, idx_ts);
        AssignExpr ae{}; ae.lhs = ie_id; ae.rhs = zero_id;
        ExprId ae_id = append_expr(n, ae, elem_ts);
        ExprStmt es{}; es.expr = ae_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
      }
    };
    // For array init lists, emit element-by-element assignments.
    if (is_array_with_init_list && use_array_init_fast_path) {
      emit_scalar_array_zero_fill(decl_ts);
      long long next_idx = 0;
      for (int ci = 0; ci < n->init->n_children; ++ci) {
        const Node* item = n->init->children[ci];
        if (!item) { ++next_idx; continue; }
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
        DeclRef dr{}; dr.name = n->name ? n->name : "<anon_local>"; dr.local = lid;
        ExprId dr_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);
        // IntLiteral for index
        TypeSpec idx_ts{}; idx_ts.base = TB_INT;
        ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
        // IndexExpr
        IndexExpr ie{}; ie.base = dr_id; ie.index = idx_id;
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
        AssignExpr ae{}; ae.lhs = ie_id; ae.rhs = val_id;
        ExprId ae_id = append_expr(n, ae, elem_ts);
        ExprStmt es{}; es.expr = ae_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
      }
    }
    if (is_array_with_string_init && n->init && n->init->sval) {
      emit_scalar_array_zero_fill(decl_ts);
      const bool is_wide = n->init->sval[0] == 'L';
      const auto vals = decode_string_literal_values(n->init->sval, is_wide);
      const long long max_count = decl_ts.array_size > 0 ? decl_ts.array_size : static_cast<long long>(vals.size());
      const long long emit_n = std::min<long long>(max_count, static_cast<long long>(vals.size()));
      for (long long idx = 0; idx < emit_n; ++idx) {
        TypeSpec elem_ts = decl_ts;
        elem_ts.array_rank--;
        elem_ts.array_size = -1;
        DeclRef dr{};
        dr.name = n->name ? n->name : "<anon_local>";
        dr.local = lid;
        ExprId dr_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);
        TypeSpec idx_ts{}; idx_ts.base = TB_INT;
        ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
        IndexExpr ie{}; ie.base = dr_id; ie.index = idx_id;
        ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
        ExprId val_id = append_expr(n, IntLiteral{vals[static_cast<size_t>(idx)], false}, idx_ts);
        AssignExpr ae{}; ae.lhs = ie_id; ae.rhs = val_id;
        ExprId ae_id = append_expr(n, ae, elem_ts);
        ExprStmt es{}; es.expr = ae_id;
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
              ExprId val_id = append_expr(n, IntLiteral{vals[static_cast<size_t>(idx)], false}, idx_ts);
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

  std::optional<ExprId> try_lower_consteval_call_expr(FunctionCtx* ctx, const Node* n) {
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
        if (fn_def->template_param_is_nttp &&
            fn_def->template_param_is_nttp[i]) {
          if (n->left->template_arg_is_value &&
              n->left->template_arg_is_value[i]) {
            if (n->left->template_arg_nttp_names &&
                n->left->template_arg_nttp_names[i] && ctx &&
                !ctx->nttp_bindings.empty()) {
              auto it = ctx->nttp_bindings.find(
                  n->left->template_arg_nttp_names[i]);
              if (it != ctx->nttp_bindings.end()) {
                ce_nttp_bindings[fn_def->template_param_names[i]] =
                    it->second;
              }
            } else {
              ce_nttp_bindings[fn_def->template_param_names[i]] =
                  n->left->template_arg_values[i];
            }
          }
          continue;
        }
        TypeSpec arg_ts = n->left->template_arg_types[i];
        if (arg_ts.base == TB_TYPEDEF && arg_ts.tag && ctx &&
            !ctx->tpl_bindings.empty()) {
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
      if (!ce_nttp_bindings.empty())
        arg_env.nttp_bindings = &ce_nttp_bindings;
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
    for (const auto& cv : args)
      pce.const_args.push_back(cv.as_int());
    pce.tpl_bindings = tpl_bindings;
    pce.nttp_bindings = ce_nttp_bindings;
    pce.call_span = make_span(n);
    pce.unlocked_by_deferred_instantiation = lowering_deferred_instantiation_;
    TypeSpec ts = n->type;
    return append_expr(n, std::move(pce), ts);
  }

  // Check if an AST expression is an lvalue (variable, subscript, deref, member).
  static bool is_ast_lvalue(const Node* n) {
    if (!n) return false;
    switch (n->kind) {
      case NK_VAR: case NK_INDEX: case NK_DEREF: case NK_MEMBER:
        return true;
      default:
        return false;
    }
  }

  // Resolve a ref-overloaded function call: pick the best overload based on
  // argument value categories. Returns the mangled name of the best match,
  // or empty string if no overload set exists.
  std::string resolve_ref_overload(const std::string& base_name, const Node* call_node) {
    auto ovit = ref_overload_set_.find(base_name);
    if (ovit == ref_overload_set_.end()) return {};
    const auto& overloads = ovit->second;
    const std::string* best_name = nullptr;
    int best_score = -1;
    for (const auto& ov : overloads) {
      bool viable = true;
      int score = 0;
      for (int i = 0; i < call_node->n_children && i < static_cast<int>(ov.param_is_rvalue_ref.size()); ++i) {
        bool arg_is_lvalue = is_ast_lvalue(call_node->children[i]);
        if (ov.param_is_lvalue_ref[static_cast<size_t>(i)] && !arg_is_lvalue) { viable = false; break; }
        if (ov.param_is_rvalue_ref[static_cast<size_t>(i)] && arg_is_lvalue) { viable = false; break; }
        // Exact match scores higher.
        if (ov.param_is_rvalue_ref[static_cast<size_t>(i)] && !arg_is_lvalue) score += 2;
        else if (ov.param_is_lvalue_ref[static_cast<size_t>(i)] && arg_is_lvalue) score += 2;
        else score += 1;
      }
      if (viable && score > best_score) { best_name = &ov.mangled_name; best_score = score; }
    }
    return best_name ? *best_name : base_name;
  }

  const Node* find_pending_method_by_mangled(const std::string& mangled_name) const {
    for (const auto& pm : pending_methods_) {
      if (pm.mangled == mangled_name) return pm.method_node;
    }
    return nullptr;
  }

  ExprId lower_call_expr(FunctionCtx* ctx, const Node* n) {
    if (auto consteval_expr = try_lower_consteval_call_expr(ctx, n)) {
      return *consteval_expr;
    }

    // C++ constructor expression parsed as a call, e.g. `T(args...)`.
    // Materialize a temporary object, invoke the selected constructor on it,
    // and use the temporary as the expression result.
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

    // Try operator() dispatch: if callee is a struct variable, call operator_call.
    if (n->left && n->left->kind == NK_VAR && n->left->name) {
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
      if (!param_ts || (!param_ts->is_lvalue_ref && !param_ts->is_rvalue_ref)) return lower_expr(ctx, arg_node);
      TypeSpec storage_ts = reference_storage_ts(*param_ts);
      if (param_ts->is_rvalue_ref) {
        // Rvalue ref param: materialize arg into a temporary, then pass &temp
        ExprId arg_val = lower_expr(ctx, arg_node);
        TypeSpec val_ts = reference_value_ts(*param_ts);
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
          // Check for ref-overloaded methods and resolve based on arg value categories.
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
      resolved_callee_name = resolve_template_call_name(n->left, enc, enc_nttp);
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
        TypeBindings bindings = build_call_bindings(n->left, tpl_fn, enc);
        for (const auto& pname : get_template_param_order(tpl_fn)) {
          auto bit = bindings.find(pname);
          if (bit != bindings.end()) tci.template_args.push_back(bit->second);
        }
        tci.nttp_args = build_call_nttp_bindings(n->left, tpl_fn, enc_nttp);
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
        for (const auto& pname : get_template_param_order(tpl_fn)) {
          auto bit = ded_it->second.bindings.find(pname);
          if (bit != ded_it->second.bindings.end())
            tci.template_args.push_back(bit->second);
        }
        tci.nttp_args = ded_it->second.nttp_bindings;
      }
      c.template_info = std::move(tci);
    } else {
      // Check for ref-overloaded function call.
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
        // Inside a method body, resolve unqualified calls that match a sibling
        // struct method (e.g. static member functions called without qualifier).
        bool resolved_as_method = false;
        if (ctx && !ctx->method_struct_tag.empty() &&
            n->left && n->left->kind == NK_VAR && n->left->name) {
          const std::string callee_name = n->left->name;
          // Only attempt method resolution when the name isn't already a known
          // global function (free functions take precedence over implicit lookup).
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
              // Pass implicit `this` as the first argument (struct methods
              // always receive a this pointer, even for static members).
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

  // Reconstruct the full TypeSpec for a struct field from its HirStructField.
  static TypeSpec field_type_of(const HirStructField& f) {
    TypeSpec ts = f.elem_type;
    ts.inner_rank = -1;
    if (f.array_first_dim >= 0) {
      for (int i = 0; i < 8; ++i) ts.array_dims[i] = -1;
      ts.array_rank = 1;
      ts.array_size = f.array_first_dim;
      ts.array_dims[0] = f.array_first_dim;
    }
    return ts;
  }

  static TypeSpec field_init_type_of(const HirStructField& f) {
    TypeSpec ts = field_type_of(f);
    if (f.is_flexible_array && ts.array_rank > 0) {
      ts.array_size = -1;
      ts.array_dims[0] = -1;
    }
    return ts;
  }

  static bool is_char_like(TypeBase base) {
    return base == TB_CHAR || base == TB_SCHAR || base == TB_UCHAR;
  }

  static bool is_scalar_init_type(const TypeSpec& ts) {
    return !is_vector_ty(ts) &&
           ts.array_rank == 0 &&
           !((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0);
  }

  static GlobalInit child_init_of(const InitListItem& item) {
    return std::visit(
        [&](const auto& v) -> GlobalInit {
          using V = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<V, InitScalar>) return GlobalInit(v);
          else return GlobalInit(*v);
        },
        item.value);
  }

  static std::optional<InitListItem> make_init_item(const GlobalInit& init) {
    if (std::holds_alternative<std::monostate>(init)) return std::nullopt;
    InitListItem item{};
    if (const auto* scalar = std::get_if<InitScalar>(&init)) {
      item.value = *scalar;
    } else {
      item.value = std::make_shared<InitList>(std::get<InitList>(init));
    }
    return item;
  }

  bool is_string_scalar(const GlobalInit& init) const {
    const auto* scalar = std::get_if<InitScalar>(&init);
    if (!scalar) return false;
    const Expr& e = module_->expr_pool[scalar->expr.value];
    return std::holds_alternative<StringLiteral>(e.payload);
  }

  long long flat_scalar_count(const TypeSpec& ts) const {
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

  long long deduce_array_size_from_init(const GlobalInit& init) const {
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

  TypeSpec resolve_array_ts(const TypeSpec& ts, const GlobalInit& init) const {
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

  bool is_direct_char_array_init(const TypeSpec& ts, const GlobalInit& init) const {
    if (ts.array_rank != 1 || ts.ptr_level != 0) return false;
    if (!is_char_like(ts.base)) return false;
    return is_string_scalar(init);
  }

  bool union_allows_init_normalization(const TypeSpec& ts) const {
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

  bool struct_allows_init_normalization(const TypeSpec& ts) const {
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

  GlobalInit normalize_global_init(const TypeSpec& ts, const GlobalInit& init) {
    std::function<GlobalInit(const TypeSpec&, const InitList&, size_t&)> consume_from_flat;
    auto has_designators = [](const InitList& list) {
      return std::any_of(
          list.items.begin(), list.items.end(),
          [](const InitListItem& item) {
            return item.field_designator.has_value() || item.index_designator.has_value();
          });
    };
    auto find_aggregate_field_index =
        [&](const HirStructDef& sd, const InitListItem& item, size_t next_idx) -> std::optional<size_t> {
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
        [&](long long idx, const TypeSpec& target_ts, const GlobalInit& child) -> std::optional<InitListItem> {
      auto item = make_init_item(child);
      if (!item) return std::nullopt;
      item->index_designator = idx;
      item->field_designator.reset();
      if (target_ts.array_rank > 0 && target_ts.array_size >= 0) {
        item->resolved_array_bound = target_ts.array_size;
      }
      return item;
    };

    auto normalize_scalar_like = [&](const TypeSpec& cur_ts, const GlobalInit& cur_init) -> GlobalInit {
      if (const auto* scalar = std::get_if<InitScalar>(&cur_init)) return GlobalInit(*scalar);
      if (const auto* list = std::get_if<InitList>(&cur_init)) {
        if (!list->items.empty()) return normalize_global_init(cur_ts, child_init_of(list->items.front()));
      }
      return std::monostate{};
    };

    consume_from_flat = [&](const TypeSpec& cur_ts, const InitList& list, size_t& cursor) -> GlobalInit {
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
          // Shift array_dims to drop the outermost dimension
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
      if ((cur_ts.base == TB_STRUCT || cur_ts.base == TB_UNION) && cur_ts.ptr_level == 0 && cur_ts.tag) {
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
            if (auto it = make_field_mapped_item(sd, 0, field_ts, child)) out.items.push_back(std::move(*it));
          }
          return out;
        }
        InitList out{};
        for (size_t fi = 0; fi < sd.fields.size() && cursor < list.items.size(); ++fi) {
          TypeSpec field_ts = field_init_type_of(sd.fields[fi]);
          auto child = consume_from_flat(field_ts, list, cursor);
          field_ts = resolve_array_ts(field_ts, child);
          child = normalize_global_init(field_ts, child);
          if (auto it = make_field_mapped_item(sd, fi, field_ts, child)) out.items.push_back(std::move(*it));
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
        // Shift array_dims to drop the outermost dimension
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
      if (sit == module_->struct_defs.end()) return list ? init : normalize_scalar_like(ts, init);
      const auto& sd = sit->second;
      if (sd.is_union) return init;
      if (!struct_allows_init_normalization(ts)) return list ? init : normalize_scalar_like(ts, init);
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

  GlobalId lower_static_local_global(FunctionCtx& ctx, const Node* n) {
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

  GlobalInit lower_global_init(const Node* n,
                              FunctionCtx* ctx = nullptr,
                              bool allow_named_const_fold = false) {
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

  InitList lower_init_list(const Node* n, FunctionCtx* ctx = nullptr) {
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

  const Node* init_item_value_node(const Node* item) const {
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

  const Node* unwrap_init_scalar_value(const Node* node) const {
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

  bool has_side_effect_expr(const Node* n) const {
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

  bool is_simple_constant_expr(const Node* n) const {
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
        if (std::strcmp(op, "+") == 0 || std::strcmp(op, "-") == 0 || std::strcmp(op, "~") == 0)
          return is_simple_constant_expr(n->left);
        return false;
      }
      default:
        return false;
    }
  }

  bool can_fast_path_scalar_array_init(const Node* init_list) const {
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

  // Check whether a struct has any member fields whose types have destructors.
  bool struct_has_member_dtors(const std::string& tag) {
    auto sit = module_->struct_defs.find(tag);
    if (sit == module_->struct_defs.end()) return false;
    for (auto it = sit->second.fields.rbegin(); it != sit->second.fields.rend(); ++it) {
      if (it->elem_type.base == TB_STRUCT && it->elem_type.ptr_level == 0 &&
          it->elem_type.tag) {
        std::string ftag = it->elem_type.tag;
        if (struct_destructors_.count(ftag) || struct_has_member_dtors(ftag))
          return true;
      }
    }
    return false;
  }

  // Generate the synthetic body for a = default special member function.
  // - Default constructor: empty (alloca zeroed by default)
  // - Copy/move constructor: memberwise copy from source param to this
  // - Copy/move assignment: memberwise copy from source param to this + return this
  // - Default destructor: empty (member dtors handled by caller)
  void emit_defaulted_method_body(FunctionCtx& ctx, Function& fn,
                                   const std::string& struct_tag,
                                   const Node* method_node) {
    auto sit = module_->struct_defs.find(struct_tag);
    bool is_copy_or_move_ctor = method_node->is_constructor && method_node->n_params == 1;
    bool is_copy_or_move_assign = (method_node->operator_kind == OP_ASSIGN) && method_node->n_params == 1;

    if (is_copy_or_move_ctor || is_copy_or_move_assign) {
      // Emit memberwise copy: this->field = other.field for each field.
      if (sit != module_->struct_defs.end()) {
        // Build 'this' reference.
        DeclRef this_ref{};
        this_ref.name = "this";
        auto pit = ctx.params.find("this");
        if (pit != ctx.params.end()) this_ref.param_index = pit->second;
        TypeSpec this_ts{};
        this_ts.base = TB_STRUCT;
        this_ts.tag = sit->second.tag.c_str();
        this_ts.ptr_level = 1;
        ExprId this_id = append_expr(method_node, this_ref, this_ts, ValueCategory::LValue);

        // Build 'other' reference (second parameter — the source).
        // For ref params the storage is a pointer, so we use it directly.
        std::string other_name = method_node->params[0]->name
            ? method_node->params[0]->name : "<anon_param>";
        DeclRef other_ref{};
        other_ref.name = other_name;
        auto opit = ctx.params.find(other_name);
        if (opit != ctx.params.end()) other_ref.param_index = opit->second;
        TypeSpec other_ts = this_ts; // ptr to struct
        ExprId other_id = append_expr(method_node, other_ref, other_ts, ValueCategory::LValue);

        for (const auto& field : sit->second.fields) {
          TypeSpec field_ts = field.elem_type;
          // Restore array dim if present.
          if (field.array_first_dim >= 0) {
            field_ts.array_rank = 1;
            field_ts.array_size = field.array_first_dim;
          }

          // this->field
          MemberExpr lhs_me{};
          lhs_me.base = this_id;
          lhs_me.field = field.name;
          lhs_me.is_arrow = true;
          ExprId lhs_member = append_expr(method_node, lhs_me, field_ts, ValueCategory::LValue);

          // other->field
          MemberExpr rhs_me{};
          rhs_me.base = other_id;
          rhs_me.field = field.name;
          rhs_me.is_arrow = true;
          ExprId rhs_member = append_expr(method_node, rhs_me, field_ts, ValueCategory::LValue);

          // this->field = other->field
          AssignExpr ae{};
          ae.lhs = lhs_member;
          ae.rhs = rhs_member;
          ExprId assign_id = append_expr(method_node, ae, field_ts);
          ExprStmt es{}; es.expr = assign_id;
          append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
        }
      }
    }

    if (is_copy_or_move_assign) {
      // Return *this (as pointer for T& return type).
      DeclRef this_ref2{};
      this_ref2.name = "this";
      auto pit2 = ctx.params.find("this");
      if (pit2 != ctx.params.end()) this_ref2.param_index = pit2->second;
      TypeSpec this_ts2{};
      this_ts2.base = TB_STRUCT;
      this_ts2.tag = sit != module_->struct_defs.end()
                         ? sit->second.tag.c_str()
                         : struct_tag.c_str();
      this_ts2.ptr_level = 1;
      ExprId this_ret = append_expr(method_node, this_ref2, this_ts2, ValueCategory::LValue);
      ReturnStmt rs{};
      rs.expr = this_ret;
      append_stmt(ctx, Stmt{StmtPayload{rs}, make_span(method_node)});
    } else if (!method_node->is_destructor) {
      // Default ctor, copy/move ctor: return void.
      // (Destructors: ret void is emitted by caller after member dtor calls.)
      ReturnStmt rs{};
      append_stmt(ctx, Stmt{StmtPayload{rs}, make_span(method_node)});
    }
  }

  // Emit destructor calls for struct member fields that have destructors.
  // Calls are emitted in reverse field order. `this_ptr_id` is an ExprId
  // of type pointer-to-struct.
  void emit_member_dtor_calls(FunctionCtx& ctx, const std::string& struct_tag,
                              ExprId this_ptr_id, const Node* span_node) {
    auto sit = module_->struct_defs.find(struct_tag);
    if (sit == module_->struct_defs.end()) return;
    const auto& fields = sit->second.fields;
    // Reverse field order for destruction.
    for (auto it = fields.rbegin(); it != fields.rend(); ++it) {
      const auto& field = *it;
      if (field.elem_type.base != TB_STRUCT || field.elem_type.ptr_level != 0 ||
          !field.elem_type.tag)
        continue;
      std::string ftag = field.elem_type.tag;
      bool has_explicit_dtor = struct_destructors_.count(ftag) > 0;
      bool has_member_dtors = struct_has_member_dtors(ftag);
      if (!has_explicit_dtor && !has_member_dtors) continue;

      // Build GEP: &(this->field)
      MemberExpr me{};
      me.base = this_ptr_id;
      me.field = field.name;
      me.is_arrow = true;
      TypeSpec field_ts = field.elem_type;
      ExprId member_id = append_expr(span_node, me, field_ts, ValueCategory::LValue);
      UnaryExpr addr{};
      addr.op = UnaryOp::AddrOf;
      addr.operand = member_id;
      TypeSpec ptr_ts = field_ts; ptr_ts.ptr_level++;
      ExprId member_ptr_id = append_expr(span_node, addr, ptr_ts);

      if (has_explicit_dtor) {
        // Call the explicit destructor (it handles its own member dtors internally).
        auto dit = struct_destructors_.find(ftag);
        CallExpr c{};
        DeclRef callee_ref{};
        callee_ref.name = dit->second.mangled_name;
        TypeSpec fn_ts{}; fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts; callee_ts.ptr_level++;
        c.callee = append_expr(span_node, callee_ref, callee_ts);
        c.args.push_back(member_ptr_id);
        ExprId call_id = append_expr(span_node, c, fn_ts);
        ExprStmt es{}; es.expr = call_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(span_node)});
      } else {
        // No explicit dtor — recursively emit member dtors for this field's type.
        emit_member_dtor_calls(ctx, ftag, member_ptr_id, span_node);
      }
    }
  }

  // Emit destructor calls for locals pushed since dtor_stack index `since`.
  // Calls are emitted in reverse order (LIFO).
  void emit_dtor_calls(FunctionCtx& ctx, size_t since, const Node* span_node) {
    for (size_t i = ctx.dtor_stack.size(); i > since; --i) {
      const auto& dl = ctx.dtor_stack[i - 1];
      auto dit = struct_destructors_.find(dl.struct_tag);

      // Build the &var expression for this local.
      DeclRef var_ref{};
      var_ref.local = dl.local_id;
      auto lt = ctx.local_types.find(dl.local_id.value);
      TypeSpec var_ts{};
      if (lt != ctx.local_types.end()) var_ts = lt->second;
      ExprId var_id = append_expr(span_node, var_ref, var_ts, ValueCategory::LValue);
      UnaryExpr addr{};
      addr.op = UnaryOp::AddrOf;
      addr.operand = var_id;
      TypeSpec ptr_ts = var_ts; ptr_ts.ptr_level++;
      ExprId addr_id = append_expr(span_node, addr, ptr_ts);

      if (dit != struct_destructors_.end()) {
        // Call explicit destructor (which handles its own member dtors).
        CallExpr c{};
        DeclRef callee_ref{};
        callee_ref.name = dit->second.mangled_name;
        TypeSpec fn_ts{}; fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts; callee_ts.ptr_level++;
        c.callee = append_expr(span_node, callee_ref, callee_ts);
        c.args.push_back(addr_id);
        ExprId call_id = append_expr(span_node, c, fn_ts);
        ExprStmt es{}; es.expr = call_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(span_node)});
      } else {
        // No explicit dtor, but has member dtors — emit them directly.
        emit_member_dtor_calls(ctx, dl.struct_tag, addr_id, span_node);
      }
    }
  }

  void lower_stmt_node(FunctionCtx& ctx, const Node* n) {
    if (!n) return;

    switch (n->kind) {
      case NK_BLOCK: {
        // ival==1 marks synthetic multi-declarator blocks (e.g. int a, b;).
        // They share the current scope and must not discard bindings.
        const bool new_scope = (n->ival != 1);
        const auto saved_locals = ctx.locals;
        const auto saved_static_globals = ctx.static_globals;
        const auto saved_enum_consts = enum_consts_;
        const auto saved_local_consts = ctx.local_const_bindings;
        const size_t saved_dtor_depth = ctx.dtor_stack.size();
        for (int i = 0; i < n->n_children; ++i) {
          lower_stmt_node(ctx, n->children[i]);
        }
        if (new_scope) {
          emit_dtor_calls(ctx, saved_dtor_depth, n);
          ctx.dtor_stack.resize(saved_dtor_depth);
          ctx.locals = saved_locals;
          ctx.static_globals = saved_static_globals;
          enum_consts_ = saved_enum_consts;
          ctx.local_const_bindings = saved_local_consts;
        }
        return;
      }
      case NK_DECL: {
        lower_local_decl_stmt(ctx, n);
        return;
      }
      case NK_EXPR_STMT: {
        ExprStmt s{};
        if (n->left) s.expr = lower_expr(&ctx, n->left);
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        return;
      }
      case NK_ASM: {
        InlineAsmStmt s{};
        s.asm_template = rewrite_gcc_asm_template(decode_string_node(n->left));
        s.has_side_effects = true;
        if (n->asm_num_outputs == 1 && n->children[0]) {
          s.output = lower_expr(&ctx, n->children[0]);
          s.output_type.spec = n->children[0]->type;
          s.output_type.category = ValueCategory::LValue;
        }
        for (int i = 0; i < n->asm_num_inputs; ++i) {
          const Node* input = n->children[n->asm_num_outputs + i];
          if (!input) continue;
          s.inputs.push_back(lower_expr(&ctx, input));
        }
        for (int i = 0; i < n->asm_n_constraints; ++i) {
          const std::string constraint = strip_quoted_string(n->asm_constraints[i]);
          if (!s.constraints.empty()) s.constraints += ",";
          s.constraints += constraint;
        }
        for (int i = 0; i < n->asm_num_clobbers; ++i) {
          const Node* clobber =
              n->children[n->asm_num_outputs + n->asm_num_inputs + i];
          std::string name = decode_string_node(clobber);
          if (name.empty()) continue;
          if (!s.constraints.empty()) s.constraints += ",";
          s.constraints += "~{" + name + "}";
        }
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        return;
      }
      case NK_RETURN: {
        ReturnStmt s{};
        if (n->left) {
          ExprId val = lower_expr(&ctx, n->left);
          // For reference return types, return the address of the expression.
          if (ctx.fn && is_any_ref_ts(ctx.fn->return_type.spec)) {
            UnaryExpr addr{};
            addr.op = UnaryOp::AddrOf;
            addr.operand = val;
            TypeSpec ptr_ts = ctx.fn->return_type.spec;
            ptr_ts.is_lvalue_ref = false;
            ptr_ts.is_rvalue_ref = false;
            ptr_ts.ptr_level++;
            val = append_expr(n, addr, ptr_ts);
          }
          s.expr = val;
        }
        // Emit destructor calls for all locals in scope before returning.
        emit_dtor_calls(ctx, 0, n);
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        return;
      }
      case NK_IF: {
        // if constexpr: try compile-time branch elimination
        if (n->is_constexpr) {
          const Node* cond_node = n->cond ? n->cond : n->left;
          ConstEvalEnv cenv{&enum_consts_, &const_int_bindings_, &ctx.local_const_bindings};
          auto cr = evaluate_constant_expr(cond_node, cenv);
          if (cr.ok()) {
            // Condition is compile-time known — only lower the selected branch
            if (cr.as_int()) {
              lower_stmt_node(ctx, n->then_);
            } else if (n->else_) {
              lower_stmt_node(ctx, n->else_);
            }
            return;
          }
          // Fall through to regular if when condition can't be folded
          // (e.g. template-dependent sizeof)
        }

        IfStmt s{};
        {
          const Node* cond_n = n->cond ? n->cond : n->left;
          s.cond = maybe_bool_convert(&ctx, lower_expr(&ctx, cond_n), cond_n);
        }
        const BlockId then_b = create_block(ctx);
        s.then_block = then_b;
        if (n->else_) s.else_block = create_block(ctx);
        const BlockId after_b = create_block(ctx);
        s.after_block = after_b;
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

        ctx.current_block = then_b;
        lower_stmt_node(ctx, n->then_);
        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = after_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }

        if (n->else_) {
          ctx.current_block = *s.else_block;
          lower_stmt_node(ctx, n->else_);
          if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
            GotoStmt j{};
            j.target.resolved_block = after_b;
            append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
            ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
          }
        }

        ctx.current_block = after_b;
        return;
      }
      case NK_WHILE: {
        const BlockId cond_b = create_block(ctx);
        const BlockId body_b = create_block(ctx);
        const BlockId after_b = create_block(ctx);

        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = cond_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }

        ctx.current_block = cond_b;
        WhileStmt s{};
        {
          const Node* cond_n = n->cond ? n->cond : n->left;
          s.cond = maybe_bool_convert(&ctx, lower_expr(&ctx, cond_n), cond_n);
        }
        s.body_block = body_b;
        s.continue_target = cond_b;
        s.break_target = after_b;
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

        ctx.break_stack.push_back(after_b);
        ctx.continue_stack.push_back(cond_b);
        ctx.current_block = body_b;
        lower_stmt_node(ctx, n->body);
        if (ctx.current_block.value != after_b.value &&
            !ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          ContinueStmt c{};
          c.target = cond_b;
          append_stmt(ctx, Stmt{StmtPayload{c}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }
        ctx.break_stack.pop_back();
        ctx.continue_stack.pop_back();

        ctx.current_block = after_b;
        return;
      }
      case NK_FOR: {
        ForStmt s{};
        if (n->init) {
          // If the for-init is a declaration, lower it as a statement first.
          // This handles `for (int i = 0; ...)` in C99.
          const bool init_is_decl = (n->init->kind == NK_DECL ||
                                     n->init->kind == NK_BLOCK);
          if (init_is_decl) {
            lower_stmt_node(ctx, n->init);
            // Leave s.init as default (no init expression in the ForStmt).
          } else {
            s.init = lower_expr(&ctx, n->init);
          }
        }
        if (n->cond) s.cond = maybe_bool_convert(&ctx, lower_expr(&ctx, n->cond), n->cond);
        if (n->update) s.update = lower_expr(&ctx, n->update);
        const BlockId body_b = create_block(ctx);
        const BlockId after_b = create_block(ctx);
        s.body_block = body_b;
        s.continue_target = body_b;
        s.break_target = after_b;
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

        ctx.break_stack.push_back(after_b);
        ctx.continue_stack.push_back(body_b);
        ctx.current_block = body_b;
        lower_stmt_node(ctx, n->body);
        if (ctx.current_block.value != after_b.value &&
            !ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          ContinueStmt c{};
          c.target = body_b;
          append_stmt(ctx, Stmt{StmtPayload{c}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }
        ctx.break_stack.pop_back();
        ctx.continue_stack.pop_back();

        ctx.current_block = after_b;
        return;
      }

      // ── Range-for desugaring ──────────────────────────────────────────────
      // for (Type var : range_expr) body
      // →
      // auto __range_begin = range_expr.begin();
      // auto __range_end   = range_expr.end();
      // for (; __range_begin != __range_end; ++__range_begin) {
      //   Type var = *__range_begin;
      //   body
      // }
      case NK_RANGE_FOR: {
        const Node* decl_node  = n->init;   // loop variable declaration
        const Node* range_node = n->right;  // range expression

        // 1. Infer range expression type — must be a struct with begin/end.
        TypeSpec range_ts = infer_generic_ctrl_type(&ctx, range_node);
        if (range_ts.base != TB_STRUCT || !range_ts.tag) {
          throw std::runtime_error(
              std::string("error: range-for expression is not a struct type (line ")
              + std::to_string(n->line) + ")");
        }

        // 2. Look up begin() and end() methods.
        auto find_method = [&](const char* method_name) -> std::string {
          std::string base_key = std::string(range_ts.tag) + "::" + method_name;
          std::string const_key = base_key + "_const";
          decltype(struct_methods_)::iterator mit;
          if (range_ts.is_const) {
            mit = struct_methods_.find(const_key);
            if (mit == struct_methods_.end())
              mit = struct_methods_.find(base_key);
          } else {
            mit = struct_methods_.find(base_key);
            if (mit == struct_methods_.end())
              mit = struct_methods_.find(const_key);
          }
          if (mit == struct_methods_.end()) {
            throw std::runtime_error(
                std::string("error: range-for: no ") + method_name
                + "() method on struct " + range_ts.tag
                + " (line " + std::to_string(n->line) + ")");
          }
          return mit->second;
        };

        std::string begin_mangled = find_method("begin");
        std::string end_mangled   = find_method("end");

        // 3. Determine iterator type from begin()'s return type.
        TypeSpec iter_ts{};
        iter_ts.base = TB_VOID;
        {
          auto fit = module_->fn_index.find(begin_mangled);
          if (fit != module_->fn_index.end() &&
              fit->second.value < module_->functions.size())
            iter_ts = module_->functions[fit->second.value].return_type.spec;
        }
        if (iter_ts.base == TB_VOID) {
          auto rit = struct_method_ret_types_.find(
              std::string(range_ts.tag) + "::begin");
          if (rit == struct_method_ret_types_.end())
            rit = struct_method_ret_types_.find(
                std::string(range_ts.tag) + "::begin_const");
          if (rit != struct_method_ret_types_.end())
            iter_ts = rit->second;
        }
        if (iter_ts.base == TB_VOID) {
          throw std::runtime_error(
              std::string("error: range-for: cannot determine iterator type from begin() (line ")
              + std::to_string(n->line) + ")");
        }

        // 4. Lower range expression and build begin()/end() calls.
        ExprId range_id = lower_expr(&ctx, range_node);

        auto build_method_call = [&](const std::string& mangled,
                                     ExprId obj_id) -> ExprId {
          CallExpr cc{};
          DeclRef dr{};
          dr.name = mangled;
          TypeSpec callee_ts = iter_ts;
          callee_ts.ptr_level++;
          cc.callee = append_expr(n, dr, callee_ts);
          // this pointer: &obj
          UnaryExpr addr{};
          addr.op = UnaryOp::AddrOf;
          addr.operand = obj_id;
          TypeSpec ptr_ts = range_ts;
          ptr_ts.ptr_level++;
          cc.args.push_back(append_expr(n, addr, ptr_ts));
          return append_expr(n, cc, iter_ts);
        };

        ExprId begin_call = build_method_call(begin_mangled, range_id);
        ExprId end_call   = build_method_call(end_mangled, range_id);

        // 5. Create hidden locals __range_begin and __range_end.
        auto make_iter_local = [&](const char* name, ExprId init_expr) -> LocalId {
          LocalDecl ld{};
          ld.id = next_local_id();
          ld.name = name;
          ld.type = qtype_from(iter_ts, ValueCategory::LValue);
          ld.init = init_expr;
          LocalId lid = ld.id;
          ctx.locals[name] = lid;
          ctx.local_types[lid.value] = iter_ts;
          append_stmt(ctx, Stmt{StmtPayload{std::move(ld)}, make_span(n)});
          return lid;
        };

        LocalId begin_lid = make_iter_local("__range_begin", begin_call);
        LocalId end_lid   = make_iter_local("__range_end", end_call);

        // Helper: build DeclRef expression for a local.
        auto ref_local = [&](const char* name, LocalId lid) -> ExprId {
          DeclRef dr{};
          dr.name = name;
          dr.local = lid;
          return append_expr(n, dr, iter_ts, ValueCategory::LValue);
        };

        // 6. Build condition: __range_begin != __range_end
        //    Use operator!= if available on the iterator struct.
        ExprId cond_expr;
        {
          std::string neq_base = std::string(iter_ts.tag) + "::operator_neq";
          std::string neq_const = neq_base + "_const";
          auto mit = struct_methods_.find(neq_base);
          if (mit == struct_methods_.end())
            mit = struct_methods_.find(neq_const);
          if (mit == struct_methods_.end()) {
            throw std::runtime_error(
                std::string("error: range-for: iterator type ") + iter_ts.tag
                + " has no operator!= (line " + std::to_string(n->line) + ")");
          }
          CallExpr cc{};
          DeclRef dr{};
          dr.name = mit->second;
          TypeSpec bool_ts{};
          bool_ts.base = TB_BOOL;
          TypeSpec callee_ts = bool_ts;
          callee_ts.ptr_level++;
          cc.callee = append_expr(n, dr, callee_ts);
          // this: &__range_begin
          UnaryExpr addr{};
          addr.op = UnaryOp::AddrOf;
          addr.operand = ref_local("__range_begin", begin_lid);
          TypeSpec ptr_ts = iter_ts;
          ptr_ts.ptr_level++;
          cc.args.push_back(append_expr(n, addr, ptr_ts));
          // arg: __range_end (by value)
          cc.args.push_back(ref_local("__range_end", end_lid));
          cond_expr = append_expr(n, cc, bool_ts);
        }

        // 7. Build update: ++__range_begin (prefix operator++)
        ExprId update_expr;
        {
          std::string inc_base = std::string(iter_ts.tag) + "::operator_preinc";
          std::string inc_const = inc_base + "_const";
          auto mit = struct_methods_.find(inc_base);
          if (mit == struct_methods_.end())
            mit = struct_methods_.find(inc_const);
          if (mit == struct_methods_.end()) {
            throw std::runtime_error(
                std::string("error: range-for: iterator type ") + iter_ts.tag
                + " has no prefix operator++ (line " + std::to_string(n->line) + ")");
          }
          CallExpr cc{};
          DeclRef dr{};
          dr.name = mit->second;
          // Get return type of operator++
          TypeSpec inc_ret_ts = iter_ts;
          {
            auto fit2 = module_->fn_index.find(mit->second);
            if (fit2 != module_->fn_index.end() &&
                fit2->second.value < module_->functions.size())
              inc_ret_ts = module_->functions[fit2->second.value].return_type.spec;
          }
          TypeSpec callee_ts = inc_ret_ts;
          callee_ts.ptr_level++;
          cc.callee = append_expr(n, dr, callee_ts);
          UnaryExpr addr{};
          addr.op = UnaryOp::AddrOf;
          addr.operand = ref_local("__range_begin", begin_lid);
          TypeSpec ptr_ts = iter_ts;
          ptr_ts.ptr_level++;
          cc.args.push_back(append_expr(n, addr, ptr_ts));
          update_expr = append_expr(n, cc, inc_ret_ts);
        }

        // 8. Build ForStmt.
        ForStmt fs{};
        fs.cond = cond_expr;
        fs.update = update_expr;
        const BlockId body_b = create_block(ctx);
        const BlockId after_b = create_block(ctx);
        fs.body_block = body_b;
        fs.continue_target = body_b;
        fs.break_target = after_b;
        append_stmt(ctx, Stmt{StmtPayload{fs}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

        ctx.break_stack.push_back(after_b);
        ctx.continue_stack.push_back(body_b);
        ctx.current_block = body_b;

        // 9. In body: declare loop variable = *__range_begin
        {
          // Build *__range_begin (operator*)
          ExprId deref_expr;
          TypeSpec deref_ret_ts{};
          deref_ret_ts.base = TB_INT; // fallback
          {
            std::string deref_base = std::string(iter_ts.tag) + "::operator_deref";
            std::string deref_const = deref_base + "_const";
            auto mit = struct_methods_.find(deref_base);
            if (mit == struct_methods_.end())
              mit = struct_methods_.find(deref_const);
            if (mit == struct_methods_.end()) {
              throw std::runtime_error(
                  std::string("error: range-for: iterator type ") + iter_ts.tag
                  + " has no operator* (line " + std::to_string(n->line) + ")");
            }
            CallExpr cc{};
            DeclRef dr{};
            dr.name = mit->second;
            // Get return type of operator*
            {
              auto fit2 = module_->fn_index.find(mit->second);
              if (fit2 != module_->fn_index.end() &&
                  fit2->second.value < module_->functions.size())
                deref_ret_ts = module_->functions[fit2->second.value].return_type.spec;
            }
            if (deref_ret_ts.base == TB_VOID || deref_ret_ts.base == TB_INT) {
              auto rit = struct_method_ret_types_.find(
                  std::string(iter_ts.tag) + "::operator_deref");
              if (rit == struct_method_ret_types_.end())
                rit = struct_method_ret_types_.find(
                    std::string(iter_ts.tag) + "::operator_deref_const");
              if (rit != struct_method_ret_types_.end())
                deref_ret_ts = rit->second;
            }
            TypeSpec callee_ts = deref_ret_ts;
            callee_ts.ptr_level++;
            cc.callee = append_expr(n, dr, callee_ts);
            UnaryExpr addr{};
            addr.op = UnaryOp::AddrOf;
            addr.operand = ref_local("__range_begin", begin_lid);
            TypeSpec ptr_ts = iter_ts;
            ptr_ts.ptr_level++;
            cc.args.push_back(append_expr(n, addr, ptr_ts));
            // Use storage type for call annotation when operator* returns a ref:
            // the LLVM call will return ptr, so annotate the expr as ptr too.
            deref_expr = append_expr(n, cc, reference_storage_ts(deref_ret_ts));
          }

          // Declare loop variable with init = deref_expr
          if (decl_node) {
            LocalDecl ld{};
            ld.id = next_local_id();
            ld.name = decl_node->name ? decl_node->name : "__range_var";
            TypeSpec var_ts = decl_node->type;
            bool is_ref = var_ts.is_lvalue_ref;
            // auto type deduction: use operator* return type
            if (var_ts.base == TB_AUTO) {
              bool was_const = var_ts.is_const;
              var_ts = deref_ret_ts;
              // Strip reference from deduced type — auto& adds its own reference
              var_ts.is_lvalue_ref = false;
              // Preserve const/ref qualifiers from the declaration
              if (was_const) var_ts.is_const = true;
              if (is_ref) var_ts.is_lvalue_ref = true;
            }
            resolve_typedef_to_struct(var_ts);
            ld.type = qtype_from(reference_storage_ts(var_ts), ValueCategory::LValue);
            if (is_ref) {
              // Reference loop variable: operator* returns T& (stored as T*),
              // which is already a pointer — use it directly as the reference
              // storage. If operator* returns by value, wrap in AddrOf.
              if (deref_ret_ts.is_lvalue_ref) {
                // operator* returns a reference — result is already a pointer
                ld.init = deref_expr;
              } else {
                // operator* returns by value — take address of the result
                UnaryExpr addr{};
                addr.op = UnaryOp::AddrOf;
                addr.operand = deref_expr;
                ld.init = append_expr(n, addr, ld.type.spec);
              }
            } else {
              ld.init = deref_expr;
            }
            ctx.locals[ld.name] = ld.id;
            ctx.local_types[ld.id.value] = var_ts;
            append_stmt(ctx, Stmt{StmtPayload{std::move(ld)}, make_span(n)});
          }
        }

        // 10. Lower user body.
        if (n->body) lower_stmt_node(ctx, n->body);

        if (ctx.current_block.value != after_b.value &&
            !ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          ContinueStmt cont{};
          cont.target = body_b;
          append_stmt(ctx, Stmt{StmtPayload{cont}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }
        ctx.break_stack.pop_back();
        ctx.continue_stack.pop_back();
        ctx.current_block = after_b;
        return;
      }

      case NK_DO_WHILE: {
        const BlockId body_b = create_block(ctx);
        const BlockId cond_b = create_block(ctx);
        const BlockId after_b = create_block(ctx);

        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = body_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }

        ctx.break_stack.push_back(after_b);
        ctx.continue_stack.push_back(cond_b);
        ctx.current_block = body_b;
        lower_stmt_node(ctx, n->body);
        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = cond_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }

        ctx.current_block = cond_b;
        DoWhileStmt s{};
        s.body_block = body_b;
        {
          const Node* cond_n = n->cond ? n->cond : n->left;
          s.cond = maybe_bool_convert(&ctx, lower_expr(&ctx, cond_n), cond_n);
        }
        s.continue_target = cond_b;
        s.break_target = after_b;
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        ensure_block(ctx, cond_b).has_explicit_terminator = true;

        ctx.break_stack.pop_back();
        ctx.continue_stack.pop_back();
        ctx.current_block = after_b;
        return;
      }
      case NK_SWITCH: {
        SwitchStmt s{};
        s.cond = lower_expr(&ctx, n->cond ? n->cond : n->left);
        const BlockId body_b = create_block(ctx);
        s.body_block = body_b;
        const BlockId after_b = create_block(ctx);
        s.break_block = after_b;
        const BlockId parent_b = ctx.current_block;
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

        ctx.switch_stack.push_back({parent_b, {}, {}, {}});
        ctx.break_stack.push_back(after_b);
        ctx.current_block = body_b;
        lower_stmt_node(ctx, n->body);
        ctx.break_stack.pop_back();

        // If the current block (last one in the switch body) has no explicit
        // terminator, branch to after_b (covers Duff's device after-do-while case).
        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = after_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }

        // Update the SwitchStmt with collected case blocks
        if (!ctx.switch_stack.empty()) {
          auto& sw_ctx = ctx.switch_stack.back();
          for (auto& blk : ctx.fn->blocks) {
            if (blk.id.value != parent_b.value) continue;
            for (auto& stmt : blk.stmts) {
              if (auto* sw = std::get_if<SwitchStmt>(&stmt.payload)) {
                sw->case_blocks = std::move(sw_ctx.cases);
                sw->case_range_blocks = std::move(sw_ctx.case_ranges);
                if (sw_ctx.default_block) sw->default_block = sw_ctx.default_block;
                break;
              }
            }
            break;
          }
          ctx.switch_stack.pop_back();
        }

        ctx.current_block = after_b;
        return;
      }
      case NK_CASE: {
        long long case_val = 0;
        if (n->left) {
          if (n->left->kind == NK_INT_LIT) {
            case_val = n->left->ival;
          } else {
            ConstEvalEnv env{&enum_consts_, nullptr, &ctx.local_const_bindings};
            if (auto r = evaluate_constant_expr(n->left, env); r.ok())
              case_val = r.as_int();
          }
        }
        if (!ctx.switch_stack.empty()) {
          // Create a dedicated block for this case entry point.
          const BlockId case_b = create_block(ctx);
          // Fall-through from previous block (if not already terminated).
          if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
            GotoStmt j{};
            j.target.resolved_block = case_b;
            append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
            ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
          }
          ctx.switch_stack.back().cases.push_back({case_val, case_b});
          ctx.current_block = case_b;
        }
        CaseStmt s{};
        s.value = case_val;
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        lower_stmt_node(ctx, n->body);
        return;
      }
      case NK_CASE_RANGE: {
        long long lo = 0, hi = 0;
        {
          ConstEvalEnv env{&enum_consts_, nullptr, &ctx.local_const_bindings};
          if (n->left) {
            if (n->left->kind == NK_INT_LIT) lo = n->left->ival;
            else if (auto r = evaluate_constant_expr(n->left, env); r.ok()) lo = r.as_int();
          }
          if (n->right) {
            if (n->right->kind == NK_INT_LIT) hi = n->right->ival;
            else if (auto r = evaluate_constant_expr(n->right, env); r.ok()) hi = r.as_int();
          }
        }
        if (!ctx.switch_stack.empty()) {
          const BlockId case_b = create_block(ctx);
          if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
            GotoStmt j{};
            j.target.resolved_block = case_b;
            append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
            ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
          }
          ctx.switch_stack.back().case_ranges.push_back({lo, hi, case_b});
          ctx.current_block = case_b;
        }
        CaseRangeStmt s{};
        s.range.lo = lo;
        s.range.hi = hi;
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        lower_stmt_node(ctx, n->body);
        return;
      }
      case NK_DEFAULT: {
        if (!ctx.switch_stack.empty()) {
          const BlockId def_b = create_block(ctx);
          if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
            GotoStmt j{};
            j.target.resolved_block = def_b;
            append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
            ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
          }
          ctx.switch_stack.back().default_block = def_b;
          ctx.current_block = def_b;
        }
        append_stmt(ctx, Stmt{StmtPayload{DefaultStmt{}}, make_span(n)});
        lower_stmt_node(ctx, n->body);
        return;
      }
      case NK_BREAK: {
        BreakStmt s{};
        if (!ctx.break_stack.empty()) s.target = ctx.break_stack.back();
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        return;
      }
      case NK_CONTINUE: {
        ContinueStmt s{};
        if (!ctx.continue_stack.empty()) s.target = ctx.continue_stack.back();
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        return;
      }
      case NK_LABEL: {
        LabelStmt s{};
        s.name = n->name ? n->name : "<anon_label>";
        const BlockId lb = create_block(ctx);
        ctx.label_blocks[s.name] = lb;
        // Append LabelStmt to current block (hir_to_llvm will emit "ulbl_name:" here).
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        // Always branch from the LabelStmt's sub-block to lb (the body block).
        // LabelStmt resets hir_to_llvm's last_term state (starts a new LLVM basic block),
        // so we always need a terminator for the "ulbl_name:" sub-block regardless of
        // the HIR block's existing has_explicit_terminator flag.
        {
          GotoStmt j{};
          j.target.resolved_block = lb;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
        }
        ctx.current_block = lb;
        lower_stmt_node(ctx, n->body);
        return;
      }
      case NK_GOTO: {
        // GCC computed goto: goto *expr
        if (n->name && std::string(n->name) == "__computed__" && n->left) {
          IndirBrStmt ib{};
          ib.target = lower_expr(&ctx, n->left);
          append_stmt(ctx, Stmt{StmtPayload{ib}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
          return;
        }
        GotoStmt s{};
        s.target.user_name = n->name ? n->name : "<anon_label>";
        auto it = ctx.label_blocks.find(s.target.user_name);
        if (it != ctx.label_blocks.end()) {
          // Backward goto: label already defined, use its block directly.
          s.target.resolved_block = it->second;
        }
        // Forward goto: leave resolved_block invalid; hir_to_llvm emits "br label %ulbl_name"
        // which connects to the "ulbl_name:" emitted by the LabelStmt later.
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        return;
      }
      case NK_EMPTY: {
        append_stmt(ctx, Stmt{StmtPayload{ExprStmt{}}, make_span(n)});
        return;
      }
      case NK_ENUM_DEF: {
        collect_enum_def(n);
        return;
      }
      case NK_INVALID_EXPR:
      case NK_INVALID_STMT:
        // Error recovery placeholders — silently skip.
        return;
      default: {
        ExprStmt s{};
        s.expr = lower_expr(&ctx, n);
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        return;
      }
    }
  }

  ExprId lower_stmt_expr_block(FunctionCtx& ctx, const Node* block, const TypeSpec& result_ts) {
    if (!block || block->kind != NK_BLOCK) {
      TypeSpec ts = result_ts;
      if (ts.base == TB_VOID) ts.base = TB_INT;
      return append_expr(nullptr, IntLiteral{0, false}, ts);
    }

    const bool new_scope = (block->ival != 1);
    const auto saved_locals = ctx.locals;
    const auto saved_static_globals = ctx.static_globals;

    ExprId result{};
    bool have_result = false;
    for (int i = 0; i < block->n_children; ++i) {
      const Node* child = block->children[i];
      const bool is_last = (i + 1 == block->n_children);
      if (is_last && child && child->kind == NK_EXPR_STMT && child->left &&
          result_ts.base != TB_VOID) {
        result = lower_expr(&ctx, child->left);
        have_result = true;
        continue;
      }
      lower_stmt_node(ctx, child);
    }

    if (new_scope) {
      ctx.locals = saved_locals;
      ctx.static_globals = saved_static_globals;
    }

    if (have_result) return result;
    TypeSpec ts = result_ts;
    if (ts.base == TB_VOID) ts.base = TB_INT;
    return append_expr(nullptr, IntLiteral{0, false}, ts);
  }

  // Try to resolve an operator expression on a struct type to a member operator
  // method call.  Returns a valid ExprId if the struct has the corresponding
  // operator method, or an invalid ExprId otherwise.
  //
  // `obj_node`      – AST node for the object (LHS for binary/index, operand
  //                   for unary)
  // `op_method_name` – canonical mangled name, e.g. "operator_subscript"
  // `arg_nodes`     – additional argument AST nodes (index for [], RHS for ==,
  //                   etc.)
  // `result_node`   – the top-level expression AST node (for source location)
  ExprId try_lower_operator_call(
      FunctionCtx* ctx, const Node* result_node,
      const Node* obj_node, const char* op_method_name,
      const std::vector<const Node*>& arg_nodes,
      const std::vector<ExprId>& extra_args = {}) {
    TypeSpec obj_ts = infer_generic_ctrl_type(ctx, obj_node);
    // If the object is a pointer-to-struct, it's not directly a struct value.
    if (obj_ts.ptr_level != 0 || obj_ts.base != TB_STRUCT || !obj_ts.tag)
      return ExprId::invalid();

    // Select const vs non-const overload based on object constness.
    std::string base_key = std::string(obj_ts.tag) + "::" + op_method_name;
    std::string const_key = base_key + "_const";
    decltype(struct_methods_)::iterator mit;
    if (obj_ts.is_const) {
      // Const object: must use const overload.
      mit = struct_methods_.find(const_key);
      if (mit == struct_methods_.end())
        mit = struct_methods_.find(base_key); // fallback to non-const
    } else {
      // Non-const object: prefer non-const, fallback to const.
      mit = struct_methods_.find(base_key);
      if (mit == struct_methods_.end())
        mit = struct_methods_.find(const_key);
    }
    if (mit == struct_methods_.end()) return ExprId::invalid();

    // Resolve ref-overloaded operators (e.g., operator=(const T&) vs operator=(T&&)).
    std::string resolved_mangled = mit->second;
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
          if (ov.param_is_lvalue_ref[i] && !arg_is_lvalue) { viable = false; break; }
          if (ov.param_is_rvalue_ref[i] && arg_is_lvalue) { viable = false; break; }
          if (ov.param_is_rvalue_ref[i] && !arg_is_lvalue) score += 2;
          else if (ov.param_is_lvalue_ref[i] && arg_is_lvalue) score += 2;
          else score += 1;
        }
        if (viable && score > best_score) { best_name = &ov.mangled_name; best_score = score; }
      }
      if (best_name) resolved_mangled = *best_name;
    }

    // Found the operator method. Build a CallExpr.
    CallExpr c{};

    // Callee: DeclRef pointing to the mangled method name.
    DeclRef dr{};
    dr.name = resolved_mangled;
    auto fit = module_->fn_index.find(dr.name);
    TypeSpec fn_ts{};
    fn_ts.base = TB_VOID;
    if (fit != module_->fn_index.end() &&
        fit->second.value < module_->functions.size()) {
      fn_ts = module_->functions[fit->second.value].return_type.spec;
    }
    // Fallback: if fn_index doesn't have the return type yet (method not
    // lowered), use struct_method_ret_types_ which is populated at collection.
    if (fn_ts.base == TB_VOID) {
      auto rit = struct_method_ret_types_.find(
          std::string(obj_ts.tag) + "::" + op_method_name);
      if (rit != struct_method_ret_types_.end()) fn_ts = rit->second;
    }
    TypeSpec callee_ts = fn_ts;
    callee_ts.ptr_level++;
    c.callee = append_expr(result_node, dr, callee_ts);

    // First arg: &obj (this pointer).
    ExprId obj_id = lower_expr(ctx, obj_node);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = obj_id;
    TypeSpec ptr_ts = obj_ts;
    ptr_ts.ptr_level++;
    c.args.push_back(append_expr(obj_node, addr, ptr_ts));

    // Remaining args: explicit operands.
    const Function* callee_fn = nullptr;
    if (fit != module_->fn_index.end() &&
        fit->second.value < module_->functions.size())
      callee_fn = &module_->functions[fit->second.value];
    // Fallback: find the method AST node from pending_methods_ for param types.
    const Node* method_ast = nullptr;
    if (!callee_fn) {
      for (const auto& pm : pending_methods_) {
        if (pm.mangled == resolved_mangled) { method_ast = pm.method_node; break; }
      }
    }
    for (size_t i = 0; i < arg_nodes.size(); ++i) {
      const TypeSpec* param_ts =
          (callee_fn && (i + 1) < callee_fn->params.size())
              ? &callee_fn->params[i + 1].type.spec
              : nullptr;
      // Fallback param type from AST method node.
      TypeSpec ast_param_ts{};
      if (!param_ts && method_ast && (int)i < method_ast->n_params) {
        ast_param_ts = method_ast->params[i]->type;
        param_ts = &ast_param_ts;
      }
      // Handle reference parameters: pass address instead of value.
      if (param_ts && (param_ts->is_rvalue_ref || param_ts->is_lvalue_ref)) {
        const Node* arg = arg_nodes[i];
        const Node* inner = arg;
        // Unwrap static_cast<T&&>(x) to get x for xvalue semantics.
        if (inner->kind == NK_CAST && inner->left &&
            inner->type.is_rvalue_ref)
          inner = inner->left;
        ExprId arg_val = lower_expr(ctx, inner);
        TypeSpec storage_ts = reference_storage_ts(*param_ts);
        UnaryExpr addr_e{};
        addr_e.op = UnaryOp::AddrOf;
        addr_e.operand = arg_val;
        c.args.push_back(append_expr(arg, addr_e, storage_ts));
      } else {
        c.args.push_back(lower_expr(ctx, arg_nodes[i]));
      }
    }
    for (auto ea : extra_args) c.args.push_back(ea);

    return append_expr(result_node, c, fn_ts);
  }

  // If the expression resolves to a struct type that has operator_bool,
  // insert an implicit call to operator_bool(). Otherwise return as-is.
  ExprId maybe_bool_convert(FunctionCtx* ctx, ExprId expr, const Node* n) {
    if (!expr.valid() || !n) return expr;
    TypeSpec ts = infer_generic_ctrl_type(ctx, n);
    if (ts.ptr_level != 0 || ts.base != TB_STRUCT || !ts.tag)
      return expr;
    std::string base_key = std::string(ts.tag) + "::operator_bool";
    std::string const_key = base_key + "_const";
    auto mit = struct_methods_.find(base_key);
    if (mit == struct_methods_.end())
      mit = struct_methods_.find(const_key);
    if (mit == struct_methods_.end()) return expr;

    // Build a CallExpr to operator_bool with &obj as this.
    CallExpr c{};
    DeclRef dr{};
    dr.name = mit->second;
    auto fit = module_->fn_index.find(dr.name);
    TypeSpec fn_ts{};
    fn_ts.base = TB_BOOL;
    if (fit != module_->fn_index.end() &&
        fit->second.value < module_->functions.size()) {
      fn_ts = module_->functions[fit->second.value].return_type.spec;
    }
    TypeSpec callee_ts = fn_ts;
    callee_ts.ptr_level++;
    c.callee = append_expr(n, dr, callee_ts);

    // &obj (this pointer)
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = expr;
    TypeSpec ptr_ts = ts;
    ptr_ts.ptr_level++;
    c.args.push_back(append_expr(n, addr, ptr_ts));

    return append_expr(n, c, fn_ts);
  }

  ExprId lower_expr(FunctionCtx* ctx, const Node* n) {
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
      case NK_FLOAT_LIT:
      {
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
      case NK_CHAR_LIT:
      {
        // In C, character constants have type int.
        TypeSpec ts = n->type;
        ts.base = TB_INT;
        return append_expr(n, CharLiteral{n->ival}, ts);
      }
      case NK_VAR: {
        if (n->name && n->name[0]) {
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
        // Inside a struct method body, resolve unqualified names as this->field
        // when the name is not a local, param, or global.
        if (ctx && !ctx->method_struct_tag.empty() && !has_local_binding &&
            !r.param_index && !r.global) {
          auto sit = module_->struct_defs.find(ctx->method_struct_tag);
          if (sit != module_->struct_defs.end()) {
            for (const auto& fld : sit->second.fields) {
              if (fld.name == r.name) {
                // Rewrite as this->field: load `this` param, then MemberExpr.
                DeclRef this_ref{};
                this_ref.name = "this";
                auto pit = ctx->params.find("this");
                if (pit != ctx->params.end()) this_ref.param_index = pit->second;
                TypeSpec this_ts{};
                this_ts.base = TB_STRUCT;
                // Use persistent tag from struct_defs.
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
        // If the name refers to a function (not a variable), annotate the
        // DeclRef with a function-pointer type so codegen does not need to
        // reconstruct it from fn_index at emit time.
        TypeSpec var_ts = n->type;
        if (var_ts.base == TB_VOID && var_ts.ptr_level == 0 &&
            var_ts.array_rank == 0 && !r.local && !r.param_index && !r.global) {
          auto fit = module_->fn_index.find(r.name);
          if (fit != module_->fn_index.end() &&
              fit->second.value < module_->functions.size()) {
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
        // Try prefix operator++ / operator-- on struct types.
        if (n->op) {
          const char* op_name = nullptr;
          if (std::string(n->op) == "++pre") op_name = "operator_preinc";
          else if (std::string(n->op) == "--pre") op_name = "operator_predec";
          else if (std::string(n->op) == "+") op_name = "operator_plus";
          else if (std::string(n->op) == "-") op_name = "operator_minus";
          else if (std::string(n->op) == "!") op_name = "operator_not";
          else if (std::string(n->op) == "~") op_name = "operator_bitnot";
          if (op_name) {
            ExprId op = try_lower_operator_call(
                ctx, n, n->left, op_name, {});
            if (op.valid()) return op;
          }
        }
        UnaryExpr u{};
        u.op = map_unary_op(n->op);
        u.operand = lower_expr(ctx, n->left);
        // Implicit operator bool for logical NOT on struct types.
        if (u.op == UnaryOp::Not)
          u.operand = maybe_bool_convert(ctx, u.operand, n->left);
        return append_expr(n, u, n->type);
      }
      case NK_POSTFIX: {
        // Try postfix operator++ / operator-- on struct types.
        if (n->op) {
          const char* op_name = nullptr;
          std::string op_str(n->op);
          if (op_str == "++") op_name = "operator_postinc";
          else if (op_str == "--") op_name = "operator_postdec";
          if (op_name) {
            // Postfix operator takes a dummy int 0 argument.
            TypeSpec int_ts{};
            int_ts.base = TB_INT;
            ExprId dummy = append_expr(n, IntLiteral{0, false}, int_ts);
            ExprId op = try_lower_operator_call(
                ctx, n, n->left, op_name, {}, {dummy});
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
          ExprId op = try_lower_operator_call(
              ctx, n, n->left, "operator_addr", {});
          if (op.valid()) return op;
        }
        // Reject taking the address of a consteval function.
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
        // Try operator* on struct types.
        {
          ExprId op = try_lower_operator_call(
              ctx, n, n->left, "operator_deref", {});
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
        // Try operator== / operator!= on struct types.
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
            ExprId op = try_lower_operator_call(
                ctx, n, n->left, op_name, {n->right});
            if (op.valid()) return op;
          }
        }
        BinaryExpr b{};
        b.op = map_binary_op(n->op);
        b.lhs = lower_expr(ctx, n->left);
        b.rhs = lower_expr(ctx, n->right);
        // Implicit operator bool for logical && / || on struct operands.
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
            ExprId op = try_lower_operator_call(
                ctx, n, n->left, op_name, {n->right});
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
            ExprId op = try_lower_operator_call(
                ctx, n, n->left, op_name, {n->right});
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
        // Substitute template type parameters in cast target type.
        TypeSpec cast_ts = n->type;
        if (ctx && !ctx->tpl_bindings.empty() &&
            cast_ts.base == TB_TYPEDEF && cast_ts.tag) {
          auto it = ctx->tpl_bindings.find(cast_ts.tag);
          if (it != ctx->tpl_bindings.end()) {
            const TypeSpec& concrete = it->second;
            cast_ts.base = concrete.base;
            cast_ts.tag = concrete.tag;
          }
        }
        if (ctx && !ctx->tpl_bindings.empty() && cast_ts.tpl_struct_origin)
          resolve_pending_tpl_struct(cast_ts, ctx->tpl_bindings, ctx->nttp_bindings);
        c.to_type = qtype_from(cast_ts);
        c.expr = lower_expr(ctx, n->left);
        // Phase C: build fn_ptr_sig for casts to callable types.
        if (cast_ts.is_fn_ptr) {
          c.fn_ptr_sig = fn_ptr_sig_from_decl_node(n);
        }
        return append_expr(n, c, cast_ts);
      }
      case NK_CALL:
      case NK_BUILTIN_CALL: {
        return lower_call_expr(ctx, n);
      }
      case NK_VA_ARG: {
        VaArgExpr v{};
        v.ap = lower_expr(ctx, n->left);
        return append_expr(n, v, n->type);
      }
      case NK_INDEX: {
        // Try operator[] on struct types.
        if (n->right) {
          ExprId op = try_lower_operator_call(
              ctx, n, n->left, "operator_subscript", {n->right});
          if (op.valid()) return op;
        }
        IndexExpr idx{};
        idx.base = lower_expr(ctx, n->left);
        idx.index = lower_expr(ctx, n->right);
        return append_expr(n, idx, n->type, ValueCategory::LValue);
      }
      case NK_MEMBER: {
        // Try operator-> on struct types when arrow access is used.
        // C++ chains operator-> until a raw pointer is obtained.
        if (n->is_arrow && n->left) {
          ExprId arrow_ptr = try_lower_operator_call(
              ctx, n, n->left, "operator_arrow", {});
          if (arrow_ptr.valid()) {
            // Chain: if the result is a struct (not a pointer), call its
            // operator->() repeatedly until we get a raw pointer.
            for (int depth = 0; depth < 8; ++depth) {
              const Expr* res_expr = module_->find_expr(arrow_ptr);
              if (!res_expr) break;
              TypeSpec rts = res_expr->type.spec;
              if (rts.ptr_level > 0) break; // raw pointer — done chaining
              if (rts.base != TB_STRUCT || !rts.tag) break;
              // Result is a struct; check for operator_arrow on it.
              std::string base_key = std::string(rts.tag) + "::operator_arrow";
              std::string const_key = base_key + "_const";
              auto mit = rts.is_const
                  ? struct_methods_.find(const_key)
                  : struct_methods_.find(base_key);
              if (mit == struct_methods_.end())
                mit = rts.is_const
                    ? struct_methods_.find(base_key)
                    : struct_methods_.find(const_key);
              if (mit == struct_methods_.end()) break;
              // Build call: method(&tmp) where tmp holds the
              // intermediate struct result (rvalue needs storage).
              auto fit = module_->fn_index.find(mit->second);
              TypeSpec fn_ts{};
              fn_ts.base = TB_VOID;
              if (fit != module_->fn_index.end() &&
                  fit->second.value < module_->functions.size())
                fn_ts = module_->functions[fit->second.value].return_type.spec;
              if (fn_ts.base == TB_VOID) {
                auto rit2 = struct_method_ret_types_.find(
                    std::string(rts.tag) + "::operator_arrow");
                if (rit2 == struct_method_ret_types_.end())
                  rit2 = struct_method_ret_types_.find(
                      std::string(rts.tag) + "::operator_arrow_const");
                if (rit2 != struct_method_ret_types_.end())
                  fn_ts = rit2->second;
              }
              // Store intermediate result in a temp local so we can
              // take its address for the this-pointer.
              LocalDecl tmp{};
              tmp.id = next_local_id();
              tmp.name = "__arrow_tmp";
              tmp.type = qtype_from(rts, ValueCategory::LValue);
              tmp.init = arrow_ptr;
              const LocalId tmp_lid = tmp.id;
              ctx->locals[tmp.name] = tmp.id;
              ctx->local_types[tmp.id.value] = rts;
              append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
              // Reference the temp local.
              DeclRef tmp_ref{};
              tmp_ref.name = "__arrow_tmp";
              tmp_ref.local = tmp_lid;
              ExprId tmp_id = append_expr(n, tmp_ref, rts, ValueCategory::LValue);
              // Call method(&tmp)
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
            // Apply field access on the final pointer.
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
        // If either branch contains a statement expression, lower to
        // if/else with a temp variable so side effects are conditional.
        if (ctx && (contains_stmt_expr(n->then_) || contains_stmt_expr(n->else_))) {
          TypeSpec result_ts = n->type;
          if (result_ts.base == TB_VOID) result_ts.base = TB_INT;

          // Create temp local
          LocalDecl tmp{};
          tmp.id = next_local_id();
          tmp.name = "__tern_tmp";
          tmp.type = qtype_from(result_ts, ValueCategory::LValue);
          TypeSpec zero_ts{}; zero_ts.base = TB_INT;
          tmp.init = append_expr(n, IntLiteral{0, false}, zero_ts);
          const LocalId tmp_lid = tmp.id;
          ctx->locals[tmp.name] = tmp.id;
          ctx->local_types[tmp.id.value] = result_ts;
          append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});

          // Lower condition
          const Node* cond_n = n->cond ? n->cond : n->left;
          ExprId cond_expr = maybe_bool_convert(ctx, lower_expr(ctx, cond_n), cond_n);

          // Create then/else/after blocks
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
            // tmp = val
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

          // Return DeclRef to tmp
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
        // sizeof always returns an integer (size_t ~ unsigned long)
        TypeSpec ts{}; ts.base = TB_ULONG;
        return append_expr(n, s, ts);
      }
      case NK_SIZEOF_TYPE: {
        // Substitute template type parameters in sizeof(T).
        TypeSpec sizeof_target = n->type;
        if (ctx && !ctx->tpl_bindings.empty() &&
            sizeof_target.base == TB_TYPEDEF && sizeof_target.tag) {
          auto it = ctx->tpl_bindings.find(sizeof_target.tag);
          if (it != ctx->tpl_bindings.end()) {
            const TypeSpec& concrete = it->second;
            sizeof_target.base = concrete.base;
            sizeof_target.tag = concrete.tag;
            if (concrete.ptr_level > 0)
              sizeof_target.ptr_level += concrete.ptr_level;
          }
        }
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

          TypeSpec ts{};
          ts.base = TB_ULONG;
          const ExprId count_id = lower_expr(ctx, n->type.array_size_expr);
          const ExprId elem_sz_id = append_expr(
              n, IntLiteral{static_cast<long long>(type_size_bytes(*module_, elem_ts)), false}, ts);
          BinaryExpr mul{};
          mul.op = BinaryOp::Mul;
          mul.lhs = count_id;
          mul.rhs = elem_sz_id;
          return append_expr(n, mul, ts);
        }
        // For concrete non-VLA types, lower sizeof(type) directly to a constant.
        const int size = type_size_bytes(*module_, sizeof_target);
        TypeSpec ts{}; ts.base = TB_ULONG;
        return append_expr(n, IntLiteral{static_cast<long long>(size), false}, ts);
      }
      case NK_ALIGNOF_TYPE: {
        // Substitute template type parameters in alignof(T).
        TypeSpec alignof_target = n->type;
        if (ctx && !ctx->tpl_bindings.empty() &&
            alignof_target.base == TB_TYPEDEF && alignof_target.tag) {
          auto it = ctx->tpl_bindings.find(alignof_target.tag);
          if (it != ctx->tpl_bindings.end()) {
            const TypeSpec& concrete = it->second;
            alignof_target.base = concrete.base;
            alignof_target.tag = concrete.tag;
            if (concrete.ptr_level > 0)
              alignof_target.ptr_level += concrete.ptr_level;
          }
        }
        const int align = type_align_bytes(*module_, alignof_target);
        TypeSpec ts{}; ts.base = TB_ULONG;
        return append_expr(n, IntLiteral{static_cast<long long>(align), false}, ts);
      }
      case NK_ALIGNOF_EXPR: {
        // __alignof__(expr) — alignment of the expression's type.
        TypeSpec expr_ts = infer_generic_ctrl_type(ctx, n->left);
        int align = 0;
        // For function identifiers, check the function's aligned attribute.
        if (n->left && n->left->kind == NK_VAR && n->left->name) {
          const std::string fn_name(n->left->name);
          auto fit = module_->fn_index.find(fn_name);
          if (fit != module_->fn_index.end() &&
              fit->second.value < module_->functions.size()) {
            int fa = module_->functions[fit->second.value].attrs.align_bytes;
            if (fa > 0) align = fa;
          }
        }
        if (align == 0) {
          if (expr_ts.align_bytes > 0)
            align = expr_ts.align_bytes;
          else
            align = type_align_bytes(*module_, expr_ts);
        }
        TypeSpec ts{}; ts.base = TB_ULONG;
        return append_expr(n, IntLiteral{static_cast<long long>(align), false}, ts);
      }
      case NK_COMPOUND_LIT: {
        // A compound literal (T){ ... } is an lvalue with automatic storage
        // duration.  Create an anonymous local, initialize it, and return a
        // DeclRef so callers can take its address or load from it.
        if (!ctx) {
          TypeSpec ts = n->type;
          if (ts.base == TB_VOID) ts.base = TB_INT;
          return append_expr(n, IntLiteral{0, false}, ts);
        }
        const TypeSpec clit_ts = n->type;
        const Node* init_list = (n->left && n->left->kind == NK_INIT_LIST) ? n->left : nullptr;

        // Deduce array size from init list if the type is unsized (e.g. int[]).
        TypeSpec actual_ts = clit_ts;
        if (actual_ts.array_rank > 0 && actual_ts.array_size < 0 && init_list) {
          long long count = init_list->n_children;
          for (int ci = 0; ci < init_list->n_children; ++ci) {
            const Node* item = init_list->children[ci];
            if (item && item->kind == NK_INIT_ITEM && item->is_designated &&
                item->is_index_desig && item->desig_val + 1 > count)
              count = item->desig_val + 1;
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
            (decl_ts.base == TB_STRUCT || decl_ts.base == TB_UNION) &&
            decl_ts.ptr_level == 0 && decl_ts.array_rank == 0;
        const bool is_array = decl_ts.array_rank > 0;
        const bool is_vector = is_vector_ty(decl_ts);

        if ((is_struct_or_union || is_array || is_vector) && init_list) {
          // Aggregate compound literal: emit zeroinitializer store first, then
          // overlay explicit element/field assignments below.
          TypeSpec int_ts{}; int_ts.base = TB_INT;
          d.init = append_expr(n, IntLiteral{0, false}, int_ts);
        } else if (init_list && init_list->n_children > 0) {
          d.init = lower_expr(ctx, unwrap_init_scalar_value(init_list));
        } else if (n->left && !init_list) {
          d.init = lower_expr(ctx, n->left);
        }

        ctx->local_types[lid.value] = decl_ts;
        append_stmt(*ctx, Stmt{StmtPayload{std::move(d)}, make_span(n)});

        DeclRef dr{}; dr.name = "<clit>"; dr.local = lid;
        ExprId base_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);

        if (init_list && (is_struct_or_union || is_array || is_vector)) {
          auto is_agg = [](const TypeSpec& ts) {
            return ((ts.base == TB_STRUCT || ts.base == TB_UNION) &&
                    ts.ptr_level == 0 && ts.array_rank == 0) ||
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
                  ExprId val_id = append_expr(
                      n, IntLiteral{vals[static_cast<size_t>(idx)], false}, idx_ts);
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
          auto can_direct_assign_agg = [&](const TypeSpec& lhs_ts,
                                           const Node* rhs_node) -> bool {
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
                  while (cursor < list_node->n_children && next_field < sd.fields.size()) {
                    const Node* item = list_node->children[cursor];
                    if (!item) {
                      ++cursor;
                      ++next_field;
                      continue;
                    }
                    size_t fi = next_field;
                    if (item->kind == NK_INIT_ITEM && item->is_designated &&
                        !item->is_index_desig && item->desig_field) {
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
                      } else if (val_node &&
                                 item->kind == NK_INIT_ITEM && item->is_designated &&
                                 !item->is_index_desig && item->desig_field) {
                        // Designated init with explicit non-list value — direct assign
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
      case NK_INVALID_EXPR:
      case NK_INVALID_STMT: {
        // Error recovery placeholder — emit a zero literal.
        TypeSpec ts{};
        ts.base = TB_INT;
        return append_expr(n, IntLiteral{0, false}, ts);
      }
      default: {
        TypeSpec ts = n->type;
        if (ts.base == TB_VOID && n->kind != NK_CALL) {
          ts.base = TB_INT;
        }
        return append_expr(n, IntLiteral{0, false}, ts);
      }
    }
  }

  // ── Template multi-instantiation support ──────────────────────────────────

  // AST-side discovered template work item.
  //
  // This is the "todo list" shape we eventually want AST preprocessing to own:
  // a seed describing a discovered template application, not proof that the
  // specialization has been fully realized/lowered yet.
  //
  // TemplateSeedOrigin, TemplateSeedWorkItem, TemplateInstance, and
  // InstantiationRegistry are defined in compile_time_engine.hpp.


  // Build a mangled name for an explicit specialization node by mapping its
  // template_arg_types/values to the generic template's param names.
  std::string mangle_specialization(const Node* spec, const Node* generic_def) {
    if (!spec || !generic_def || generic_def->n_template_params <= 0) return "";
    TypeBindings bindings;
    NttpBindings nttp_bindings;
    int n = std::min(spec->n_template_args, generic_def->n_template_params);
    for (int i = 0; i < n; ++i) {
      const char* pname = generic_def->template_param_names[i];
      if (!pname) continue;
      bool is_nttp = generic_def->template_param_is_nttp &&
                     generic_def->template_param_is_nttp[i];
      if (is_nttp) {
        if (spec->template_arg_is_value && spec->template_arg_is_value[i])
          nttp_bindings[pname] = spec->template_arg_values[i];
      } else {
        bindings[pname] = spec->template_arg_types[i];
      }
    }
    return mangle_template_name(spec->name, bindings, nttp_bindings);
  }

  // Build TypeBindings from a call-site template args and a function definition.
  // Resolves typedef args through `enclosing_bindings` if provided.
  // Fills missing args from fn_def's default template parameters.
  TypeBindings build_call_bindings(const Node* call_var, const Node* fn_def,
                                   const TypeBindings* enclosing_bindings) {
    TypeBindings bindings;
    if (!call_var || !fn_def || fn_def->n_template_params <= 0) return bindings;
    // Allow n_template_args == 0 when defaults are available.
    int explicit_count = call_var->n_template_args > 0
        ? std::min(call_var->n_template_args, fn_def->n_template_params) : 0;
    for (int i = 0; i < explicit_count; ++i) {
      if (!fn_def->template_param_names[i]) continue;
      // Skip NTTP params in type bindings — they go in nttp_bindings.
      if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) continue;
      TypeSpec arg_ts = call_var->template_arg_types[i];
      // Resolve typedef template args through enclosing function's bindings.
      if (arg_ts.base == TB_TYPEDEF && arg_ts.tag && enclosing_bindings) {
        auto resolved = enclosing_bindings->find(arg_ts.tag);
        if (resolved != enclosing_bindings->end()) arg_ts = resolved->second;
      }
      bindings[fn_def->template_param_names[i]] = arg_ts;
    }
    // Fill remaining type params from defaults.
    if (fn_def->template_param_has_default) {
      for (int i = explicit_count; i < fn_def->n_template_params; ++i) {
        if (!fn_def->template_param_names[i]) continue;
        if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) continue;
        if (fn_def->template_param_has_default[i]) {
          bindings[fn_def->template_param_names[i]] = fn_def->template_param_default_types[i];
        }
      }
    }
    return bindings;
  }

  NttpBindings build_call_nttp_bindings(const Node* call_var, const Node* fn_def,
                                        const NttpBindings* enclosing_nttp = nullptr) {
    NttpBindings bindings;
    if (!call_var || !fn_def || fn_def->n_template_params <= 0) return bindings;
    int explicit_count = call_var->n_template_args > 0
        ? std::min(call_var->n_template_args, fn_def->n_template_params) : 0;
    for (int i = 0; i < explicit_count; ++i) {
      if (!fn_def->template_param_names[i]) continue;
      if (!fn_def->template_param_is_nttp || !fn_def->template_param_is_nttp[i]) continue;
      if (call_var->template_arg_is_value && call_var->template_arg_is_value[i]) {
        // Check if this is a forwarded NTTP name that needs resolution.
        if (call_var->template_arg_nttp_names && call_var->template_arg_nttp_names[i]) {
          if (enclosing_nttp) {
            auto it = enclosing_nttp->find(call_var->template_arg_nttp_names[i]);
            if (it != enclosing_nttp->end()) {
              bindings[fn_def->template_param_names[i]] = it->second;
              continue;
            }
          }
          // Unresolved forwarded NTTP — skip (will be resolved during deferred instantiation).
          continue;
        }
        bindings[fn_def->template_param_names[i]] = call_var->template_arg_values[i];
      }
    }
    // Fill remaining NTTP params from defaults.
    if (fn_def->template_param_has_default) {
      for (int i = explicit_count; i < fn_def->n_template_params; ++i) {
        if (!fn_def->template_param_names[i]) continue;
        if (!fn_def->template_param_is_nttp || !fn_def->template_param_is_nttp[i]) continue;
        if (fn_def->template_param_has_default[i]) {
          bindings[fn_def->template_param_names[i]] = fn_def->template_param_default_values[i];
        }
      }
    }
    return bindings;
  }

  // Check if a template instantiation has already been discovered as a seed.
  bool has_seed(const std::string& fn_name, const std::string& mangled) {
    return registry_.has_seed_or_instance(fn_name, mangled);
  }

  // Check if a call node has any forwarded NTTP names (not yet resolved to values).
  static bool has_forwarded_nttp(const Node* call_var) {
    if (!call_var || !call_var->template_arg_nttp_names) return false;
    for (int i = 0; i < call_var->n_template_args; ++i) {
      if (call_var->template_arg_nttp_names[i]) return true;
    }
    return false;
  }

  // ── Template argument deduction ──────────────────────────────────────────

  // Try to infer the type of an AST expression node for template argument
  // deduction.  Only handles cases where the type is statically obvious
  // from the AST (literals, typed variables, address-of, casts).
  // Returns nullopt when the type cannot be determined.
  static std::optional<TypeSpec> try_infer_arg_type_for_deduction(
      const Node* expr, const Node* enclosing_fn) {
    if (!expr) return std::nullopt;

    // If the node already carries a concrete type, use it.
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
        // Check enclosing function parameters.
        for (int i = 0; i < enclosing_fn->n_params; ++i) {
          const Node* p = enclosing_fn->params[i];
          if (p && p->name && std::strcmp(p->name, expr->name) == 0 &&
              has_concrete_type(p->type))
            return p->type;
        }
        // Check local declarations in the function body (shallow walk).
        if (enclosing_fn->body) {
          const Node* body = enclosing_fn->body;
          for (int i = 0; i < body->n_children; ++i) {
            const Node* stmt = body->children[i];
            if (stmt && stmt->kind == NK_DECL && stmt->name &&
                std::strcmp(stmt->name, expr->name) == 0 &&
                has_concrete_type(stmt->type))
              return stmt->type;
          }
        }
        return std::nullopt;
      }
      case NK_CAST: {
        // Cast target type is in the node's type field.
        if (has_concrete_type(expr->type)) return expr->type;
        return std::nullopt;
      }
      case NK_ADDR: {
        // Address-of: &expr → pointer to expr's type.
        if (expr->left) {
          auto inner = try_infer_arg_type_for_deduction(expr->left, enclosing_fn);
          if (inner) {
            inner->ptr_level++;
            return inner;
          }
        }
        return std::nullopt;
      }
      case NK_DEREF: {
        // Dereference: *expr → remove one pointer level.
        if (expr->left) {
          auto inner = try_infer_arg_type_for_deduction(expr->left, enclosing_fn);
          if (inner && inner->ptr_level > 0) {
            inner->ptr_level--;
            return inner;
          }
        }
        return std::nullopt;
      }
      case NK_UNARY: {
        // Unary operators: try to infer from the operand for simple cases.
        return std::nullopt;
      }
      default:
        return std::nullopt;
    }
  }

  // Try to deduce template type arguments from call arguments.
  // Matches each function parameter whose type is a template type parameter
  // (TB_TYPEDEF with tag matching a template param name) against the
  // corresponding call argument's inferred type.
  // Returns the deduced bindings.  May be incomplete if some params cannot
  // be deduced (caller should fill from defaults or reject).
  static TypeBindings try_deduce_template_type_args(
      const Node* call_node, const Node* fn_def, const Node* enclosing_fn) {
    TypeBindings deduced;
    if (!call_node || !fn_def || fn_def->n_template_params <= 0) return deduced;

    // Build set of type parameter names (skip NTTPs).
    std::unordered_set<std::string> type_param_names;
    for (int i = 0; i < fn_def->n_template_params; ++i) {
      if (fn_def->template_param_names[i] &&
          !(fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]))
        type_param_names.insert(fn_def->template_param_names[i]);
    }
    if (type_param_names.empty()) return deduced;

    // Match each function parameter against the corresponding call argument.
    int n_args = call_node->n_children;
    int n_params = fn_def->n_params;
    int match_count = std::min(n_args, n_params);

    for (int i = 0; i < match_count; ++i) {
      const Node* param = fn_def->params[i];
      const Node* arg = call_node->children[i];
      if (!param || !arg) continue;

      const TypeSpec& param_ts = param->type;

      // Case 1: parameter type IS the template type param directly (e.g., T x).
      if (param_ts.base == TB_TYPEDEF && param_ts.tag &&
          type_param_names.count(param_ts.tag) &&
          param_ts.ptr_level == 0 && param_ts.array_rank == 0) {
        auto arg_type = try_infer_arg_type_for_deduction(arg, enclosing_fn);
        if (!arg_type) continue;
        // Strip array/pointer qualifiers from arg to get the base type for T.
        TypeSpec deduced_ts = *arg_type;
        auto existing = deduced.find(param_ts.tag);
        if (existing != deduced.end()) {
          // Conflict check: deduced types must match.
          if (existing->second.base != deduced_ts.base ||
              existing->second.ptr_level != deduced_ts.ptr_level ||
              existing->second.tag != deduced_ts.tag)
            return {};  // Conflicting deductions → fail.
        } else {
          deduced[param_ts.tag] = deduced_ts;
        }
      }
      // Case 2: parameter type is T* (pointer to template param).
      else if (param_ts.base == TB_TYPEDEF && param_ts.tag &&
               type_param_names.count(param_ts.tag) &&
               param_ts.ptr_level > 0 && param_ts.array_rank == 0) {
        auto arg_type = try_infer_arg_type_for_deduction(arg, enclosing_fn);
        if (!arg_type || arg_type->ptr_level < param_ts.ptr_level) continue;
        TypeSpec deduced_ts = *arg_type;
        deduced_ts.ptr_level -= param_ts.ptr_level;
        auto existing = deduced.find(param_ts.tag);
        if (existing != deduced.end()) {
          if (existing->second.base != deduced_ts.base ||
              existing->second.ptr_level != deduced_ts.ptr_level)
            return {};
        } else {
          deduced[param_ts.tag] = deduced_ts;
        }
      }
    }

    return deduced;
  }

  // Check if deduced bindings cover all required type parameters (those
  // without defaults).
  static bool deduction_covers_all_type_params(const TypeBindings& deduced,
                                                const Node* fn_def) {
    for (int i = 0; i < fn_def->n_template_params; ++i) {
      if (!fn_def->template_param_names[i]) continue;
      if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) continue;
      const std::string pname = fn_def->template_param_names[i];
      if (deduced.count(pname)) continue;
      // Not deduced — check for default.
      if (fn_def->template_param_has_default && fn_def->template_param_has_default[i]) continue;
      return false;  // Missing type param with no default.
    }
    return true;
  }

  // Fill missing deduced bindings from defaults.
  static void fill_deduced_defaults(TypeBindings& deduced, const Node* fn_def) {
    if (!fn_def->template_param_has_default) return;
    for (int i = 0; i < fn_def->n_template_params; ++i) {
      if (!fn_def->template_param_names[i]) continue;
      if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) continue;
      const std::string pname = fn_def->template_param_names[i];
      if (deduced.count(pname)) continue;
      if (fn_def->template_param_has_default[i])
        deduced[pname] = fn_def->template_param_default_types[i];
    }
  }

  // ── End template argument deduction ────────────────────────────────────

  // Check if all bindings are concrete (no unresolved TB_TYPEDEF).

  // Get template parameter names in declaration order.
  static std::vector<std::string> get_template_param_order(const Node* fn_def) {
    std::vector<std::string> params;
    if (!fn_def) return params;
    for (int i = 0; i < fn_def->n_template_params; ++i) {
      if (fn_def->template_param_names[i])
        params.emplace_back(fn_def->template_param_names[i]);
    }
    return params;
  }

  // Get template parameter order from the definition (for specialization key).
  std::vector<std::string> get_template_param_order_from_instances(const std::string& fn_name) {
    const Node* tpl_def = ct_state_->find_template_def(fn_name);
    if (tpl_def) return get_template_param_order(tpl_def);
    return {};
  }

  // Record a template seed via the centralized registry.
  // Returns the mangled name, or "" if bindings are not concrete.
  std::string record_seed(const std::string& fn_name, TypeBindings bindings,
                           NttpBindings nttp_bindings = {},
                           TemplateSeedOrigin origin = TemplateSeedOrigin::DirectCall) {
    auto param_order = get_template_param_order_from_instances(fn_name);
    return registry_.record_seed(fn_name, std::move(bindings),
                                 std::move(nttp_bindings), param_order,
                                 origin);
  }

  // Resolve the mangled name for a call to a template function.
  std::string resolve_template_call_name(const Node* call_var,
                                          const TypeBindings* enclosing_bindings,
                                          const NttpBindings* enclosing_nttp = nullptr) {
    if (!call_var || !call_var->name ||
        (call_var->n_template_args <= 0 && !call_var->has_template_args))
      return call_var ? (call_var->name ? call_var->name : "") : "";
    const Node* fn_def = ct_state_->find_template_def(call_var->name);
    if (!fn_def) return call_var->name;
    if (fn_def->is_consteval) return call_var->name;  // consteval handled separately
    TypeBindings bindings = build_call_bindings(call_var, fn_def, enclosing_bindings);
    NttpBindings nttp_bindings = build_call_nttp_bindings(call_var, fn_def, enclosing_nttp);
    return mangle_template_name(call_var->name, bindings, nttp_bindings);
  }

  // Recursively collect template instantiations from call sites in AST.
  void collect_template_instantiations(const Node* n, const Node* enclosing_fn) {
    if (!n) return;
    if (n->kind == NK_CALL && n->left && n->left->kind == NK_VAR &&
        n->left->name &&
        (n->left->n_template_args > 0 || n->left->has_template_args)) {
      const Node* fn_def = ct_state_->find_template_def(n->left->name);
      if (fn_def) {
        if (!fn_def->is_consteval && fn_def->n_template_params > 0) {
          // Determine the enclosing function's template bindings (if any).
          const TypeBindings* enclosing_bindings = nullptr;
          TypeBindings enc_bindings;
          if (enclosing_fn && enclosing_fn->name) {
            auto* enc_list = registry_.find_instances(enclosing_fn->name);
            if (enc_list) {
              // For each enclosing instantiation, record an inner instantiation.
              for (const auto& enc_inst : *enc_list) {
                TypeBindings inner = build_call_bindings(n->left, fn_def, &enc_inst.bindings);
                NttpBindings call_nttp = build_call_nttp_bindings(n->left, fn_def, &enc_inst.nttp_bindings);
                std::string mangled = mangle_template_name(n->left->name, inner, call_nttp);
                if (!has_seed(n->left->name, mangled))
                  record_seed(
                      n->left->name, std::move(inner), call_nttp,
                      TemplateSeedOrigin::EnclosingTemplateExpansion);
              }
              goto recurse;  // Already handled all enclosing instantiations.
            }
          }
          {
            NttpBindings call_nttp = build_call_nttp_bindings(n->left, fn_def);
            if (has_forwarded_nttp(n->left)) goto recurse;  // Deferred: forwarded NTTPs not yet resolved.
            TypeBindings bindings = build_call_bindings(n->left, fn_def, nullptr);
            std::string mangled = mangle_template_name(n->left->name, bindings, call_nttp);
            if (!has_seed(n->left->name, mangled))
              record_seed(
                  n->left->name, std::move(bindings), call_nttp,
                  TemplateSeedOrigin::DirectCall);
          }
        }
      }
    }
    // Template argument deduction: if the call has no explicit template args
    // but the callee name matches a template function, try to deduce type
    // args from the call arguments.
    if (n->kind == NK_CALL && n->left && n->left->kind == NK_VAR &&
        n->left->name &&
        n->left->n_template_args == 0 && !n->left->has_template_args) {
      const Node* fn_def = ct_state_->find_template_def(n->left->name);
      if (fn_def) {
        if (!fn_def->is_consteval && fn_def->n_template_params > 0) {
          TypeBindings deduced = try_deduce_template_type_args(n, fn_def, enclosing_fn);

          if (deduction_covers_all_type_params(deduced, fn_def)) {
            fill_deduced_defaults(deduced, fn_def);
            NttpBindings nttp{};
            // Fill NTTP defaults if any.
            if (fn_def->template_param_has_default) {
              for (int i = 0; i < fn_def->n_template_params; ++i) {
                if (!fn_def->template_param_names[i]) continue;
                if (!fn_def->template_param_is_nttp || !fn_def->template_param_is_nttp[i]) continue;
                if (fn_def->template_param_has_default[i])
                  nttp[fn_def->template_param_names[i]] = fn_def->template_param_default_values[i];
              }
            }
            std::string mangled = mangle_template_name(n->left->name, deduced, nttp);
            if (!has_seed(n->left->name, mangled))
              record_seed(
                  n->left->name, TypeBindings(deduced), nttp,
                  TemplateSeedOrigin::DeducedCall);
            // Store the deduction result for use during call lowering.
            deduced_template_calls_[n] = {mangled, std::move(deduced), std::move(nttp)};
          }
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
    for (int i = 0; i < n->n_children; ++i)
      if (n->children[i]) collect_template_instantiations(n->children[i], enclosing_fn);
  }

  // Like collect_template_instantiations, but only records instances for
  // consteval template functions.  Non-consteval template calls inside template
  // bodies are left for the compile-time reduction pass to discover and lower.
  void collect_consteval_template_instantiations(const Node* n, const Node* enclosing_fn) {
    if (!n) return;
    if (n->kind == NK_CALL && n->left && n->left->kind == NK_VAR &&
        n->left->name &&
        (n->left->n_template_args > 0 || n->left->has_template_args)) {
      const Node* fn_def = ct_state_->find_template_def(n->left->name);
      if (fn_def) {
        if (fn_def->is_consteval && fn_def->n_template_params > 0) {
          if (enclosing_fn && enclosing_fn->name) {
            auto* enc_list = registry_.find_instances(enclosing_fn->name);
            if (enc_list) {
              for (const auto& enc_inst : *enc_list) {
                TypeBindings inner = build_call_bindings(n->left, fn_def, &enc_inst.bindings);
                NttpBindings call_nttp = build_call_nttp_bindings(n->left, fn_def, &enc_inst.nttp_bindings);
                std::string mangled = mangle_template_name(n->left->name, inner, call_nttp);
                if (!has_seed(n->left->name, mangled))
                  record_seed(
                      n->left->name, std::move(inner), call_nttp,
                      TemplateSeedOrigin::ConstevalEnclosingExpansion);
              }
              goto recurse_ce;
            }
            // Enclosing template function has no concrete instances yet.
            // Skip recording — the consteval call will be discovered when
            // the enclosing function is later instantiated by the HIR
            // compile-time reduction pass (deferred consteval).
            if (enclosing_fn->n_template_params > 0)
              goto recurse_ce;
          }
          {
            NttpBindings call_nttp = build_call_nttp_bindings(n->left, fn_def);
            if (has_forwarded_nttp(n->left)) goto recurse_ce;  // Deferred.
            TypeBindings bindings = build_call_bindings(n->left, fn_def, nullptr);
            std::string mangled = mangle_template_name(n->left->name, bindings, call_nttp);
            if (!has_seed(n->left->name, mangled))
              record_seed(
                  n->left->name, std::move(bindings), call_nttp,
                  TemplateSeedOrigin::ConstevalSeed);
          }
        }
      }
    }
    recurse_ce:
    if (n->left) collect_consteval_template_instantiations(n->left, enclosing_fn);
    if (n->right) collect_consteval_template_instantiations(n->right, enclosing_fn);
    if (n->cond) collect_consteval_template_instantiations(n->cond, enclosing_fn);
    if (n->then_) collect_consteval_template_instantiations(n->then_, enclosing_fn);
    if (n->else_) collect_consteval_template_instantiations(n->else_, enclosing_fn);
    if (n->body) collect_consteval_template_instantiations(n->body, enclosing_fn);
    if (n->init) collect_consteval_template_instantiations(n->init, enclosing_fn);
    if (n->update) collect_consteval_template_instantiations(n->update, enclosing_fn);
    for (int i = 0; i < n->n_children; ++i)
      if (n->children[i]) collect_consteval_template_instantiations(n->children[i], enclosing_fn);
  }

  // Check if a template function is called from any non-template function
  // without explicit template args (implicit deduction / plain call).
  static bool is_referenced_without_template_args(
      const char* fn_name, const std::vector<const Node*>& items) {
    if (!fn_name) return false;
    for (const Node* item : items) {
      if (item->kind != NK_FUNCTION || !item->body) continue;
      if (item->n_template_params > 0) continue;  // Skip template function bodies
      if (has_plain_call(item->body, fn_name)) return true;
    }
    return false;
  }

  static bool has_plain_call(const Node* n, const char* fn_name) {
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

  Module* module_ = nullptr;
  std::unordered_map<std::string, long long> enum_consts_;
  std::unordered_map<std::string, long long> const_int_bindings_;
  std::unordered_set<std::string> weak_symbols_;
  // Engine-owned compile-time state.  Shared with the pipeline so
  // the compile-time engine can access the registry directly.
  std::shared_ptr<CompileTimeState> ct_state_ = std::make_shared<CompileTimeState>();
  // Convenience alias — shorthand for ct_state_->registry.
  InstantiationRegistry& registry_ = ct_state_->registry;
  // Template struct definitions indexed by struct tag name.
  std::unordered_map<std::string, const Node*> template_struct_defs_;
  // Already-instantiated template struct mangled names (avoid double instantiation).
  std::unordered_set<std::string> instantiated_tpl_structs_;
  // Struct method map: "struct_tag::method_name" → mangled function name.
  std::unordered_map<std::string, std::string> struct_methods_;
  // Struct method return types: "struct_tag::method_name" → return TypeSpec.
  std::unordered_map<std::string, TypeSpec> struct_method_ret_types_;
  // Pending struct methods to lower.
  struct PendingMethod {
    std::string mangled;
    std::string struct_tag;
    const Node* method_node;
    TypeBindings tpl_bindings;
    NttpBindings nttp_bindings;
  };
  std::vector<PendingMethod> pending_methods_;
  // Deduced template call info: call_node → mangled name + bindings.
  struct DeducedTemplateCall {
    std::string mangled_name;
    TypeBindings bindings;
    NttpBindings nttp_bindings;
  };
  std::unordered_map<const Node*, DeducedTemplateCall> deduced_template_calls_;
  // Constructor overloads per struct tag: tag → list of {mangled, method_node}.
  struct CtorOverload {
    std::string mangled_name;
    const Node* method_node;  // for parameter type matching
  };
  std::unordered_map<std::string, std::vector<CtorOverload>> struct_constructors_;
  // Destructor per struct tag: tag → {mangled, method_node}.
  struct DtorInfo {
    std::string mangled_name;
    const Node* method_node;
  };
  std::unordered_map<std::string, DtorInfo> struct_destructors_;
  // Ref-overload resolution: base function name → list of overload entries.
  struct RefOverloadEntry {
    std::string mangled_name;
    std::vector<bool> param_is_rvalue_ref;
    std::vector<bool> param_is_lvalue_ref;
  };
  std::unordered_map<std::string, std::vector<RefOverloadEntry>> ref_overload_set_;
  // Reverse mapping: AST Node* of overloaded function → mangled name.
  std::unordered_map<const Node*, std::string> ref_overload_mangled_;
  // Marks lowering performed on behalf of the compile-time engine so pending
  // consteval nodes can preserve deferred-instantiation provenance.
  bool lowering_deferred_instantiation_ = false;

};


InitialHirBuildResult build_initial_hir(
    const Node* program_root,
    const sema::ResolvedTypeTable* resolved_types) {
  auto lowerer = std::make_shared<Lowerer>();
  lowerer->resolved_types_ = resolved_types;

  auto module = std::make_shared<Module>();
  lowerer->lower_initial_program(program_root, *module);

  InitialHirBuildResult result{};
  result.module = module;
  result.ct_state = lowerer->ct_state();
  result.deferred_instantiate =
      [lowerer, module](const std::string& tpl_name,
                        const TypeBindings& bindings,
                        const NttpBindings& nttp_bindings,
                        const std::string& mangled) -> bool {
    (void)module;
    return lowerer->instantiate_deferred_template(
        tpl_name, bindings, nttp_bindings, mangled);
  };
  return result;
}

}  // namespace c4c::hir
