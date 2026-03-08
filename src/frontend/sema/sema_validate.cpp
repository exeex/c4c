#include "sema_validate.hpp"

#include <algorithm>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tinyc2ll::frontend_cxx::sema {

namespace {

struct FunctionSig {
  TypeSpec ret{};
  std::vector<TypeSpec> params;
  bool variadic = false;
};

struct ExprInfo {
  TypeSpec type{};
  bool valid = false;
  bool is_lvalue = false;
  bool is_const_lvalue = false;
};

TypeSpec make_int_ts() {
  TypeSpec ts{};
  ts.base = TB_INT;
  ts.array_size = -1;
  ts.array_rank = 0;
  return ts;
}

TypeSpec decay_array(TypeSpec ts) {
  if (ts.array_rank > 0) {
    ts.array_rank = 0;
    ts.array_size = -1;
    ts.ptr_level += 1;
  }
  return ts;
}

bool drops_const_on_ptr_assign(const TypeSpec& lhs, const TypeSpec& rhs) {
  TypeSpec l = decay_array(lhs);
  TypeSpec r = decay_array(rhs);
  if (l.ptr_level <= 0 || r.ptr_level <= 0) return false;
  return !l.is_const && r.is_const;
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

bool same_tagged_type(const TypeSpec& a, const TypeSpec& b) {
  if (a.base != b.base) return false;
  if (a.base != TB_STRUCT && a.base != TB_UNION && a.base != TB_ENUM) return true;
  if (a.tag == nullptr || b.tag == nullptr) return false;
  return std::string(a.tag) == std::string(b.tag);
}

bool call_arg_type_compatible(const TypeSpec& param_raw, const TypeSpec& arg_raw) {
  const TypeSpec param = decay_array(param_raw);
  const TypeSpec arg = decay_array(arg_raw);

  // pointer vs non-pointer mismatch
  const bool p_ptr = param.ptr_level > 0;
  const bool a_ptr = arg.ptr_level > 0;
  if (p_ptr != a_ptr) return false;

  // Pointer compatibility: require same depth and pointee family.
  if (p_ptr && a_ptr) {
    if (param.ptr_level != arg.ptr_level) return false;
    // void* is compatible with any object pointer at level 1.
    if (param.ptr_level == 1 && (param.base == TB_VOID || arg.base == TB_VOID)) return true;
    if (param.base == TB_STRUCT || param.base == TB_UNION || param.base == TB_ENUM ||
        arg.base == TB_STRUCT || arg.base == TB_UNION || arg.base == TB_ENUM) {
      return same_tagged_type(param, arg);
    }
    return param.base == arg.base;
  }

  // Non-pointer: structs/unions must match exactly.
  if (param.base == TB_STRUCT || param.base == TB_UNION ||
      arg.base == TB_STRUCT || arg.base == TB_UNION) {
    return same_tagged_type(param, arg);
  }

  // Arithmetic scalar conversions are allowed.
  if (is_arithmetic_base(param.base) && is_arithmetic_base(arg.base)) return true;

  return param.base == arg.base;
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
  std::unordered_map<std::string, TypeSpec> enum_consts_;
  std::unordered_map<std::string, FunctionSig> funcs_;
  std::unordered_set<std::string> complete_structs_;
  std::unordered_set<std::string> complete_unions_;
  std::vector<std::unordered_map<std::string, ScopedSym>> scopes_;
  bool suppress_uninit_read_ = false;

  int loop_depth_ = 0;

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
    return true;
  }

  void emit(int line, std::string msg) {
    diags_.push_back(Diagnostic{line, std::move(msg)});
  }

  ValidateResult finish() {
    ValidateResult r;
    r.ok = diags_.empty();
    r.diagnostics = std::move(diags_);
    return r;
  }

  void enter_scope() { scopes_.emplace_back(); }

  void leave_scope() {
    if (!scopes_.empty()) scopes_.pop_back();
  }

  void bind_local(const std::string& name, const TypeSpec& ts, bool initialized, int line) {
    if (scopes_.empty()) enter_scope();
    auto& scope = scopes_.back();
    if (scope.find(name) != scope.end()) {
      emit(line, "redefinition of symbol '" + name + "' in same scope");
      return;
    }
    scope[name] = ScopedSym{ts, initialized};
  }

  std::optional<ScopedSym> lookup_symbol(const std::string& name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      auto f = it->find(name);
      if (f != it->end()) return f->second;
    }
    auto g = globals_.find(name);
    if (g != globals_.end()) return ScopedSym{g->second, true};
    auto fn = funcs_.find(name);
    if (fn != funcs_.end()) {
      TypeSpec fts = fn->second.ret;
      fts.ptr_level += 1;
      return ScopedSym{fts, true};
    }
    auto ec = enum_consts_.find(name);
    if (ec != enum_consts_.end()) return ScopedSym{ec->second, true};
    return std::nullopt;
  }

  void bind_enum_constants_global(const Node* n) {
    if (!n || n->kind != NK_ENUM_DEF || n->n_enum_variants <= 0 || !n->enum_names) return;
    TypeSpec its = make_int_ts();
    for (int i = 0; i < n->n_enum_variants; ++i) {
      if (!n->enum_names[i] || !n->enum_names[i][0]) continue;
      enum_consts_[n->enum_names[i]] = its;
    }
  }

  void bind_enum_constants_local(const Node* n) {
    if (!n || n->kind != NK_ENUM_DEF || n->n_enum_variants <= 0 || !n->enum_names) return;
    TypeSpec its = make_int_ts();
    for (int i = 0; i < n->n_enum_variants; ++i) {
      if (!n->enum_names[i] || !n->enum_names[i][0]) continue;
      bind_local(n->enum_names[i], its, true, n->line);
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
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      auto f = it->find(n->name);
      if (f != it->end()) {
        f->second.initialized = true;
        return;
      }
    }
  }

  bool is_complete_object_type(const TypeSpec& ts) const {
    if (ts.ptr_level > 0 || ts.array_rank > 0) return true;
    if ((ts.base != TB_STRUCT && ts.base != TB_UNION) || !ts.tag || !ts.tag[0]) return true;
    const std::string tag(ts.tag);
    if (ts.base == TB_STRUCT) return complete_structs_.count(tag) > 0;
    return complete_unions_.count(tag) > 0;
  }

  void note_struct_def(const Node* n) {
    if (!n || n->kind != NK_STRUCT_DEF || !n->name || !n->name[0]) return;
    if (n->n_fields <= 0) return;
    const std::string tag(n->name);
    if (n->is_union) {
      complete_unions_.insert(tag);
    } else {
      complete_structs_.insert(tag);
    }
  }

  void collect_toplevel_node(const Node* n) {
    if (!n) return;
    if (n->kind == NK_BLOCK) {
      // Multi-declarator global block: recurse into children.
      for (int i = 0; i < n->n_children; ++i) collect_toplevel_node(n->children[i]);
      return;
    }
    if (n->kind == NK_GLOBAL_VAR) {
      if (n->name && n->name[0]) globals_[n->name] = n->type;
      if (!is_complete_object_type(n->type)) {
        emit(n->line, "object has incomplete struct/union type");
      }
    } else if (n->kind == NK_FUNCTION) {
      if (!n->name || !n->name[0]) return;
      FunctionSig sig;
      sig.ret = n->type;
      sig.variadic = n->variadic;
      for (int p = 0; p < n->n_params; ++p) {
        const Node* param = n->params[p];
        if (!param) continue;
        // In C, `f(void)` means no parameters; skip the void sentinel param.
        const TypeSpec& pt = param->type;
        if (pt.base == TB_VOID && pt.ptr_level == 0 && pt.array_rank == 0) continue;
        sig.params.push_back(pt);
      }
      funcs_[n->name] = std::move(sig);
    } else if (n->kind == NK_STRUCT_DEF) {
      note_struct_def(n);
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
    } else if (n->kind == NK_STRUCT_DEF) {
      note_struct_def(n);
    } else if (n->kind == NK_ENUM_DEF) {
      bind_enum_constants_global(n);
    }
  }

  void validate_global(const Node* n) {
    if (!n) return;
    if (n->init) {
      ExprInfo rhs = infer_expr(n->init);
      if (drops_const_on_ptr_assign(n->type, rhs.type)) {
        emit(n->line, "discarding const qualifier in pointer initialization");
      }
    }
  }

