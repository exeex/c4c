#include "ast_to_hir.hpp"
#include "compile_time_pass.hpp"
#include "consteval.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <functional>
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

  Module lower_program(const Node* root) {
    if (!root || root->kind != NK_PROGRAM) {
      throw std::runtime_error("ast_to_hir: root is not NK_PROGRAM");
    }

    Module m{};
    module_ = &m;

    // Flatten top-level items (may be wrapped in NK_BLOCK)
    std::vector<const Node*> items;
    for (int i = 0; i < root->n_children; ++i) {
      const Node* item = root->children[i];
      if (!item) continue;
      if (item->kind == NK_BLOCK) {
        for (int j = 0; j < item->n_children; ++j)
          if (item->children[j]) items.push_back(item->children[j]);
      } else {
        items.push_back(item);
      }
    }

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
      if (item->kind == NK_FUNCTION && item->is_consteval && item->name)
        consteval_fns_[item->name] = item;
    }

    // Phase 1.6: collect template instantiation info from call sites.
    // For each template function, collect all unique sets of concrete template
    // arg types, so template functions that contain consteval calls with
    // template-dependent args can be lowered once per instantiation.
    for (const Node* item : items) {
      if (item->kind == NK_FUNCTION && item->name && item->n_template_params > 0)
        template_fn_defs_[item->name] = item;
    }
    // Collect explicit template specializations (template<> T foo<Args>(...)).
    for (const Node* item : items) {
      if (item->kind == NK_FUNCTION && item->name && item->is_explicit_specialization
          && item->n_template_args > 0) {
        auto def_it = template_fn_defs_.find(item->name);
        if (def_it != template_fn_defs_.end()) {
          std::string mangled = mangle_specialization(item, def_it->second);
          if (!mangled.empty())
            template_fn_specs_[mangled] = item;
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
    // Also collect consteval template calls from template function bodies,
    // since consteval is still evaluated eagerly during lowering and needs
    // all instantiation bindings to be available at lowering time.
    // This uses the fixpoint loop because consteval templates may chain.
    for (int pass = 0; pass < 8; ++pass) {
      size_t prev_size = 0;
      for (const auto& [k, v] : template_fn_instances_) prev_size += v.size();
      for (const Node* item : items) {
        if (item->kind == NK_FUNCTION && item->body && item->n_template_params > 0)
          collect_consteval_template_instantiations(item->body, item);
      }
      size_t cur_size = 0;
      for (const auto& [k, v] : template_fn_instances_) cur_size += v.size();
      if (cur_size == prev_size) break;
    }

    // Phase 1.7: populate HirTemplateDef metadata for all template functions.
    for (const auto& [name, fn_def] : template_fn_defs_) {
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
      auto inst_it = template_fn_instances_.find(name);
      if (inst_it != template_fn_instances_.end()) {
        for (const auto& inst : inst_it->second) {
          HirTemplateInstantiation hi;
          hi.mangled_name = inst.mangled_name;
          hi.bindings = inst.bindings;
          hi.nttp_bindings = inst.nttp_bindings;
          hi.spec_key = inst.nttp_bindings.empty()
              ? make_specialization_key(name, tdef.template_params, inst.bindings)
              : make_specialization_key(name, tdef.template_params, inst.bindings, inst.nttp_bindings);
          tdef.instances.push_back(std::move(hi));
        }
      }
      m.template_defs[name] = std::move(tdef);
    }

    // Phase 2: lower functions and globals
    for (const Node* item : items) {
      if (item->kind == NK_FUNCTION) {
        if (item->is_consteval && item->n_template_params == 0) {
          // Non-template consteval function: add as declaration-only to HIR
          // for analysis/debugging.  The body is not lowered (it's evaluated
          // by the AST-level consteval interpreter).  Marked consteval_only
          // so the materialization pass won't emit it.
          Function ce_fn{};
          ce_fn.id = next_fn_id();
          ce_fn.name = item->name ? item->name : "<anon_consteval>";
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
          auto inst_it = template_fn_instances_.find(item->name);
          if (inst_it != template_fn_instances_.end() && !inst_it->second.empty()) {
            for (const auto& inst : inst_it->second) {
              // Check for explicit specialization matching this instantiation.
              auto spec_it = template_fn_specs_.find(inst.mangled_name);
              if (spec_it != template_fn_specs_.end()) {
                // Use the specialization body (no template bindings needed).
                lower_function(spec_it->second, &inst.mangled_name);
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
          lower_function(item);
        }
      } else if (item->kind == NK_GLOBAL_VAR) {
        lower_global(item);
      }
    }

    // Phase 3: run HIR compile-time reduction with deferred instantiation.
    // Nested template calls (e.g., add<T>() inside twice<T>()) were not
    // collected in Phase 1.6 and thus not lowered in Phase 2.  The
    // compile-time reduction pass discovers these unresolved calls and
    // triggers on-demand lowering via the callback below.
    auto deferred_lower = [this](const std::string& tpl_name,
                                 const TypeBindings& bindings,
                                 const NttpBindings& nttp_bindings,
                                 const std::string& mangled) -> bool {
      auto fn_it = template_fn_defs_.find(tpl_name);
      if (fn_it == template_fn_defs_.end()) return false;
      const Node* fn_def = fn_it->second;
      if (fn_def->is_consteval) return false;  // consteval handled eagerly
      // Check for explicit specialization matching this instantiation.
      auto spec_it = template_fn_specs_.find(mangled);
      if (spec_it != template_fn_specs_.end()) {
        lower_function(spec_it->second, &mangled);
      } else {
        // Enable deferred consteval: consteval calls inside this function
        // will create PendingConstevalExpr nodes instead of evaluating.
        defer_consteval_ = true;
        const NttpBindings* nttp_ptr = nttp_bindings.empty() ? nullptr : &nttp_bindings;
        lower_function(fn_def, &mangled, &bindings, nttp_ptr);
        defer_consteval_ = false;
      }
      // Track template origin and specialization key for deferred instantiations.
      if (!module_->functions.empty()) {
        module_->functions.back().template_origin = tpl_name;
        auto param_order = get_template_param_order_from_instances(tpl_name);
        module_->functions.back().spec_key = nttp_bindings.empty()
            ? make_specialization_key(tpl_name, param_order, bindings)
            : make_specialization_key(tpl_name, param_order, bindings, nttp_bindings);
      }
      return true;
    };
    // Consteval evaluation callback for the compile-time pass.
    // Evaluates PendingConstevalExpr nodes using the AST-level interpreter.
    auto deferred_consteval = [this](const std::string& fn_name,
                                     const std::vector<long long>& const_args,
                                     const TypeBindings& bindings,
                                     const NttpBindings& nttp_binds)
        -> std::optional<long long> {
      auto ce_it = consteval_fns_.find(fn_name);
      if (ce_it == consteval_fns_.end()) return std::nullopt;
      // Build ConstValue args and evaluation environment.
      std::vector<ConstValue> args;
      args.reserve(const_args.size());
      for (long long v : const_args) args.push_back(ConstValue::make_int(v));
      ConstEvalEnv env{&enum_consts_, &const_int_bindings_, nullptr};
      TypeBindings tpl_bindings = bindings;
      env.type_bindings = &tpl_bindings;
      NttpBindings nttp_copy = nttp_binds;
      if (!nttp_copy.empty())
        env.nttp_bindings = &nttp_copy;
      auto result = evaluate_consteval_call(
          ce_it->second, args, env, consteval_fns_);
      if (result.ok()) return result.as_int();
      return std::nullopt;
    };
    auto ct_stats = run_compile_time_reduction(m, deferred_lower, deferred_consteval);
    m.ct_info.deferred_instantiations = ct_stats.templates_deferred;
    m.ct_info.deferred_consteval = ct_stats.consteval_deferred;
    m.ct_info.total_iterations = ct_stats.iterations;
    m.ct_info.templates_resolved = ct_stats.templates_instantiated;
    m.ct_info.consteval_reduced = ct_stats.consteval_reduced;
    m.ct_info.converged = ct_stats.converged;

    return m;
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
  };

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
            if (tt != ctx->local_types.end()) return tt->second;
          }
          auto pit = ctx->params.find(name);
          if (pit != ctx->params.end() && ctx->fn &&
              pit->second < ctx->fn->params.size())
            return ctx->fn->params[pit->second].type.spec;
          auto sit = ctx->static_globals.find(name);
          if (sit != ctx->static_globals.end()) {
            if (const GlobalVar* gv = module_->find_global(sit->second)) return gv->type.spec;
          }
        }
        auto git = module_->global_index.find(name);
        if (git != module_->global_index.end()) {
          if (const GlobalVar* gv = module_->find_global(git->second)) return gv->type.spec;
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
    if (sd->n_template_params > 0) return;
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
  }

  void collect_enum_def(const Node* ed) {
    if (!ed || ed->kind != NK_ENUM_DEF || ed->n_enum_variants <= 0) return;
    if (!ed->enum_names || !ed->enum_vals) return;
    for (int i = 0; i < ed->n_enum_variants; ++i) {
      const char* name = ed->enum_names[i];
      if (!name || !name[0]) continue;
      enum_consts_[name] = ed->enum_vals[i];
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
        case TB_CHAR: case TB_SCHAR: mangled += "char"; break;
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
        case TB_STRUCT: case TB_UNION:
          mangled += pts.tag ? pts.tag : "anon";
          break;
        default: mangled += "T"; break;
      }
      for (int p = 0; p < pts.ptr_level; ++p) mangled += "p";
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
    // Substitute template type parameters in the return type.
    {
      TypeSpec ret_ts = fn_node->type;
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
    if (fn_node->type.is_fn_ptr) {
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
          fn.ret_fn_ptr_sig = std::move(ret_sig);
        }
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
        param.type = qtype_from(param_ts, ValueCategory::LValue);
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
      module_->functions.push_back(Function{fn.id, fn.name, fn.return_type});
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
        } else if (const auto* ch = std::get_if<CharLiteral>(&e.payload)) {
          const_int_bindings_[g.name] = ch->value;
        }
      }
    }

    module_->global_index[g.name] = g.id;
    module_->globals.push_back(std::move(g));
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
        for (int i = 0; i < n->n_children; ++i) {
          lower_stmt_node(ctx, n->children[i]);
        }
        if (new_scope) {
          ctx.locals = saved_locals;
          ctx.static_globals = saved_static_globals;
          enum_consts_ = saved_enum_consts;
          ctx.local_const_bindings = saved_local_consts;
        }
        return;
      }
      case NK_DECL: {
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
          d.type = qtype_from(decl_ts, ValueCategory::LValue);
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
        if (!is_array_with_init_list && !is_array_with_string_init &&
            !is_struct_with_init_list && n->init)
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
            n->type.ptr_level == 0 && n->type.array_rank == 0) {
          ConstEvalEnv cenv{&enum_consts_, &const_int_bindings_, &ctx.local_const_bindings};
          if (auto cr = evaluate_constant_expr(n->init, cenv); cr.ok()) {
            ctx.local_const_bindings[n->name] = cr.as_int();
          }
        }
        append_stmt(ctx, Stmt{StmtPayload{std::move(d)}, make_span(n)});
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
                // Parser stores compound literal initializers in `left` as NK_INIT_LIST.
                // Unwrap the first scalar so array-of-aggregate local init doesn't
                // route through generic aggregate assignment (which expects a true
                // aggregate rvalue expression).
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

            // Value expr
            ExprId val_id = lower_expr(&ctx, val_node);
            // AssignExpr
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
        // For struct init lists, emit recursive field-by-field assignments with
        // brace-elision consumption, so nested aggregates don't become invalid
        // scalar-to-aggregate stores (e.g. `store %struct.S 5`).
        if (((is_struct_with_init_list && decl_ts.tag) ||
             (is_array_with_init_list && !use_array_init_fast_path)) &&
            n->init) {
          auto is_agg = [](const TypeSpec& ts) {
            return (ts.base == TB_STRUCT || ts.base == TB_UNION) &&
                   ts.ptr_level == 0 && ts.array_rank == 0;
          };
          auto append_assign = [&](ExprId lhs_id, const TypeSpec& lhs_ts, const Node* rhs_node) {
            if (!rhs_node) return;
            // char[N] = "..." inside aggregate init: emit element-wise stores
            // instead of assigning the string pointer value.
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
                      } else if (can_direct_assign_agg(field_ts, val_node)) {
                        append_assign(me_id, field_ts, val_node);
                        ++cursor;
                      } else if (val_node && has_field_designator) {
                        // Designated init with explicit non-list value — direct assign
                        // even when can_direct_assign_agg fails (e.g. empty struct from
                        // function call whose return type was not inferred).
                        append_assign(me_id, field_ts, val_node);
                        ++cursor;
                      } else {
                        // Brace elision: nested aggregate consumes from parent list.
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

                if (is_vector_ty(cur_ts)) {
                  TypeSpec elem_ts = vector_element_type(cur_ts);
                  long long next_idx = 0;
                  const long long bound = cur_ts.vector_lanes > 0 ? cur_ts.vector_lanes : 0;
                  while (cursor < list_node->n_children && next_idx < bound) {
                    const Node* item = list_node->children[cursor];
                    if (!item) { ++cursor; ++next_idx; continue; }
                    TypeSpec idx_ts{}; idx_ts.base = TB_INT;
                    ExprId idx_id = append_expr(n, IntLiteral{next_idx, false}, idx_ts);
                    IndexExpr ie{}; ie.base = cur_lhs; ie.index = idx_id;
                    ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
                    const Node* val_node = init_item_value_node(item);
                    append_assign(ie_id, elem_ts, val_node);
                    ++cursor;
                    ++next_idx;
                  }
                  return;
                }

                if (cur_ts.array_rank > 0) {
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
                  // Shift array_dims to drop the outermost dimension
                  for (int di = 0; di < elem_ts.array_rank - 1; ++di)
                    elem_ts.array_dims[di] = elem_ts.array_dims[di + 1];
                  elem_ts.array_dims[elem_ts.array_rank - 1] = -1;
                  elem_ts.array_rank--;
                  elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
                  long long next_idx = 0;
                  const long long bound = cur_ts.array_size > 0 ? cur_ts.array_size : (1LL << 30);
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
        if (n->left) s.expr = lower_expr(&ctx, n->left);
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
        s.cond = lower_expr(&ctx, n->cond ? n->cond : n->left);
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
        s.cond = lower_expr(&ctx, n->cond ? n->cond : n->left);
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
        if (n->cond) s.cond = lower_expr(&ctx, n->cond);
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
        s.cond = lower_expr(&ctx, n->cond ? n->cond : n->left);
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
        return append_expr(n, std::move(r), var_ts, ValueCategory::LValue);
      }
      case NK_UNARY: {
        UnaryExpr u{};
        u.op = map_unary_op(n->op);
        u.operand = lower_expr(ctx, n->left);
        return append_expr(n, u, n->type);
      }
      case NK_POSTFIX: {
        UnaryExpr u{};
        const std::string op = n->op ? n->op : "";
        u.op = (op == "--") ? UnaryOp::PostDec : UnaryOp::PostInc;
        u.operand = lower_expr(ctx, n->left);
        return append_expr(n, u, n->type);
      }
      case NK_ADDR: {
        // Reject taking the address of a consteval function.
        if (n->left && n->left->kind == NK_VAR && n->left->name &&
            consteval_fns_.count(n->left->name)) {
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
        BinaryExpr b{};
        b.op = map_binary_op(n->op);
        b.lhs = lower_expr(ctx, n->left);
        b.rhs = lower_expr(ctx, n->right);
        return append_expr(n, b, n->type);
      }
      case NK_ASSIGN:
      case NK_COMPOUND_ASSIGN: {
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
        // Try consteval interpretation for calls to consteval functions.
        if (n->kind == NK_CALL && n->left && n->left->kind == NK_VAR && n->left->name) {
          auto ce_it = consteval_fns_.find(n->left->name);
          if (ce_it != consteval_fns_.end()) {
            // Evaluate all arguments as constant expressions.
            ConstEvalEnv arg_env{&enum_consts_, &const_int_bindings_,
                                 ctx ? &ctx->local_const_bindings : nullptr};
            // Build template type substitution map from call-site template args
            // and the consteval function's template parameter names.
            // When call-site template args are unresolved typedefs (template params
            // of the enclosing function), resolve them through the enclosing
            // function's template bindings.
            TypeBindings tpl_bindings;
            NttpBindings ce_nttp_bindings;
            const Node* fn_def = ce_it->second;
            if ((n->left->n_template_args > 0 || n->left->has_template_args) &&
                fn_def->n_template_params > 0) {
              int count = std::min(n->left->n_template_args, fn_def->n_template_params);
              for (int i = 0; i < count; ++i) {
                if (!fn_def->template_param_names[i]) continue;
                // NTTP argument: store as constant binding, not type binding.
                if (fn_def->template_param_is_nttp &&
                    fn_def->template_param_is_nttp[i]) {
                  if (n->left->template_arg_is_value &&
                      n->left->template_arg_is_value[i]) {
                    // Check for forwarded NTTP name that needs resolution.
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
                // Resolve through enclosing template function's bindings.
                if (arg_ts.base == TB_TYPEDEF && arg_ts.tag && ctx &&
                    !ctx->tpl_bindings.empty()) {
                  auto resolved = ctx->tpl_bindings.find(arg_ts.tag);
                  if (resolved != ctx->tpl_bindings.end()) arg_ts = resolved->second;
                }
                tpl_bindings[fn_def->template_param_names[i]] = arg_ts;
              }
              // Fill remaining params from defaults.
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
            if (all_const) {
              // When defer_consteval_ is set (deferred template instantiation),
              // create a PendingConstevalExpr instead of evaluating eagerly.
              // The compile-time pass will evaluate it later.
              if (defer_consteval_) {
                PendingConstevalExpr pce;
                pce.fn_name = n->left->name;
                for (const auto& cv : args)
                  pce.const_args.push_back(cv.as_int());
                pce.tpl_bindings = tpl_bindings;
                pce.nttp_bindings = ce_nttp_bindings;
                pce.call_span = make_span(n);
                TypeSpec ts = n->type;
                return append_expr(n, std::move(pce), ts);
              }
              auto result = evaluate_consteval_call(
                  ce_it->second, args, arg_env, consteval_fns_);
              if (result.ok()) {
                TypeSpec ts = n->type;
                long long rv = result.as_int();
                ExprId eid = append_expr(n, IntLiteral{rv, false}, ts);
                // Record consteval call metadata for HIR inspection.
                ConstevalCallInfo info;
                info.fn_name = n->left->name;
                for (const auto& cv : args)
                  info.const_args.push_back(cv.as_int());
                info.template_bindings = tpl_bindings;
                info.result_value = rv;
                info.result_expr = eid;
                info.span = make_span(n);
                module_->consteval_calls.push_back(std::move(info));
                return eid;
              }
              // Consteval call failed — hard error, no runtime fallback.
              std::string diag = "error: call to consteval function '";
              diag += n->left->name;
              diag += "' could not be evaluated at compile time";
              if (!result.error.empty()) {
                diag += "\n  note: ";
                diag += result.error;
              }
              throw std::runtime_error(diag);
            } else {
              // Non-constant arguments to consteval function — hard error.
              std::string diag = "error: call to consteval function '";
              diag += n->left->name;
              diag += "' with non-constant arguments";
              throw std::runtime_error(diag);
            }
          }
        }

        CallExpr c{};
        // For template function calls, resolve the mangled instantiation name.
        std::string resolved_callee_name;
        if (n->left && n->left->kind == NK_VAR && n->left->name &&
            (n->left->n_template_args > 0 || n->left->has_template_args) &&
            template_fn_defs_.count(n->left->name) &&
            !consteval_fns_.count(n->left->name)) {
          const TypeBindings* enc = (ctx && !ctx->tpl_bindings.empty())
                                        ? &ctx->tpl_bindings : nullptr;
          const NttpBindings* enc_nttp = (ctx && !ctx->nttp_bindings.empty())
                                              ? &ctx->nttp_bindings : nullptr;
          resolved_callee_name = resolve_template_call_name(n->left, enc, enc_nttp);
          // Create a DeclRef with the mangled name directly.
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
          // Preserve template application metadata.
          TemplateCallInfo tci;
          tci.source_template = n->left->name;
          auto fn_it = template_fn_defs_.find(n->left->name);
          if (fn_it != template_fn_defs_.end()) {
            TypeBindings bindings = build_call_bindings(n->left, fn_it->second, enc);
            for (const auto& pname : get_template_param_order(fn_it->second)) {
              auto bit = bindings.find(pname);
              if (bit != bindings.end()) tci.template_args.push_back(bit->second);
            }
            tci.nttp_args = build_call_nttp_bindings(n->left, fn_it->second, enc_nttp);
          }
          c.template_info = std::move(tci);
        } else {
          c.callee = lower_expr(ctx, n->left);
        }
        c.builtin_id = n->builtin_id;
        for (int i = 0; i < n->n_children; ++i) c.args.push_back(lower_expr(ctx, n->children[i]));
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
      case NK_VA_ARG: {
        VaArgExpr v{};
        v.ap = lower_expr(ctx, n->left);
        return append_expr(n, v, n->type);
      }
      case NK_INDEX: {
        IndexExpr idx{};
        idx.base = lower_expr(ctx, n->left);
        idx.index = lower_expr(ctx, n->right);
        return append_expr(n, idx, n->type, ValueCategory::LValue);
      }
      case NK_MEMBER: {
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
          ExprId cond_expr = lower_expr(ctx, n->cond ? n->cond : n->left);

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
        t.cond = lower_expr(ctx, n->cond ? n->cond : n->left);
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
        SizeofTypeExpr s{};
        s.type = qtype_from(sizeof_target);
        // sizeof always returns an integer (size_t ~ unsigned long)
        TypeSpec ts{}; ts.base = TB_ULONG;
        return append_expr(n, s, ts);
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

  struct TemplateInstance {
    TypeBindings bindings;
    NttpBindings nttp_bindings;  // non-type template param → constant value
    std::string mangled_name;
    SpecializationKey spec_key;  // stable identity for dedup/caching
  };

  // Produce a deterministic type suffix for name mangling.
  static std::string type_suffix(const TypeSpec& ts) {
    if (ts.ptr_level > 0) return "p" + std::to_string(ts.ptr_level);
    switch (ts.base) {
      case TB_BOOL: return "b";
      case TB_CHAR: case TB_SCHAR: return "c";
      case TB_UCHAR: return "uc";
      case TB_SHORT: return "s";
      case TB_USHORT: return "us";
      case TB_INT: return "i";
      case TB_UINT: return "ui";
      case TB_LONG: return "l";
      case TB_ULONG: return "ul";
      case TB_LONGLONG: return "ll";
      case TB_ULONGLONG: return "ull";
      case TB_FLOAT: return "f";
      case TB_DOUBLE: return "d";
      case TB_LONGDOUBLE: return "ld";
      case TB_INT128: return "i128";
      case TB_UINT128: return "u128";
      case TB_VOID: return "v";
      default:
        if (ts.tag) return std::string("T") + ts.tag;
        return "unknown";
    }
  }

  // Build a mangled name from base name and template bindings.
  static std::string mangle_template_name(const std::string& base,
                                           const TypeBindings& bindings,
                                           const NttpBindings& nttp_bindings = {}) {
    if (bindings.empty() && nttp_bindings.empty()) return base;
    // Collect all param names for deterministic ordering.
    std::vector<std::string> all_params;
    for (const auto& [k, v] : bindings) all_params.push_back(k);
    for (const auto& [k, v] : nttp_bindings) all_params.push_back(k);
    std::sort(all_params.begin(), all_params.end());
    std::string result = base;
    for (const auto& param : all_params) {
      result += "_";
      auto nttp_it = nttp_bindings.find(param);
      if (nttp_it != nttp_bindings.end()) {
        // NTTP: encode value (use 'n' prefix for negatives).
        if (nttp_it->second < 0)
          result += "n" + std::to_string(-nttp_it->second);
        else
          result += std::to_string(nttp_it->second);
      } else {
        auto it = bindings.find(param);
        if (it != bindings.end()) result += type_suffix(it->second);
      }
    }
    return result;
  }

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

  // Check if an instantiation with the given mangled name already exists (O(1)).
  bool has_instance(const std::string& fn_name, const std::string& mangled) {
    return instance_keys_.count(fn_name + "::" + mangled) > 0;
  }

  // Check if a call node has any forwarded NTTP names (not yet resolved to values).
  static bool has_forwarded_nttp(const Node* call_var) {
    if (!call_var || !call_var->template_arg_nttp_names) return false;
    for (int i = 0; i < call_var->n_template_args; ++i) {
      if (call_var->template_arg_nttp_names[i]) return true;
    }
    return false;
  }

  // Check if all bindings are concrete (no unresolved TB_TYPEDEF).
  static bool bindings_are_concrete(const TypeBindings& bindings) {
    for (const auto& [param, ts] : bindings) {
      if (ts.base == TB_TYPEDEF) return false;
    }
    return true;
  }

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
    auto it = template_fn_defs_.find(fn_name);
    if (it != template_fn_defs_.end()) return get_template_param_order(it->second);
    return {};
  }

  // Record a template instantiation. Returns the mangled name.
  // Only records if all bindings are concrete (no unresolved typedefs).
  std::string record_instance(const std::string& fn_name, TypeBindings bindings,
                               NttpBindings nttp_bindings = {}) {
    if (!bindings_are_concrete(bindings)) return "";  // Skip unresolved.
    std::string mangled = mangle_template_name(fn_name, bindings, nttp_bindings);
    auto param_order = get_template_param_order_from_instances(fn_name);
    SpecializationKey sk = nttp_bindings.empty()
        ? make_specialization_key(fn_name, param_order, bindings)
        : make_specialization_key(fn_name, param_order, bindings, nttp_bindings);
    template_fn_instances_[fn_name].push_back(
        {std::move(bindings), std::move(nttp_bindings), mangled, sk});
    instance_keys_.insert(fn_name + "::" + mangled);
    return mangled;
  }

  // Resolve the mangled name for a call to a template function.
  std::string resolve_template_call_name(const Node* call_var,
                                          const TypeBindings* enclosing_bindings,
                                          const NttpBindings* enclosing_nttp = nullptr) {
    if (!call_var || !call_var->name ||
        (call_var->n_template_args <= 0 && !call_var->has_template_args))
      return call_var ? (call_var->name ? call_var->name : "") : "";
    auto fn_it = template_fn_defs_.find(call_var->name);
    if (fn_it == template_fn_defs_.end()) return call_var->name;
    const Node* fn_def = fn_it->second;
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
      auto fn_it = template_fn_defs_.find(n->left->name);
      if (fn_it != template_fn_defs_.end()) {
        const Node* fn_def = fn_it->second;
        if (!fn_def->is_consteval && fn_def->n_template_params > 0) {
          // Determine the enclosing function's template bindings (if any).
          const TypeBindings* enclosing_bindings = nullptr;
          TypeBindings enc_bindings;
          if (enclosing_fn && enclosing_fn->name) {
            auto enc_it = template_fn_instances_.find(enclosing_fn->name);
            if (enc_it != template_fn_instances_.end()) {
              // For each enclosing instantiation, record an inner instantiation.
              for (const auto& enc_inst : enc_it->second) {
                TypeBindings inner = build_call_bindings(n->left, fn_def, &enc_inst.bindings);
                NttpBindings call_nttp = build_call_nttp_bindings(n->left, fn_def, &enc_inst.nttp_bindings);
                std::string mangled = mangle_template_name(n->left->name, inner, call_nttp);
                if (!has_instance(n->left->name, mangled))
                  record_instance(n->left->name, std::move(inner), call_nttp);
              }
              goto recurse;  // Already handled all enclosing instantiations.
            }
          }
          {
            NttpBindings call_nttp = build_call_nttp_bindings(n->left, fn_def);
            if (has_forwarded_nttp(n->left)) goto recurse;  // Deferred: forwarded NTTPs not yet resolved.
            TypeBindings bindings = build_call_bindings(n->left, fn_def, nullptr);
            std::string mangled = mangle_template_name(n->left->name, bindings, call_nttp);
            if (!has_instance(n->left->name, mangled))
              record_instance(n->left->name, std::move(bindings), call_nttp);
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
      auto fn_it = template_fn_defs_.find(n->left->name);
      if (fn_it != template_fn_defs_.end()) {
        const Node* fn_def = fn_it->second;
        if (fn_def->is_consteval && fn_def->n_template_params > 0) {
          if (enclosing_fn && enclosing_fn->name) {
            auto enc_it = template_fn_instances_.find(enclosing_fn->name);
            if (enc_it != template_fn_instances_.end()) {
              for (const auto& enc_inst : enc_it->second) {
                TypeBindings inner = build_call_bindings(n->left, fn_def, &enc_inst.bindings);
                NttpBindings call_nttp = build_call_nttp_bindings(n->left, fn_def, &enc_inst.nttp_bindings);
                std::string mangled = mangle_template_name(n->left->name, inner, call_nttp);
                if (!has_instance(n->left->name, mangled))
                  record_instance(n->left->name, std::move(inner), call_nttp);
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
            if (!has_instance(n->left->name, mangled))
              record_instance(n->left->name, std::move(bindings), call_nttp);
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
  std::unordered_map<std::string, const Node*> consteval_fns_;
  // All template function definitions indexed by name (consteval and non-consteval).
  std::unordered_map<std::string, const Node*> template_fn_defs_;
  // Template function name → all unique instantiations.
  std::unordered_map<std::string, std::vector<TemplateInstance>> template_fn_instances_;
  // O(1) dedup set: "fn_name::mangled_name" for each recorded instance.
  std::unordered_set<std::string> instance_keys_;
  // Explicit template specializations: mangled_name → specialization AST node.
  std::unordered_map<std::string, const Node*> template_fn_specs_;
  // Template struct definitions indexed by struct tag name.
  std::unordered_map<std::string, const Node*> template_struct_defs_;
  // Already-instantiated template struct mangled names (avoid double instantiation).
  std::unordered_set<std::string> instantiated_tpl_structs_;
  // When true, consteval calls create PendingConstevalExpr instead of
  // evaluating eagerly.  Set during deferred template instantiation.
  bool defer_consteval_ = false;

};


Module lower_ast_to_hir(const Node* program_root,
                        const sema::ResolvedTypeTable* resolved_types) {
  Lowerer l;
  l.resolved_types_ = resolved_types;
  return l.lower_program(program_root);
}

std::string format_summary(const Module& module) {
  const ProgramSummary s = module.summary();
  char buf[256];
  std::snprintf(
      buf,
      sizeof(buf),
      "HIR summary: functions=%zu globals=%zu blocks=%zu statements=%zu expressions=%zu",
      s.functions,
      s.globals,
      s.blocks,
      s.statements,
      s.expressions);
  return std::string(buf);
}

}  // namespace c4c::hir
