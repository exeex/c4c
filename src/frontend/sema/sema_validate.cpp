#include "sema_validate.hpp"

#include <algorithm>
#include <cstring>
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
  // True when this expression is a function name (from funcs_ lookup).
  // In C, a function name and &function-name have the same pointer type, so
  // NK_ADDR should not add an extra ptr_level when this flag is set.
  bool is_fn_name = false;
};

TypeSpec make_int_ts() {
  TypeSpec ts{};
  ts.base = TB_INT;
  ts.array_size = -1;
  ts.array_rank = 0;
  return ts;
}

TypeSpec decay_array(TypeSpec ts) {
  // is_ptr_to_array means the type is already a pointer wrapping an array (e.g. char (*)[4]);
  // do not decay such types a second time.
  if (ts.array_rank > 0 && !ts.is_ptr_to_array) {
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
  ts = decay_array(ts);
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
  if (a.ptr_level != b.ptr_level) return false;
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
  if (a.variadic != b.variadic) return false;
  if (a.params.size() != b.params.size()) return false;
  for (size_t i = 0; i < a.params.size(); ++i) {
    if (!same_types_for_function_compat(a.params[i], b.params[i], true)) return false;
  }
  return true;
}

bool same_tagged_type(const TypeSpec& a, const TypeSpec& b) {
  if (a.base != b.base) return false;
  if (a.base != TB_STRUCT && a.base != TB_UNION && a.base != TB_ENUM) return true;
  if (a.tag == nullptr || b.tag == nullptr) return false;
  return std::string(a.tag) == std::string(b.tag);
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

using EnumConstMap = std::unordered_map<std::string, long long>;

bool eval_int_const_expr(const Node* n, long long& out, const EnumConstMap* ev = nullptr) {
  if (!n) return false;
  switch (n->kind) {
    case NK_INT_LIT:
    case NK_CHAR_LIT:
      out = n->ival;
      return true;
    case NK_VAR:
      // Enum constant reference: look up value if available.
      if (ev && n->name) {
        auto it = ev->find(n->name);
        if (it != ev->end()) { out = it->second; return true; }
      }
      return false;
    case NK_CAST:
      return eval_int_const_expr(n->left, out, ev);
    case NK_UNARY: {
      long long v = 0;
      if (!eval_int_const_expr(n->left, v, ev)) return false;
      const char* op = n->op ? n->op : "";
      if (std::strcmp(op, "+") == 0) out = +v;
      else if (std::strcmp(op, "-") == 0) out = -v;
      else if (std::strcmp(op, "~") == 0) out = ~v;
      else if (std::strcmp(op, "!") == 0) out = !v;
      else return false;
      return true;
    }
    case NK_BINOP:
    case NK_COMMA_EXPR: {
      long long l = 0, r = 0;
      if (!eval_int_const_expr(n->left, l, ev)) return false;
      if (!eval_int_const_expr(n->right, r, ev)) return false;
      const char* op = n->op ? n->op : "";
      if (std::strcmp(op, "+") == 0) out = l + r;
      else if (std::strcmp(op, "-") == 0) out = l - r;
      else if (std::strcmp(op, "*") == 0) out = l * r;
      else if (std::strcmp(op, "/") == 0) out = (r == 0) ? 0 : (l / r);
      else if (std::strcmp(op, "%") == 0) out = (r == 0) ? 0 : (l % r);
      else if (std::strcmp(op, "<<") == 0) out = l << r;
      else if (std::strcmp(op, ">>") == 0) out = l >> r;
      else if (std::strcmp(op, "&") == 0) out = l & r;
      else if (std::strcmp(op, "|") == 0) out = l | r;
      else if (std::strcmp(op, "^") == 0) out = l ^ r;
      else if (std::strcmp(op, "&&") == 0) out = (l && r);
      else if (std::strcmp(op, "||") == 0) out = (l || r);
      else if (std::strcmp(op, "<") == 0) out = (l < r);
      else if (std::strcmp(op, "<=") == 0) out = (l <= r);
      else if (std::strcmp(op, ">") == 0) out = (l > r);
      else if (std::strcmp(op, ">=") == 0) out = (l >= r);
      else if (std::strcmp(op, "==") == 0) out = (l == r);
      else if (std::strcmp(op, "!=") == 0) out = (l != r);
      else return false;
      return true;
    }
    default:
      return false;
  }
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
    // void* conversion is for object pointers, i.e. one level of indirection.
    // C11 6.3.2.3p1.
    if (dst.ptr_level == 1 && src.ptr_level == 1 &&
        (dst.base == TB_VOID || src.base == TB_VOID)) return true;
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
    return dst.base == src.base;
  }

  // Non-pointer: structs/unions must match exactly.
  if (dst.base == TB_STRUCT || dst.base == TB_UNION ||
      src.base == TB_STRUCT || src.base == TB_UNION) {
    return same_tagged_type(dst, src);
  }

  // Arithmetic scalar conversions are allowed.
  if (is_arithmetic_base(dst.base) && is_arithmetic_base(src.base)) return true;

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
  std::unordered_map<std::string, TypeSpec> enum_consts_;
  EnumConstMap enum_const_vals_;   // integer values of enum constants
  std::unordered_map<std::string, FunctionSig> funcs_;
  std::unordered_set<std::string> complete_structs_;
  std::unordered_set<std::string> complete_unions_;
  std::vector<std::unordered_map<std::string, ScopedSym>> scopes_;
  bool suppress_uninit_read_ = false;

  int loop_depth_ = 0;
  // Set when the current function contains any goto statement.  Functions with
  // gotos have non-trivial control flow, so uninit-read detection is suppressed
  // to avoid false positives on dead code paths.
  bool fn_has_goto_ = false;
  bool in_function_ = false;
  TypeSpec current_fn_ret_{};

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
      if (n->enum_vals) enum_const_vals_[n->enum_names[i]] = n->enum_vals[i];
    }
  }

  void bind_enum_constants_local(const Node* n) {
    if (!n || n->kind != NK_ENUM_DEF || n->n_enum_variants <= 0 || !n->enum_names) return;
    TypeSpec its = make_int_ts();
    for (int i = 0; i < n->n_enum_variants; ++i) {
      if (!n->enum_names[i] || !n->enum_names[i][0]) continue;
      bind_local(n->enum_names[i], its, true, n->line);
      if (n->enum_vals) enum_const_vals_[n->enum_names[i]] = n->enum_vals[i];
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
      auto it = funcs_.find(n->name);
      if (it != funcs_.end()) {
        if (!function_sig_compatible(it->second, sig)) {
          emit(n->line, std::string("conflicting types for function '") + n->name + "'");
        }
      } else {
        funcs_[n->name] = std::move(sig);
      }
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
      // String literals can initialize char/wchar_t arrays with any char-like type.
      const bool init_is_str = n->init->kind == NK_STR_LIT;
      if (!init_is_str && rhs.valid &&
          !implicit_convertible(n->type, rhs.type, is_null_pointer_constant_expr(n->init))) {
        emit(n->line, "incompatible initializer type");
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

  void validate_function(const Node* fn) {
    const bool old_in_function = in_function_;
    const TypeSpec old_fn_ret = current_fn_ret_;
    in_function_ = true;
    current_fn_ret_ = fn->type;
    fn_has_goto_ = fn->body && node_contains_goto(fn->body);
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
    in_function_ = old_in_function;
    current_fn_ret_ = old_fn_ret;
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
      // String literals can initialize char/wchar_t arrays with any char-like type.
      const bool init_is_str = decl->init->kind == NK_STR_LIT;
      if (!init_is_str && rhs.valid &&
          !implicit_convertible(decl->type, rhs.type, is_null_pointer_constant_expr(decl->init))) {
        emit(decl->line, "incompatible initializer type");
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
        if (!in_function_) {
          emit(n->line, "return statement not within function");
          return;
        }

        const bool fn_returns_void =
            current_fn_ret_.base == TB_VOID &&
            current_fn_ret_.ptr_level == 0 &&
            current_fn_ret_.array_rank == 0;

        if (fn_returns_void && n->left) {
          emit(n->line, "return with a value in function returning void");
        } else if (!fn_returns_void && !n->left) {
          emit(n->line, "non-void function should return a value");
        }

        if (n->left) {
          ExprInfo rv_info = infer_expr(n->left);
          // Detect direct return of an uninitialized plain-scalar local.
          // Skip if the function has goto statements (complex control flow
          // can make the read unreachable, leading to false positives).
          const Node* rv = n->left;
          if (!fn_has_goto_ && rv->kind == NK_VAR && rv->name && rv->name[0]) {
            auto sym = lookup_symbol(rv->name);
            if (sym.has_value() && !sym->initialized &&
                tracks_uninit_read(sym->type)) {
              emit(rv->line, std::string("read of uninitialized variable '") +
                   rv->name + "'");
            }
          }
          if (!fn_returns_void && rv_info.valid &&
              !implicit_convertible(
                  current_fn_ret_, rv_info.type, is_null_pointer_constant_expr(n->left))) {
            emit(n->line, "incompatible return type");
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
      case NK_SWITCH: {
        if (n->cond) {
          ExprInfo c = infer_expr(n->cond);
          if (c.valid && !is_switch_integer_type(c.type)) {
            emit(n->line, "switch quantity is not an integer");
          }
        }
        switch_stack_.push_back(SwitchCtx{});
        if (n->body) visit_stmt(n->body);
        switch_stack_.pop_back();
        return;
      }
      case NK_CASE: {
        if (switch_stack_.empty()) {
          emit(n->line, "case label not within a switch statement");
        } else if (n->left) {
          ExprInfo case_expr = infer_expr(n->left);
          if (case_expr.valid && !is_switch_integer_type(case_expr.type)) {
            emit(n->line, "case label does not have an integer type");
          }
          long long v = 0;
          if (!eval_int_const_expr(n->left, v, &enum_const_vals_)) {
            emit(n->line, "case label does not reduce to an integer constant");
          } else {
            auto [it, inserted] = switch_stack_.back().case_vals.insert(v);
            if (!inserted) emit(n->line, "duplicate case label in switch");
          }
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
        // Mark function names so that &func doesn't add a spurious ptr_level.
        out.is_fn_name = funcs_.find(n->name) != funcs_.end();
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
        out.is_fn_name = false;
        out.type = base.type;
        out.type = decay_array(out.type);
        // In C, &function-name has the same type as function-name (both decay
        // to a function pointer).  Do not add an extra ptr_level for functions.
        if (!base.is_fn_name) {
          out.type.ptr_level += 1;
        }
        return out;
      }
      case NK_DEREF: {
        ExprInfo base = infer_expr(n->left);
        out.valid = base.valid;
        out.type = base.type;
        out.is_lvalue = true;
        if (out.type.ptr_level > 0) out.type.ptr_level -= 1;
        // is_const=true with ptr_level>0 means the *pointee* is const, not the
        // pointer itself.  After dereference, the lvalue is const only when the
        // resulting type is a plain (non-pointer) const-qualified object.
        out.is_const_lvalue = out.type.is_const && out.type.ptr_level == 0;
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
        // We don't track struct field types, so mark valid=false to suppress
        // false-positive incompatible-assignment-type errors for member accesses.
        out.valid = false;
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
        // Only type-check simple '=' assignments; compound operators (+=, -=, etc.)
        // are in-place operations whose result type is always the lhs type.
        const bool is_simple_assign = n->op && n->op[0] == '=' && n->op[1] == '\0';
        if (is_simple_assign && lhs.valid && rhs.valid &&
            !implicit_convertible(lhs.type, rhs.type, is_null_pointer_constant_expr(n->right))) {
          emit(n->line, "incompatible assignment type");
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
              if (arg.valid &&
                  !implicit_convertible(
                      sig.params[i], arg.type, is_null_pointer_constant_expr(n->children[i]))) {
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
        out.valid = true;
        // Pointer arithmetic: if one operand is a pointer and op is + or -, the
        // result has the pointer type so subsequent assignment checks don't fire.
        if (n->op && n->op[1] == '\0' &&
            (n->op[0] == '+' || n->op[0] == '-')) {
          if (l.type.ptr_level > 0 && r.type.ptr_level == 0) {
            out.type = l.type;
          } else if (r.type.ptr_level > 0 && l.type.ptr_level == 0) {
            out.type = r.type;
          }
          // ptr - ptr yields ptrdiff_t; keep make_int_ts() default.
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
            emit(n->line, "invalid application of sizeof to incomplete type");
          }
        }
        out.valid = true;
        return out;
      case NK_SIZEOF_TYPE:
        if (!is_complete_object_type(n->type)) {
          emit(n->line, "invalid application of sizeof to incomplete type");
        }
        out.valid = true;
        return out;
      case NK_ALIGNOF_TYPE:
        if (!is_complete_object_type(n->type)) {
          emit(n->line, "invalid application of _Alignof to incomplete type");
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
    std::cerr << file << ":" << d.line << ":1: error: " << d.message << "\n";
  }
}

}  // namespace tinyc2ll::frontend_cxx::sema