  void validate_function(const Node* fn) {
    enter_scope();
    for (int i = 0; i < fn->n_params; ++i) {
      const Node* p = fn->params[i];
      if (!p || !p->name || !p->name[0]) continue;
      bind_local(p->name, p->type, true, p->line);
    }

    if (fn->body) {
      if (fn->body->kind == NK_BLOCK) {
        for (int i = 0; i < fn->body->n_children; ++i) visit_stmt(fn->body->children[i]);
      } else {
        visit_stmt(fn->body);
      }
    }
    leave_scope();
  }

  void validate_decl_init(const Node* decl) {
    if (!decl) return;
    if (!is_complete_object_type(decl->type)) {
      emit(decl->line, "object has incomplete struct/union type");
    }
    if (decl->init) {
      ExprInfo rhs = infer_expr(decl->init);
      if (drops_const_on_ptr_assign(decl->type, rhs.type)) {
        emit(decl->line, "discarding const qualifier in pointer initialization");
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
        if (n->name && n->name[0]) bind_local(n->name, n->type, n->init != nullptr, n->line);
        validate_decl_init(n);
        return;
      }
      case NK_EXPR_STMT: {
        if (n->left) (void)infer_expr(n->left);
        return;
      }
      case NK_RETURN: {
        if (n->left) (void)infer_expr(n->left);
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
      case NK_SWITCH: {
        if (n->cond) (void)infer_expr(n->cond);
        switch_stack_.push_back(SwitchCtx{});
        if (n->body) visit_stmt(n->body);
        switch_stack_.pop_back();
        return;
      }
      case NK_CASE: {
        if (switch_stack_.empty()) {
          emit(n->line, "case label not within a switch statement");
        } else if (n->left && n->left->kind == NK_INT_LIT) {
          long long v = n->left->ival;
          auto [it, inserted] = switch_stack_.back().case_vals.insert(v);
          if (!inserted) emit(n->line, "duplicate case label in switch");
        }
        if (n->body) visit_stmt(n->body);
        return;
      }
      case NK_DEFAULT: {
        if (switch_stack_.empty()) {
          emit(n->line, "default label not within a switch statement");
        } else if (switch_stack_.back().has_default) {
          emit(n->line, "duplicate default label in switch");
        } else {
          switch_stack_.back().has_default = true;
        }
        if (n->body) visit_stmt(n->body);
        return;
      }
      case NK_BREAK: {
        if (loop_depth_ <= 0 && switch_stack_.empty()) {
          emit(n->line, "break statement not within loop or switch");
        }
        return;
      }
      case NK_CONTINUE: {
        if (loop_depth_ <= 0) {
          emit(n->line, "continue statement not within loop");
        }
        return;
      }
      case NK_LABEL: {
        if (n->body) visit_stmt(n->body);
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
      case NK_FLOAT_LIT:
      case NK_CHAR_LIT:
        out.valid = true;
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
        auto sym = lookup_symbol(n->name);
        if (!sym.has_value()) {
          emit(n->line, std::string("use of undeclared identifier '") + n->name + "'");
          return out;
        }
        out.valid = true;
        out.type = sym->type;
        out.is_lvalue = true;
        // is_const with ptr_level>0 means the pointee is const, not the pointer.
        // Only the pointer variable itself is a const lvalue if ptr_level==0.
        out.is_const_lvalue = out.type.is_const &&
                              out.type.ptr_level == 0 &&
                              out.type.array_rank == 0;
        // Note: reading an uninitialized local is UB in C but not a compile error.
        // Emitting an error here causes false positives on valid programs.
        return out;
      }
      case NK_ADDR: {
        bool old_suppress = suppress_uninit_read_;
        suppress_uninit_read_ = true;
        ExprInfo base = infer_expr(n->left);
        suppress_uninit_read_ = old_suppress;
        out = base;
        out.valid = base.valid;
        out.is_lvalue = false;
        out.is_const_lvalue = false;
        out.type = base.type;
        out.type = decay_array(out.type);
        out.type.ptr_level += 1;
        return out;
      }
      case NK_DEREF: {
        ExprInfo base = infer_expr(n->left);
        out.valid = base.valid;
        out.type = base.type;
        out.is_lvalue = true;
        if (out.type.ptr_level > 0) out.type.ptr_level -= 1;
        out.is_const_lvalue = out.type.is_const;
        return out;
      }
      case NK_INDEX: {
        ExprInfo arr = infer_expr(n->left);
        (void)infer_expr(n->right);
        out.valid = arr.valid;
        out.type = arr.type;
        out.type = decay_array(out.type);
        if (out.type.ptr_level > 0) out.type.ptr_level -= 1;
        out.is_lvalue = true;
        out.is_const_lvalue = out.type.is_const;
        return out;
      }
      case NK_MEMBER: {
        ExprInfo base = infer_expr(n->left);
        out.valid = base.valid;
        out.is_lvalue = true;
        out.type = make_int_ts();
        out.is_const_lvalue = base.is_const_lvalue;
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
          emit(n->line, "assignment to const-qualified lvalue");
        }
        if (drops_const_on_ptr_assign(lhs.type, rhs.type)) {
          emit(n->line, "discarding const qualifier in pointer assignment");
        }
        mark_initialized_if_local_var(n->left);
        out.valid = lhs.valid && rhs.valid;
        out.type = lhs.type;
        return out;
      }
      case NK_UNARY: {
        ExprInfo e = infer_expr(n->left);
        out = e;
        if (n->op &&
            (std::string(n->op).rfind("++", 0) == 0 || std::string(n->op).rfind("--", 0) == 0)) {
          if (e.is_const_lvalue) {
            emit(n->line, "increment/decrement of const-qualified object");
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
            emit(n->line, "increment/decrement of const-qualified object");
          }
        }
        out.is_lvalue = false;
        out.is_const_lvalue = false;
        return out;
      }
      case NK_CALL: {
        for (int i = 0; i < n->n_children; ++i) {
          (void)infer_expr(n->children[i]);
        }
        if (n->left && n->left->kind == NK_VAR && n->left->name && n->left->name[0]) {
          auto it = funcs_.find(n->left->name);
          if (it != funcs_.end()) {
            const FunctionSig& sig = it->second;
            const int argc = n->n_children;
            const int required = static_cast<int>(sig.params.size());
            if ((!sig.variadic && argc != required) || (sig.variadic && argc < required)) {
              emit(n->line, "function call arity mismatch");
            }
            const int check_n = std::min(argc, required);
            for (int i = 0; i < check_n; ++i) {
              ExprInfo arg = infer_expr(n->children[i]);
              if (drops_const_on_ptr_assign(sig.params[i], arg.type)) {
                emit(n->line, "passing const-qualified pointer to mutable parameter");
              }
              if (!call_arg_type_compatible(sig.params[i], arg.type)) {
                emit(n->line, "function call argument type mismatch");
              }
            }
            out.valid = true;
            out.type = sig.ret;
            return out;
          }
        }
        out.valid = true;
        return out;
      }
      case NK_CAST: {
        ExprInfo src = infer_expr(n->left);
        out.valid = true;
        out.type = n->type;
        if (n->type.base == TB_TYPEDEF) {
          const std::string tname = n->type.tag ? n->type.tag : "<anonymous>";
          emit(n->line, "cast to unknown type name '" + tname + "'");
        }
        if (drops_const_on_ptr_assign(n->type, src.type)) {
          emit(n->line, "cast discards const qualifier from pointer target type");
        }
        if (!is_complete_object_type(n->type)) {
          emit(n->line, "cast to incomplete struct/union object type");
        }
        return out;
      }
      case NK_TERNARY: {
        (void)infer_expr(n->cond);
        ExprInfo t = infer_expr(n->then_);
        ExprInfo e = infer_expr(n->else_);
        out = t.valid ? t : e;
        out.is_lvalue = false;
        out.is_const_lvalue = false;
        return out;
      }
      case NK_BINOP:
      case NK_COMMA_EXPR:
        (void)infer_expr(n->left);
        (void)infer_expr(n->right);
        out.valid = true;
        return out;
      case NK_SIZEOF_EXPR:
        {
          bool old_suppress = suppress_uninit_read_;
          suppress_uninit_read_ = true;
          (void)infer_expr(n->left);
          suppress_uninit_read_ = old_suppress;
        }
        out.valid = true;
        return out;
      case NK_SIZEOF_TYPE:
      case NK_ALIGNOF_TYPE:
        out.valid = true;
        return out;
      case NK_INIT_LIST:
      case NK_COMPOUND_LIT:
        for (int i = 0; i < n->n_children; ++i) (void)infer_expr(n->children[i]);
        out.valid = true;
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
    std::cerr << file << ":" << d.line << ":1: error: " << d.message << "\n";
  }
}

}  // namespace tinyc2ll::frontend_cxx::sema
