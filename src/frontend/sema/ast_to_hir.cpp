#include "ast_to_hir.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir {
namespace {

using namespace tinyc2ll::frontend_cxx::sema_ir;

SourceSpan make_span(const Node* n) {
  if (!n) {
    return {};
  }
  SourceSpan s{};
  s.begin.line = n->line;
  s.end.line = n->line;
  return s;
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
  if (ts.array_rank > 0 && !ts.is_ptr_to_array) {
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
  if (ts.ptr_level == 0 && ts.array_rank == 0) {
    ts.is_const = false;
    ts.is_volatile = false;
  }
  return ts;
}

bool generic_type_compatible(TypeSpec a, TypeSpec b) {
  a = normalize_generic_type(a);
  b = normalize_generic_type(b);
  if (a.base != b.base) return false;
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
  return ts.base != TB_VOID || ts.ptr_level > 0 || ts.array_rank > 0 || ts.is_fn_ptr;
}

std::vector<long long> decode_string_literal_values(const char* sval, bool wide) {
  std::vector<long long> out;
  if (!sval) {
    out.push_back(0);
    return out;
  }
  const char* p = sval;
  while (*p && *p != '"') ++p;
  if (*p == '"') ++p;
  while (*p && *p != '"') {
    if (*p == '\\' && *(p + 1)) {
      ++p;
      switch (*p) {
        case 'n': out.push_back('\n'); ++p; break;
        case 't': out.push_back('\t'); ++p; break;
        case 'r': out.push_back('\r'); ++p; break;
        case '0': out.push_back(0); ++p; break;
        case '\\': out.push_back('\\'); ++p; break;
        case '\'': out.push_back('\''); ++p; break;
        case '"': out.push_back('"'); ++p; break;
        default: out.push_back(static_cast<unsigned char>(*p)); ++p; break;
      }
      continue;
    }

    const unsigned char c0 = static_cast<unsigned char>(*p);
    if (!wide || c0 < 0x80) {
      out.push_back(static_cast<long long>(c0));
      ++p;
      continue;
    }
    if ((c0 & 0xE0) == 0xC0 && p[1]) {
      const uint32_t cp = ((c0 & 0x1F) << 6) | (static_cast<unsigned char>(p[1]) & 0x3F);
      out.push_back(static_cast<long long>(cp));
      p += 2;
      continue;
    }
    if ((c0 & 0xF0) == 0xE0 && p[1] && p[2]) {
      const uint32_t cp = ((c0 & 0x0F) << 12) |
                          ((static_cast<unsigned char>(p[1]) & 0x3F) << 6) |
                          (static_cast<unsigned char>(p[2]) & 0x3F);
      out.push_back(static_cast<long long>(cp));
      p += 3;
      continue;
    }
    if ((c0 & 0xF8) == 0xF0 && p[1] && p[2] && p[3]) {
      const uint32_t cp = ((c0 & 0x07) << 18) |
                          ((static_cast<unsigned char>(p[1]) & 0x3F) << 12) |
                          ((static_cast<unsigned char>(p[2]) & 0x3F) << 6) |
                          (static_cast<unsigned char>(p[3]) & 0x3F);
      out.push_back(static_cast<long long>(cp));
      p += 4;
      continue;
    }
    out.push_back(static_cast<long long>(c0));
    ++p;
  }
  out.push_back(0);
  return out;
}

std::optional<long long> eval_int_const_expr(
    const Node* n,
    const std::unordered_map<std::string, long long>& enum_consts) {
  if (!n) return std::nullopt;
  switch (n->kind) {
    case NK_INT_LIT:
    case NK_CHAR_LIT:
      return n->ival;
    case NK_VAR: {
      if (n->name && n->name[0]) {
        auto it = enum_consts.find(n->name);
        if (it != enum_consts.end()) return it->second;
      }
      return std::nullopt;
    }
    case NK_CAST:
      return eval_int_const_expr(n->left, enum_consts);
    case NK_UNARY: {
      auto v = eval_int_const_expr(n->left, enum_consts);
      if (!v || !n->op) return std::nullopt;
      if (std::strcmp(n->op, "+") == 0) return *v;
      if (std::strcmp(n->op, "-") == 0) return -*v;
      if (std::strcmp(n->op, "~") == 0) return ~*v;
      if (std::strcmp(n->op, "!") == 0) return static_cast<long long>(!*v);
      return std::nullopt;
    }
    case NK_BINOP: {
      auto l = eval_int_const_expr(n->left, enum_consts);
      auto r = eval_int_const_expr(n->right, enum_consts);
      if (!l || !r || !n->op) return std::nullopt;
      if (std::strcmp(n->op, "+") == 0) return *l + *r;
      if (std::strcmp(n->op, "-") == 0) return *l - *r;
      if (std::strcmp(n->op, "*") == 0) return *l * *r;
      if (std::strcmp(n->op, "/") == 0) return (*r == 0) ? 0LL : (*l / *r);
      if (std::strcmp(n->op, "%") == 0) return (*r == 0) ? 0LL : (*l % *r);
      if (std::strcmp(n->op, "<<") == 0) return *l << *r;
      if (std::strcmp(n->op, ">>") == 0) return *l >> *r;
      if (std::strcmp(n->op, "&") == 0) return *l & *r;
      if (std::strcmp(n->op, "|") == 0) return *l | *r;
      if (std::strcmp(n->op, "^") == 0) return *l ^ *r;
      if (std::strcmp(n->op, "<") == 0) return static_cast<long long>(*l < *r);
      if (std::strcmp(n->op, "<=") == 0) return static_cast<long long>(*l <= *r);
      if (std::strcmp(n->op, ">") == 0) return static_cast<long long>(*l > *r);
      if (std::strcmp(n->op, ">=") == 0) return static_cast<long long>(*l >= *r);
      if (std::strcmp(n->op, "==") == 0) return static_cast<long long>(*l == *r);
      if (std::strcmp(n->op, "!=") == 0) return static_cast<long long>(*l != *r);
      if (std::strcmp(n->op, "&&") == 0) return static_cast<long long>(*l && *r);
      if (std::strcmp(n->op, "||") == 0) return static_cast<long long>(*l || *r);
      return std::nullopt;
    }
    case NK_TERNARY: {
      auto c = eval_int_const_expr(n->cond ? n->cond : n->left, enum_consts);
      if (!c) return std::nullopt;
      return eval_int_const_expr(*c ? n->then_ : n->else_, enum_consts);
    }
    default:
      return std::nullopt;
  }
}

class Lowerer {
 public:
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

    // Phase 1: collect type definitions used by later lowering
    for (const Node* item : items) {
      if (item->kind == NK_STRUCT_DEF) lower_struct_def(item);
      if (item->kind == NK_ENUM_DEF) collect_enum_def(item);
    }

    // Phase 2: lower functions and globals
    for (const Node* item : items) {
      if (item->kind == NK_FUNCTION) {
        lower_function(item);
      } else if (item->kind == NK_GLOBAL_VAR) {
        lower_global(item);
      }
    }

    return m;
  }

 private:
  struct SwitchCtx {
    BlockId parent_block{};
    std::vector<std::pair<long long, BlockId>> cases;
    std::optional<BlockId> default_block;
  };

  struct FunctionCtx {
    Function* fn = nullptr;
    std::unordered_map<std::string, LocalId> locals;
    std::unordered_map<uint32_t, TypeSpec> local_types;
    std::unordered_map<std::string, GlobalId> static_globals;
    std::unordered_map<std::string, uint32_t> params;
    BlockId current_block{};
    std::vector<BlockId> break_stack;
    std::vector<BlockId> continue_stack;
    std::unordered_map<std::string, BlockId> label_blocks;
    std::vector<SwitchCtx> switch_stack;
  };

  FunctionId next_fn_id() { return module_->alloc_function_id(); }
  GlobalId next_global_id() { return module_->alloc_global_id(); }
  LocalId next_local_id() { return module_->alloc_local_id(); }
  BlockId next_block_id() { return module_->alloc_block_id(); }
  ExprId next_expr_id() { return module_->alloc_expr_id(); }

  QualType qtype_from(const TypeSpec& t, ValueCategory c = ValueCategory::RValue) {
    QualType qt{};
    qt.spec = t;
    qt.category = c;
    return qt;
  }

  TypeSpec infer_generic_ctrl_type(FunctionCtx* ctx, const Node* n) {
    if (!n) return {};
    if (has_concrete_type(n->type)) return n->type;
    switch (n->kind) {
      case NK_INT_LIT:
        return infer_int_literal_type(n);
      case NK_FLOAT_LIT: {
        TypeSpec ts = n->type;
        if (!has_concrete_type(ts)) ts.base = TB_DOUBLE;
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
        if (ts.array_rank > 0) {
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
        else if (ts.array_rank > 0) {
          ts.array_rank -= 1;
          ts.array_size = (ts.array_rank > 0) ? ts.array_dims[0] : -1;
        }
        return ts;
      }
      case NK_CAST:
      case NK_COMPOUND_LIT:
        return n->type;
      case NK_BINOP: {
        const TypeSpec l = infer_generic_ctrl_type(ctx, n->left);
        const TypeSpec r = infer_generic_ctrl_type(ctx, n->right);
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

    int llvm_idx = 0;
    for (int i = 0; i < num_fields; ++i) {
      const Node* f = get_field(i);
      if (!f || !f->name) continue;

      HirStructField hf;
      hf.name = f->name;
      TypeSpec ft = f->type;
      // Extract first array dimension (keep base element type for LLVM)
      if (ft.array_rank > 0) {
        // Flexible array member (last field with unknown bound) is represented
        // as zero-length in LLVM layout/sizeof calculations.
        hf.array_first_dim = (ft.array_size >= 0) ? ft.array_size : 0;
        ft.array_rank = 0;
        ft.array_size = -1;
      }
      hf.elem_type = ft;
      hf.is_anon_member = f->is_anon_field;
      hf.llvm_idx = sd->is_union ? 0 : llvm_idx;
      def.fields.push_back(std::move(hf));
      if (!sd->is_union) ++llvm_idx;
    }

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

  void lower_function(const Node* fn_node) {
    Function fn{};
    fn.id = next_fn_id();
    fn.name = fn_node->name ? fn_node->name : "<anon_fn>";
    fn.return_type = qtype_from(fn_node->type);
    fn.linkage = {fn_node->is_static, fn_node->is_extern || fn_node->body == nullptr, fn_node->is_inline};
    fn.attrs.variadic = fn_node->variadic;
    fn.span = make_span(fn_node);

    FunctionCtx ctx{};
    ctx.fn = &fn;

    for (int i = 0; i < fn_node->n_params; ++i) {
      const Node* p = fn_node->params[i];
      if (!p) continue;
      Param param{};
      param.name = p->name ? p->name : "<anon_param>";
      param.type = qtype_from(p->type, ValueCategory::LValue);
      param.span = make_span(p);
      ctx.params[param.name] = static_cast<uint32_t>(fn.params.size());
      fn.params.push_back(std::move(param));
    }

    if (!fn_node->body) {
      module_->fn_index[fn.name] = fn.id;
      module_->functions.push_back(std::move(fn));
      return;
    }

    const BlockId entry = create_block(ctx);
    fn.entry = entry;
    ctx.current_block = entry;

    lower_stmt_node(ctx, fn_node->body);

    if (fn.blocks.empty()) {
      fn.blocks.push_back(Block{entry, {}, false});
    }

    module_->fn_index[fn.name] = fn.id;
    module_->functions.push_back(std::move(fn));
  }

  void lower_global(const Node* gv) {
    GlobalInit computed_init{};
    bool has_init = false;

    // Handle compound literal at global scope: `T *p = &(T){...};`
    // The compound literal must be lowered to a separate static global, and
    // the parent global initialized to point at it.
    // IMPORTANT: the synthetic global must be pushed BEFORE allocating g.id,
    // since hir_to_llvm.cpp uses GlobalId.value as a direct vector index.
    if (gv->init && gv->init->kind == NK_ADDR &&
        gv->init->left && gv->init->left->kind == NK_COMPOUND_LIT) {
      const Node* clit = gv->init->left;
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
      ExprId addr_id = append_expr(gv->init, addr, ptr_ts);
      InitScalar sc{};
      sc.expr = addr_id;
      computed_init = sc;
      has_init = true;
    }

    GlobalVar g{};
    g.id = next_global_id();  // allocated AFTER any synthetic globals
    g.name = gv->name ? gv->name : "<anon_global>";
    g.type = qtype_from(gv->type, ValueCategory::LValue);
    g.linkage = {gv->is_static, gv->is_extern, false};
    g.is_const = gv->type.is_const;
    g.span = make_span(gv);

    if (has_init) {
      g.init = computed_init;
    } else if (gv->init) {
      g.init = lower_global_init(gv->init);
    }

    module_->global_index[g.name] = g.id;
    module_->globals.push_back(std::move(g));
  }

  // Reconstruct the full TypeSpec for a struct field from its HirStructField.
  TypeSpec field_type_of(const HirStructField& f) {
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

  GlobalId lower_static_local_global(FunctionCtx& ctx, const Node* n) {
    GlobalVar g{};
    g.id = next_global_id();
    g.name = "__static_local_" + sanitize_symbol(ctx.fn->name) + "_" + std::to_string(g.id.value);
    g.type = qtype_from(n->type, ValueCategory::LValue);
    g.linkage = {true, false, false};  // internal linkage
    g.is_const = n->type.is_const;
    g.span = make_span(n);
    if (n->init) g.init = lower_global_init(n->init, &ctx);
    module_->global_index[g.name] = g.id;
    module_->globals.push_back(std::move(g));
    return g.id;
  }

  GlobalInit lower_global_init(const Node* n, FunctionCtx* ctx = nullptr) {
    if (!n) return std::monostate{};
    if (n->kind == NK_INIT_LIST) {
      return lower_init_list(n, ctx);
    }
    // Compound literal at top level: extract its init list.
    if (n->kind == NK_COMPOUND_LIT && n->left && n->left->kind == NK_INIT_LIST) {
      return lower_init_list(n->left, ctx);
    }
    InitScalar s{};
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
      } else {
        InitScalar s{};
        s.expr = lower_expr(ctx, value_node);
        item.value = s;
      }
      out.items.push_back(std::move(item));
    }
    return out;
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
        for (int i = 0; i < n->n_children; ++i) {
          lower_stmt_node(ctx, n->children[i]);
        }
        if (new_scope) {
          ctx.locals = saved_locals;
          ctx.static_globals = saved_static_globals;
        }
        return;
      }
      case NK_DECL: {
        // Local function prototype (e.g. `int f1(char *);` inside a function body):
        // if the name is already registered as a known function, skip creating a
        // local alloca — later references will resolve directly to the global function.
        if (n->name && !n->init && module_->fn_index.count(n->name)) return;

        if (n->is_static) {
          if (n->name && n->name[0]) {
            ctx.static_globals[n->name] = lower_static_local_global(ctx, n);
          }
          return;
        }

        LocalDecl d{};
        d.id = next_local_id();
        d.name = n->name ? n->name : "<anon_local>";
        d.type = qtype_from(n->type, ValueCategory::LValue);
        if (n->type.array_rank > 0 && n->type.array_size_expr &&
            (n->type.array_size <= 0 || n->type.array_dims[0] <= 0)) {
          d.vla_size = lower_expr(&ctx, n->type.array_size_expr);
        }
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
        d.storage = n->is_static ? StorageClass::Static : StorageClass::Auto;
        d.span = make_span(n);
        const bool is_array_with_init_list =
            n->init && n->init->kind == NK_INIT_LIST && d.type.spec.array_rank == 1;
        const bool is_array_with_string_init =
            n->init && n->init->kind == NK_STR_LIT && d.type.spec.array_rank == 1;
        const bool is_struct_with_init_list =
            n->init && n->init->kind == NK_INIT_LIST &&
            (d.type.spec.base == TB_STRUCT || d.type.spec.base == TB_UNION) &&
            d.type.spec.ptr_level == 0 && d.type.spec.array_rank == 0;
        if (!is_array_with_init_list && !is_array_with_string_init &&
            !is_struct_with_init_list && n->init)
          d.init = lower_expr(&ctx, n->init);
        // For struct init lists, emit zeroinitializer first then field-by-field stores.
        if (is_struct_with_init_list) {
          TypeSpec int_ts{}; int_ts.base = TB_INT;
          d.init = append_expr(n, IntLiteral{0, false}, int_ts);
        }
        const LocalId lid = d.id;
        const TypeSpec decl_ts = d.type.spec;
        ctx.locals[d.name] = d.id;
        ctx.local_types[d.id.value] = d.type.spec;
        append_stmt(ctx, Stmt{StmtPayload{std::move(d)}, make_span(n)});
        // For array init lists, emit element-by-element assignments.
        if (is_array_with_init_list) {
          long long next_idx = 0;
          for (int ci = 0; ci < n->init->n_children; ++ci) {
            const Node* item = n->init->children[ci];
            if (!item) { ++next_idx; continue; }
            long long idx = next_idx;
            const Node* val_node = item;
            if (item->kind == NK_INIT_ITEM) {
              if (item->is_designated && item->is_index_desig) idx = item->desig_val;
              val_node = item->left ? item->left : item->right;
              if (!val_node) val_node = item->init;
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
        if (is_struct_with_init_list && decl_ts.tag && n->init) {
          auto is_agg = [](const TypeSpec& ts) {
            return (ts.base == TB_STRUCT || ts.base == TB_UNION) &&
                   ts.ptr_level == 0 && ts.array_rank == 0;
          };
          auto extract_val_node = [](const Node* item) -> const Node* {
            if (!item) return nullptr;
            if (item->kind != NK_INIT_ITEM) return item;
            const Node* v = item->left ? item->left : item->right;
            if (!v) v = item->init;
            return v;
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
            const Node* val_node = rhs_node;
            if (val_node->kind == NK_INIT_LIST && val_node->n_children > 0) {
              const Node* first = val_node->children[0];
              const Node* unwrapped = extract_val_node(first);
              if (unwrapped) val_node = unwrapped;
            }
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

                    const Node* val_node = extract_val_node(item);
                    if (is_agg(field_ts) || field_ts.array_rank > 0) {
                      if (val_node && val_node->kind == NK_INIT_LIST) {
                        int sub_cursor = 0;
                        consume_from_list(field_ts, me_id, val_node, sub_cursor);
                        ++cursor;
                      } else if (can_direct_assign_agg(field_ts, val_node)) {
                        append_assign(me_id, field_ts, val_node);
                        ++cursor;
                      } else {
                        // Brace elision: nested aggregate consumes from parent list.
                        consume_from_list(field_ts, me_id, list_node, cursor);
                      }
                    } else {
                      append_assign(me_id, field_ts, val_node);
                      ++cursor;
                    }
                    next_field = fi + 1;
                  }
                  return;
                }

                if (cur_ts.array_rank > 0) {
                  TypeSpec elem_ts = cur_ts;
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

                    const Node* val_node = extract_val_node(item);
                    if (is_agg(elem_ts) || elem_ts.array_rank > 0) {
                      if (val_node && val_node->kind == NK_INIT_LIST) {
                        int sub_cursor = 0;
                        consume_from_list(elem_ts, ie_id, val_node, sub_cursor);
                        ++cursor;
                      } else {
                        consume_from_list(elem_ts, ie_id, list_node, cursor);
                      }
                    } else {
                      append_assign(ie_id, elem_ts, val_node);
                      ++cursor;
                    }
                    next_idx = idx + 1;
                  }
                  return;
                }

                if (cursor < list_node->n_children) {
                  const Node* item = list_node->children[cursor];
                  const Node* val_node = extract_val_node(item);
                  append_assign(cur_lhs, cur_ts, val_node);
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
      case NK_RETURN: {
        ReturnStmt s{};
        if (n->left) s.expr = lower_expr(&ctx, n->left);
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        return;
      }
      case NK_IF: {
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

        ctx.switch_stack.push_back({parent_b, {}, {}});
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
          } else if (auto v = eval_int_const_expr(n->left, enum_consts_)) {
            case_val = *v;
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
        CaseRangeStmt s{};
        if (n->left && n->left->kind == NK_INT_LIT) s.range.lo = n->left->ival;
        if (n->right && n->right->kind == NK_INT_LIT) s.range.hi = n->right->ival;
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
        return append_expr(n, FloatLiteral{n->fval}, n->type);
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
        return append_expr(n, std::move(r), n->type, ValueCategory::LValue);
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
        c.to_type = qtype_from(n->type);
        c.expr = lower_expr(ctx, n->left);
        return append_expr(n, c, n->type);
      }
      case NK_CALL: {
        CallExpr c{};
        c.callee = lower_expr(ctx, n->left);
        for (int i = 0; i < n->n_children; ++i) c.args.push_back(lower_expr(ctx, n->children[i]));
        return append_expr(n, c, n->type);
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
          if (auto cv = eval_int_const_expr(cond, enum_consts_)) {
            return lower_expr(ctx, (*cv != 0) ? n->then_ : n->else_);
          }
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
        if (ctx && n->body) {
          lower_stmt_node(*ctx, n->body);
        }
        TypeSpec ts = n->type;
        if (ts.base == TB_VOID) ts.base = TB_INT;
        return append_expr(n, IntLiteral{0, false}, ts);
      }
      case NK_SIZEOF_EXPR: {
        SizeofExpr s{};
        s.expr = lower_expr(ctx, n->left);
        // sizeof always returns an integer (size_t ~ unsigned long)
        TypeSpec ts{}; ts.base = TB_ULONG;
        return append_expr(n, s, ts);
      }
      case NK_SIZEOF_TYPE: {
        SizeofTypeExpr s{};
        s.type = qtype_from(n->type);
        // sizeof always returns an integer (size_t ~ unsigned long)
        TypeSpec ts{}; ts.base = TB_ULONG;
        return append_expr(n, s, ts);
      }
      case NK_ALIGNOF_TYPE: {
        // Keep alignof as integer-typed constant in HIR to avoid inheriting the
        // queried type (e.g. float), which would produce invalid LLVM constants.
        TypeSpec ts{}; ts.base = TB_ULONG;
        return append_expr(n, IntLiteral{0, false}, ts);
      }
      case NK_COMPOUND_LIT: {
        // A compound literal (T){ ... } is an lvalue with automatic storage
        // duration.  Create an anonymous local, initialize it, and return a
        // DeclRef so callers can take its address or load from it.
        if (!ctx) break;  // global scope: handled elsewhere, fall to default
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

        if (is_struct_or_union && init_list) {
          // Struct/union: emit zeroinitializer store first, then field assignments below.
          TypeSpec int_ts{}; int_ts.base = TB_INT;
          d.init = append_expr(n, IntLiteral{0, false}, int_ts);
        } else if (is_array) {
          // Array: no init store; element-by-element assignments follow below.
        } else if (init_list && init_list->n_children > 0) {
          d.init = lower_expr(ctx, init_list->children[0]);
        } else if (n->left && !init_list) {
          d.init = lower_expr(ctx, n->left);
        }

        ctx->local_types[lid.value] = decl_ts;
        append_stmt(*ctx, Stmt{StmtPayload{std::move(d)}, make_span(n)});

        DeclRef dr{}; dr.name = "<clit>"; dr.local = lid;
        ExprId base_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);

        // Emit struct/union field-by-field initialization.
        if (is_struct_or_union && init_list && decl_ts.tag) {
          const auto sit = module_->struct_defs.find(decl_ts.tag);
          if (sit != module_->struct_defs.end()) {
            const auto& sd = sit->second;
            size_t fi = 0;
            for (int ci = 0; ci < init_list->n_children && fi < sd.fields.size(); ++ci) {
              const Node* item = init_list->children[ci];
              if (!item) { ++fi; continue; }
              const Node* val_node = item;
              if (item->kind == NK_INIT_ITEM) {
                val_node = item->left ? item->left : item->right;
                if (!val_node) val_node = item->init;
                // Handle designated initializers.
                if (item->is_designated && !item->is_index_desig && item->desig_field) {
                  for (size_t k = 0; k < sd.fields.size(); ++k) {
                    if (sd.fields[k].name == item->desig_field) { fi = k; break; }
                  }
                }
              }
              if (!val_node || fi >= sd.fields.size()) { ++fi; continue; }
              const HirStructField& fld = sd.fields[fi];
              TypeSpec field_ts = field_type_of(fld);
              MemberExpr me{}; me.base = base_id; me.field = fld.name; me.is_arrow = false;
              ExprId me_id = append_expr(n, me, field_ts, ValueCategory::LValue);
              ExprId val_id = lower_expr(ctx, val_node);
              AssignExpr ae{}; ae.lhs = me_id; ae.rhs = val_id;
              ExprId ae_id = append_expr(n, ae, field_ts);
              ExprStmt es{}; es.expr = ae_id;
              append_stmt(*ctx, Stmt{StmtPayload{es}, make_span(n)});
              ++fi;
            }
          }
        }

        // Emit array element-by-element initialization.
        if (is_array && init_list) {
          TypeSpec elem_ts = decl_ts;
          elem_ts.array_rank--;
          elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
          const long long bound = decl_ts.array_size > 0 ? decl_ts.array_size : (1LL << 20);
          long long next_idx = 0;
          for (int ci = 0; ci < init_list->n_children && next_idx < bound; ++ci) {
            const Node* item = init_list->children[ci];
            if (!item) { ++next_idx; continue; }
            long long idx = next_idx;
            const Node* val_node = item;
            if (item->kind == NK_INIT_ITEM) {
              val_node = item->left ? item->left : item->right;
              if (!val_node) val_node = item->init;
              if (item->is_designated && item->is_index_desig) idx = item->desig_val;
            }
            if (!val_node) { ++next_idx; continue; }
            TypeSpec idx_ts{}; idx_ts.base = TB_INT;
            ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
            IndexExpr ie{}; ie.base = base_id; ie.index = idx_id;
            ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
            ExprId val_id = lower_expr(ctx, val_node);
            AssignExpr ae{}; ae.lhs = ie_id; ae.rhs = val_id;
            ExprId ae_id = append_expr(n, ae, elem_ts);
            ExprStmt es{}; es.expr = ae_id;
            append_stmt(*ctx, Stmt{StmtPayload{es}, make_span(n)});
            next_idx = idx + 1;
          }
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

  Module* module_ = nullptr;
  std::unordered_map<std::string, long long> enum_consts_;

};

}  // namespace

Module lower_ast_to_hir(const Node* program_root) {
  Lowerer l;
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

}  // namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir
