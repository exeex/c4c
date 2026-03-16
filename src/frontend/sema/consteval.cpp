#include "consteval.hpp"

#include <cctype>
#include <cstdint>
#include <cstring>

namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir {
namespace {

bool is_any_int_base_local(TypeBase b) {
  switch (b) {
    case TB_BOOL:
    case TB_CHAR:
    case TB_SCHAR:
    case TB_UCHAR:
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
    case TB_ENUM:
      return true;
    default:
      return false;
  }
}

bool is_signed_int_base_local(TypeBase b) {
  switch (b) {
    case TB_CHAR:
    case TB_SCHAR:
    case TB_SHORT:
    case TB_INT:
    case TB_LONG:
    case TB_LONGLONG:
    case TB_INT128:
    case TB_ENUM:
      return true;
    default:
      return false;
  }
}

int int_bits_local(TypeBase b) {
  switch (b) {
    case TB_BOOL: return 1;
    case TB_CHAR:
    case TB_SCHAR:
    case TB_UCHAR: return 8;
    case TB_SHORT:
    case TB_USHORT: return 16;
    case TB_INT:
    case TB_UINT: return 32;
    case TB_LONG:
    case TB_ULONG:
    case TB_LONGLONG:
    case TB_ULONGLONG: return 64;
    case TB_INT128:
    case TB_UINT128: return 128;
    default: return 32;
  }
}

ConstValue apply_integer_cast(long long value, const TypeSpec& ts) {
  if (ts.ptr_level != 0 || ts.array_rank != 0 || !is_any_int_base_local(ts.base)) {
    return ConstValue::make_int(value);
  }
  const int bits = int_bits_local(ts.base);
  if (bits <= 0 || bits >= 64) return ConstValue::make_int(value);

  const unsigned long long mask = (1ULL << bits) - 1;
  const unsigned long long uv = static_cast<unsigned long long>(value) & mask;
  if (is_signed_int_base_local(ts.base) && bits > 1 && (uv >> (bits - 1))) {
    return ConstValue::make_int(static_cast<long long>(uv | ~mask));
  }
  return ConstValue::make_int(static_cast<long long>(uv));
}

// Resolve a TypeSpec through type_bindings if it's a TB_TYPEDEF with a known substitution.
TypeSpec resolve_type(const TypeSpec& ts, const TypeBindings* bindings) {
  if (!bindings || ts.base != TB_TYPEDEF || !ts.tag) return ts;
  auto it = bindings->find(ts.tag);
  if (it == bindings->end()) return ts;
  return it->second;
}

// Compute sizeof for a TypeSpec (scalar, pointer, array of scalar).
// Returns failure for struct/union/void/typedef types (require Module context or type resolution).
ConstEvalResult compute_sizeof_type(const TypeSpec& ts) {
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return ConstEvalResult::success(ConstValue::make_int(8));
  if (ts.base == TB_STRUCT || ts.base == TB_UNION)
    return ConstEvalResult::failure("sizeof(struct/union) not supported in constant evaluator");
  if (ts.base == TB_VOID || ts.base == TB_TYPEDEF)
    return ConstEvalResult::failure("sizeof: unresolved type");
  int sz = 0;
  switch (ts.base) {
    case TB_BOOL: case TB_CHAR: case TB_SCHAR: case TB_UCHAR: sz = 1; break;
    case TB_SHORT: case TB_USHORT: sz = 2; break;
    case TB_INT: case TB_UINT: case TB_FLOAT: sz = 4; break;
    case TB_LONG: case TB_ULONG: case TB_LONGLONG: case TB_ULONGLONG:
    case TB_DOUBLE: sz = 8; break;
    case TB_LONGDOUBLE: sz = 16; break;
    case TB_INT128: case TB_UINT128: sz = 16; break;
    default:
      return ConstEvalResult::failure("sizeof: unsupported type base");
  }
  if (ts.array_rank > 0 && ts.array_size > 0) {
    long long total = sz;
    total *= ts.array_size;
    for (int i = 1; i < ts.array_rank && i < 4; ++i) {
      if (ts.array_dims[i] > 0) total *= ts.array_dims[i];
    }
    return ConstEvalResult::success(ConstValue::make_int(total));
  }
  return ConstEvalResult::success(ConstValue::make_int(sz));
}

// Compute alignof for a TypeSpec (scalar, pointer, array).
// Returns failure for struct/union/void/typedef types.
ConstEvalResult compute_alignof_type(const TypeSpec& ts) {
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return ConstEvalResult::success(ConstValue::make_int(8));
  if (ts.base == TB_STRUCT || ts.base == TB_UNION)
    return ConstEvalResult::failure("alignof(struct/union) not supported in constant evaluator");
  if (ts.base == TB_VOID || ts.base == TB_TYPEDEF)
    return ConstEvalResult::failure("alignof: unresolved type");
  // For arrays, alignment is that of the element type.
  TypeSpec elem = ts;
  if (elem.array_rank > 0) {
    elem.array_rank = 0;
    elem.array_size = -1;
  }
  int align = 1;
  switch (elem.base) {
    case TB_BOOL: case TB_CHAR: case TB_SCHAR: case TB_UCHAR: align = 1; break;
    case TB_SHORT: case TB_USHORT: align = 2; break;
    case TB_INT: case TB_UINT: case TB_FLOAT: case TB_ENUM: align = 4; break;
    case TB_LONG: case TB_ULONG: case TB_LONGLONG: case TB_ULONGLONG:
    case TB_DOUBLE: align = 8; break;
    case TB_LONGDOUBLE: align = 16; break;
    case TB_INT128: case TB_UINT128: align = 16; break;
    default: align = 4; break;
  }
  if (ts.align_bytes > 0 && ts.align_bytes > align) align = ts.align_bytes;
  return ConstEvalResult::success(ConstValue::make_int(align));
}

ConstEvalResult eval_impl(const Node* n, const ConstEvalEnv& env) {
  if (!n) return ConstEvalResult::failure("null expression");
  switch (n->kind) {
    case NK_INT_LIT:
    case NK_CHAR_LIT:
      return ConstEvalResult::success(ConstValue::make_int(n->ival));
    case NK_VAR: {
      if (n->name && n->name[0]) {
        auto v = env.lookup(n->name);
        if (v) return ConstEvalResult::success(ConstValue::make_int(*v));
        return ConstEvalResult::failure(
            std::string("read of non-constant variable '") + n->name + "'");
      }
      return ConstEvalResult::failure("unnamed variable reference");
    }
    case NK_CAST: {
      auto r = eval_impl(n->left, env);
      if (!r.ok()) return r;
      return ConstEvalResult::success(apply_integer_cast(r.as_int(), n->type));
    }
    case NK_UNARY: {
      auto r = eval_impl(n->left, env);
      if (!r.ok()) return r;
      if (!n->op) return ConstEvalResult::failure("unary operator missing");
      long long v = r.as_int();
      if (std::strcmp(n->op, "+") == 0) return ConstEvalResult::success(ConstValue::make_int(v));
      if (std::strcmp(n->op, "-") == 0) return ConstEvalResult::success(ConstValue::make_int(-v));
      if (std::strcmp(n->op, "~") == 0) return ConstEvalResult::success(ConstValue::make_int(~v));
      if (std::strcmp(n->op, "!") == 0) return ConstEvalResult::success(ConstValue::make_int(!v));
      return ConstEvalResult::failure(
          std::string("unsupported unary operator '") + n->op + "' in constant expression");
    }
    case NK_BINOP: {
      auto lr = eval_impl(n->left, env);
      auto rr = eval_impl(n->right, env);
      if (!lr.ok()) return lr;
      if (!rr.ok()) return rr;
      if (!n->op) return ConstEvalResult::failure("binary operator missing");
      long long l = lr.as_int(), r = rr.as_int();
      long long result;
      if (std::strcmp(n->op, "+") == 0) result = l + r;
      else if (std::strcmp(n->op, "-") == 0) result = l - r;
      else if (std::strcmp(n->op, "*") == 0) result = l * r;
      else if (std::strcmp(n->op, "/") == 0) result = (r == 0) ? 0LL : (l / r);
      else if (std::strcmp(n->op, "%") == 0) result = (r == 0) ? 0LL : (l % r);
      else if (std::strcmp(n->op, "<<") == 0) result = l << r;
      else if (std::strcmp(n->op, ">>") == 0) result = l >> r;
      else if (std::strcmp(n->op, "&") == 0) result = l & r;
      else if (std::strcmp(n->op, "|") == 0) result = l | r;
      else if (std::strcmp(n->op, "^") == 0) result = l ^ r;
      else if (std::strcmp(n->op, "<") == 0) result = static_cast<long long>(l < r);
      else if (std::strcmp(n->op, "<=") == 0) result = static_cast<long long>(l <= r);
      else if (std::strcmp(n->op, ">") == 0) result = static_cast<long long>(l > r);
      else if (std::strcmp(n->op, ">=") == 0) result = static_cast<long long>(l >= r);
      else if (std::strcmp(n->op, "==") == 0) result = static_cast<long long>(l == r);
      else if (std::strcmp(n->op, "!=") == 0) result = static_cast<long long>(l != r);
      else if (std::strcmp(n->op, "&&") == 0) result = static_cast<long long>(l && r);
      else if (std::strcmp(n->op, "||") == 0) result = static_cast<long long>(l || r);
      else return ConstEvalResult::failure(
          std::string("unsupported binary operator '") + n->op + "' in constant expression");
      return ConstEvalResult::success(ConstValue::make_int(result));
    }
    case NK_TERNARY: {
      auto cr = eval_impl(n->cond ? n->cond : n->left, env);
      if (!cr.ok()) return cr;
      return eval_impl(cr.as_int() ? n->then_ : n->else_, env);
    }
    case NK_SIZEOF_TYPE:
      return compute_sizeof_type(resolve_type(n->type, env.type_bindings));
    case NK_SIZEOF_EXPR:
      // sizeof(expr) — use the expression's type from the AST.
      if (n->left) return compute_sizeof_type(resolve_type(n->left->type, env.type_bindings));
      return ConstEvalResult::failure("sizeof(expr): missing expression");
    case NK_ALIGNOF_TYPE:
      return compute_alignof_type(resolve_type(n->type, env.type_bindings));
    case NK_ALIGNOF_EXPR:
      // alignof(expr) — use the expression's type from the AST.
      if (n->left) return compute_alignof_type(resolve_type(n->left->type, env.type_bindings));
      return ConstEvalResult::failure("alignof(expr): missing expression");
    default:
      return ConstEvalResult::failure(
          std::string("unsupported expression kind (NK=") +
          std::to_string(static_cast<int>(n->kind)) + ") in constant expression");
  }
}

std::string decode_c_escaped_bytes_local(const std::string& raw) {
  std::string out;
  for (size_t i = 0; i < raw.size();) {
    const unsigned char c = static_cast<unsigned char>(raw[i]);
    if (c != '\\') {
      out.push_back(static_cast<char>(c));
      ++i;
      continue;
    }
    ++i;
    if (i >= raw.size()) break;
    const char esc = raw[i++];
    switch (esc) {
      case 'n': out.push_back('\n'); break;
      case 't': out.push_back('\t'); break;
      case 'r': out.push_back('\r'); break;
      case 'a': out.push_back('\a'); break;
      case 'b': out.push_back('\b'); break;
      case 'f': out.push_back('\f'); break;
      case 'v': out.push_back('\v'); break;
      case '\\': out.push_back('\\'); break;
      case '\'': out.push_back('\''); break;
      case '"': out.push_back('"'); break;
      case '?': out.push_back('\?'); break;
      case 'x': {
        unsigned value = 0;
        while (i < raw.size() && std::isxdigit(static_cast<unsigned char>(raw[i]))) {
          const unsigned char h = static_cast<unsigned char>(raw[i++]);
          value = value * 16 + (std::isdigit(h) ? h - '0' : std::tolower(h) - 'a' + 10);
        }
        out.push_back(static_cast<char>(value & 0xFF));
        break;
      }
      default:
        if (esc >= '0' && esc <= '7') {
          unsigned value = static_cast<unsigned>(esc - '0');
          for (int k = 0; k < 2 && i < raw.size() && raw[i] >= '0' && raw[i] <= '7'; ++k, ++i) {
            value = value * 8 + static_cast<unsigned>(raw[i] - '0');
          }
          out.push_back(static_cast<char>(value & 0xFF));
        } else {
          out.push_back(esc);
        }
        break;
    }
  }
  return out;
}

}  // namespace

// ── Unified evaluator entry point ────────────────────────────────────────────

ConstEvalResult evaluate_constant_expr(const Node* n, const ConstEvalEnv& env) {
  return eval_impl(n, env);
}

// ── Consteval function-body interpreter ──────────────────────────────────────

namespace {

constexpr int kMaxConstevalDepth = 64;

constexpr int kMaxConstevalSteps = 1000000;  // prevent infinite loops

// Result of interpreting a statement: either continue executing, or return/break/continue.
struct StmtResult {
  bool returned = false;
  bool did_break = false;
  bool did_continue = false;
  ConstValue return_val = ConstValue::unknown();
  std::string error;  // non-empty if evaluation failed
};

StmtResult interp_stmt(const Node* n, ConstMap& locals,
                       const ConstEvalEnv& outer_env,
                       const std::unordered_map<std::string, const Node*>& consteval_fns,
                       int depth, int& steps);

ConstEvalResult interp_expr(const Node* n, ConstMap& locals,
                            const ConstEvalEnv& outer_env,
                            const std::unordered_map<std::string, const Node*>& consteval_fns,
                            int depth) {
  if (!n) return ConstEvalResult::failure("null expression in consteval body");

  // Build an env that includes the interpreter locals on top of the outer env.
  ConstEvalEnv env = outer_env;
  env.local_consts = &locals;

  switch (n->kind) {
    case NK_INT_LIT:
    case NK_CHAR_LIT:
      return eval_impl(n, env);

    case NK_VAR:
      return eval_impl(n, env);

    case NK_CAST: {
      auto r = interp_expr(n->left, locals, outer_env, consteval_fns, depth);
      if (!r.ok()) return r;
      return ConstEvalResult::success(apply_integer_cast(r.as_int(),
          resolve_type(n->type, env.type_bindings)));
    }

    case NK_UNARY: {
      auto r = interp_expr(n->left, locals, outer_env, consteval_fns, depth);
      if (!r.ok()) return r;
      if (!n->op) return ConstEvalResult::failure("unary operator missing");
      long long v = r.as_int();
      if (std::strcmp(n->op, "+") == 0) return ConstEvalResult::success(ConstValue::make_int(v));
      if (std::strcmp(n->op, "-") == 0) return ConstEvalResult::success(ConstValue::make_int(-v));
      if (std::strcmp(n->op, "~") == 0) return ConstEvalResult::success(ConstValue::make_int(~v));
      if (std::strcmp(n->op, "!") == 0) return ConstEvalResult::success(ConstValue::make_int(!v));
      return ConstEvalResult::failure(
          std::string("unsupported unary operator '") + n->op + "' in consteval context");
    }

    case NK_BINOP: {
      auto lr = interp_expr(n->left, locals, outer_env, consteval_fns, depth);
      auto rr = interp_expr(n->right, locals, outer_env, consteval_fns, depth);
      if (!lr.ok()) return lr;
      if (!rr.ok()) return rr;
      if (!n->op) return ConstEvalResult::failure("binary operator missing");
      long long l = lr.as_int(), r = rr.as_int();
      long long result;
      if (std::strcmp(n->op, "+") == 0) result = l + r;
      else if (std::strcmp(n->op, "-") == 0) result = l - r;
      else if (std::strcmp(n->op, "*") == 0) result = l * r;
      else if (std::strcmp(n->op, "/") == 0) result = (r == 0) ? 0LL : (l / r);
      else if (std::strcmp(n->op, "%") == 0) result = (r == 0) ? 0LL : (l % r);
      else if (std::strcmp(n->op, "<<") == 0) result = l << r;
      else if (std::strcmp(n->op, ">>") == 0) result = l >> r;
      else if (std::strcmp(n->op, "&") == 0) result = l & r;
      else if (std::strcmp(n->op, "|") == 0) result = l | r;
      else if (std::strcmp(n->op, "^") == 0) result = l ^ r;
      else if (std::strcmp(n->op, "<") == 0) result = static_cast<long long>(l < r);
      else if (std::strcmp(n->op, "<=") == 0) result = static_cast<long long>(l <= r);
      else if (std::strcmp(n->op, ">") == 0) result = static_cast<long long>(l > r);
      else if (std::strcmp(n->op, ">=") == 0) result = static_cast<long long>(l >= r);
      else if (std::strcmp(n->op, "==") == 0) result = static_cast<long long>(l == r);
      else if (std::strcmp(n->op, "!=") == 0) result = static_cast<long long>(l != r);
      else if (std::strcmp(n->op, "&&") == 0) result = static_cast<long long>(l && r);
      else if (std::strcmp(n->op, "||") == 0) result = static_cast<long long>(l || r);
      else return ConstEvalResult::failure(
          std::string("unsupported binary operator '") + n->op + "' in consteval context");
      return ConstEvalResult::success(ConstValue::make_int(result));
    }

    case NK_TERNARY: {
      auto cr = interp_expr(n->cond ? n->cond : n->left, locals, outer_env, consteval_fns, depth);
      if (!cr.ok()) return cr;
      return interp_expr(cr.as_int() ? n->then_ : n->else_, locals, outer_env, consteval_fns, depth);
    }

    case NK_CALL: {
      // Check if the callee is a consteval function we can interpret.
      if (!n->left || n->left->kind != NK_VAR || !n->left->name)
        return ConstEvalResult::failure("non-trivial callee in consteval context");
      auto it = consteval_fns.find(n->left->name);
      if (it == consteval_fns.end())
        return ConstEvalResult::failure(
            std::string("call to non-consteval function '") + n->left->name +
            "' is not allowed in constant evaluation");

      // Evaluate all arguments.
      std::vector<ConstValue> args;
      for (int i = 0; i < n->n_children; ++i) {
        auto r = interp_expr(n->children[i], locals, outer_env, consteval_fns, depth);
        if (!r.ok())
          return ConstEvalResult::failure(
              std::string("argument ") + std::to_string(i) +
              " of call to '" + n->left->name + "' is not a constant expression" +
              (r.error.empty() ? "" : ": " + r.error));
        args.push_back(*r.value);
      }

      return evaluate_consteval_call(it->second, args, outer_env, consteval_fns, depth + 1);
    }

    case NK_ASSIGN: {
      // left = right: evaluate right, assign to left (must be NK_VAR).
      if (!n->left || !n->right || n->left->kind != NK_VAR || !n->left->name)
        return ConstEvalResult::failure("unsupported assignment target in consteval context");
      auto r = interp_expr(n->right, locals, outer_env, consteval_fns, depth);
      if (!r.ok()) return r;
      locals[n->left->name] = r.as_int();
      return r;
    }

    case NK_COMMA_EXPR: {
      // left, right — evaluate both, return right.
      interp_expr(n->left, locals, outer_env, consteval_fns, depth);
      return interp_expr(n->right, locals, outer_env, consteval_fns, depth);
    }

    case NK_SIZEOF_TYPE:
      return compute_sizeof_type(resolve_type(n->type, env.type_bindings));
    case NK_SIZEOF_EXPR:
      if (n->left) return compute_sizeof_type(resolve_type(n->left->type, env.type_bindings));
      return ConstEvalResult::failure("sizeof(expr): missing expression");
    case NK_ALIGNOF_TYPE:
      return compute_alignof_type(resolve_type(n->type, env.type_bindings));
    case NK_ALIGNOF_EXPR:
      if (n->left) return compute_alignof_type(resolve_type(n->left->type, env.type_bindings));
      return ConstEvalResult::failure("alignof(expr): missing expression");

    default:
      return ConstEvalResult::failure(
          std::string("unsupported expression kind (NK=") +
          std::to_string(static_cast<int>(n->kind)) + ") in consteval body");
  }
}

StmtResult interp_block(const Node* block, ConstMap& locals,
                        const ConstEvalEnv& outer_env,
                        const std::unordered_map<std::string, const Node*>& consteval_fns,
                        int depth, int& steps) {
  if (!block) return {};
  if (block->kind == NK_BLOCK) {
    for (int i = 0; i < block->n_children; ++i) {
      auto r = interp_stmt(block->children[i], locals, outer_env, consteval_fns, depth, steps);
      if (r.returned || r.did_break || r.did_continue) return r;
    }
    return {};
  }
  // Single statement (not wrapped in block).
  return interp_stmt(block, locals, outer_env, consteval_fns, depth, steps);
}

StmtResult interp_stmt(const Node* n, ConstMap& locals,
                        const ConstEvalEnv& outer_env,
                        const std::unordered_map<std::string, const Node*>& consteval_fns,
                        int depth, int& steps) {
  if (!n) return {};
  if (++steps > kMaxConstevalSteps)
    return {true, false, false, ConstValue::unknown(), "step limit exceeded in consteval evaluation"};

  switch (n->kind) {
    case NK_RETURN: {
      if (!n->left) return {true, false, false, ConstValue::make_int(0)};
      auto r = interp_expr(n->left, locals, outer_env, consteval_fns, depth);
      if (!r.ok())
        return {true, false, false, ConstValue::unknown(),
                "return expression is not a constant expression" +
                (r.error.empty() ? std::string{} : ": " + r.error)};
      return {true, false, false, *r.value};
    }

    case NK_DECL: {
      // Local variable declaration — mutable or const with initializer.
      if (n->name && n->init) {
        auto r = interp_expr(n->init, locals, outer_env, consteval_fns, depth);
        if (r.ok()) {
          locals[n->name] = r.as_int();
        } else {
          return {true, false, false, ConstValue::unknown(),
                  std::string("initializer of '") + n->name +
                  "' is not a constant expression" +
                  (r.error.empty() ? std::string{} : ": " + r.error)};
        }
      } else if (n->name) {
        // Declaration without initializer — default to 0.
        locals[n->name] = 0;
      }
      return {};
    }

    case NK_IF: {
      auto cr = interp_expr(n->cond, locals, outer_env, consteval_fns, depth);
      if (!cr.ok())
        return {true, false, false, ConstValue::unknown(),
                "if condition is not a constant expression" +
                (cr.error.empty() ? std::string{} : ": " + cr.error)};
      if (cr.as_int()) {
        return interp_block(n->then_, locals, outer_env, consteval_fns, depth, steps);
      } else if (n->else_) {
        return interp_block(n->else_, locals, outer_env, consteval_fns, depth, steps);
      }
      return {};
    }

    case NK_WHILE: {
      while (true) {
        auto cr = interp_expr(n->cond, locals, outer_env, consteval_fns, depth);
        if (!cr.ok())
          return {true, false, false, ConstValue::unknown(),
                  "while condition is not a constant expression" +
                  (cr.error.empty() ? std::string{} : ": " + cr.error)};
        if (!cr.as_int()) break;
        auto r = interp_block(n->body, locals, outer_env, consteval_fns, depth, steps);
        if (r.returned) return r;
        if (r.did_break) break;
        // did_continue: just continue the loop
        if (++steps > kMaxConstevalSteps)
          return {true, false, false, ConstValue::unknown(), "step limit exceeded in consteval evaluation"};
      }
      return {};
    }

    case NK_FOR: {
      // init
      if (n->init) {
        auto r = interp_stmt(n->init, locals, outer_env, consteval_fns, depth, steps);
        if (r.returned) return r;
      }
      while (true) {
        // cond
        if (n->cond) {
          auto cr = interp_expr(n->cond, locals, outer_env, consteval_fns, depth);
          if (!cr.ok())
            return {true, false, false, ConstValue::unknown(),
                    "for condition is not a constant expression" +
                    (cr.error.empty() ? std::string{} : ": " + cr.error)};
          if (!cr.as_int()) break;
        }
        // body
        auto r = interp_block(n->body, locals, outer_env, consteval_fns, depth, steps);
        if (r.returned) return r;
        if (r.did_break) break;
        // update
        if (n->update) {
          interp_expr(n->update, locals, outer_env, consteval_fns, depth);
        }
        if (++steps > kMaxConstevalSteps)
          return {true, false, false, ConstValue::unknown(), "step limit exceeded in consteval evaluation"};
      }
      return {};
    }

    case NK_DO_WHILE: {
      do {
        auto r = interp_block(n->body, locals, outer_env, consteval_fns, depth, steps);
        if (r.returned) return r;
        if (r.did_break) break;
        auto cr = interp_expr(n->cond, locals, outer_env, consteval_fns, depth);
        if (!cr.ok())
          return {true, false, false, ConstValue::unknown(),
                  "do-while condition is not a constant expression" +
                  (cr.error.empty() ? std::string{} : ": " + cr.error)};
        if (!cr.as_int()) break;
        if (++steps > kMaxConstevalSteps)
          return {true, false, false, ConstValue::unknown(), "step limit exceeded in consteval evaluation"};
      } while (true);
      return {};
    }

    case NK_EXPR_STMT: {
      // Expression statement: evaluate the expression for side effects.
      if (n->left) {
        interp_expr(n->left, locals, outer_env, consteval_fns, depth);
      }
      return {};
    }

    case NK_BREAK:
      return {false, true, false, ConstValue::unknown()};

    case NK_CONTINUE:
      return {false, false, true, ConstValue::unknown()};

    case NK_BLOCK:
      return interp_block(n, locals, outer_env, consteval_fns, depth, steps);

    default:
      // Expression used as statement (e.g., assignment in for-init) — evaluate for side effects.
      interp_expr(n, locals, outer_env, consteval_fns, depth);
      return {};
  }
}

}  // namespace

ConstEvalResult evaluate_consteval_call(
    const Node* func_def,
    const std::vector<ConstValue>& args,
    const ConstEvalEnv& env,
    const std::unordered_map<std::string, const Node*>& consteval_fns,
    int depth) {
  if (!func_def || func_def->kind != NK_FUNCTION)
    return ConstEvalResult::failure("consteval target is not a function definition");
  if (depth > kMaxConstevalDepth)
    return ConstEvalResult::failure("consteval recursion depth limit exceeded");

  // Build local bindings from parameters.
  ConstMap locals;
  const int n_params = func_def->n_params;
  for (int i = 0; i < n_params && i < static_cast<int>(args.size()); ++i) {
    const Node* p = func_def->params[i];
    if (p && p->name && args[i].is_known()) {
      locals[p->name] = args[i].as_int();
    }
  }

  // Interpret the body.
  int steps = 0;
  auto result = interp_block(func_def->body, locals, env, consteval_fns, depth, steps);
  if (result.returned && result.return_val.is_known()) {
    return ConstEvalResult::success(result.return_val);
  }
  // Propagate the error from the statement result if available.
  std::string err = result.error;
  if (err.empty()) {
    if (!result.returned)
      err = "consteval function did not return a value";
    else
      err = "consteval function returned a non-constant value";
  }
  const char* fname = func_def->name ? func_def->name : "<anonymous>";
  return ConstEvalResult::failure(
      std::string("in consteval function '") + fname + "': " + err);
}

// ── Legacy API (thin wrapper) ────────────────────────────────────────────────

std::optional<long long> eval_int_const_expr(
    const Node* n,
    const std::unordered_map<std::string, long long>& enum_consts,
    const std::unordered_map<std::string, long long>* named_consts) {
  ConstEvalEnv env;
  env.enum_consts = &enum_consts;
  env.named_consts = named_consts;
  auto result = evaluate_constant_expr(n, env);
  return result.as_optional_int();
}

// ── String literal helpers ───────────────────────────────────────────────────

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

std::string bytes_from_string_literal(const StringLiteral& sl) {
  std::string bytes = sl.raw;
  if (bytes.size() >= 2 && bytes.front() == '"' && bytes.back() == '"') {
    bytes = bytes.substr(1, bytes.size() - 2);
  } else if (bytes.size() >= 3 && bytes[0] == 'L' && bytes[1] == '"' && bytes.back() == '"') {
    bytes = bytes.substr(2, bytes.size() - 3);
  }
  return decode_c_escaped_bytes_local(bytes);
}

}  // namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir
