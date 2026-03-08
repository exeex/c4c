#include "hir_printer.hpp"

#include <cassert>
#include <sstream>
#include <string>

#include "ast.hpp"  // TypeBase, TypeSpec

namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir {
namespace {

using namespace tinyc2ll::frontend_cxx;

// ── TypeSpec -> readable string ───────────────────────────────────────────────

static std::string ts_str(const TypeSpec& ts) {
  std::string s;
  if (ts.is_const) s += "const ";

  switch (ts.base) {
    case TB_VOID:          s += "void"; break;
    case TB_CHAR:          s += "char"; break;
    case TB_UCHAR:         s += "uchar"; break;
    case TB_SHORT:         s += "short"; break;
    case TB_USHORT:        s += "ushort"; break;
    case TB_INT:           s += "int"; break;
    case TB_UINT:          s += "uint"; break;
    case TB_LONG:          s += "long"; break;
    case TB_ULONG:         s += "ulong"; break;
    case TB_LONGLONG:      s += "llong"; break;
    case TB_ULONGLONG:     s += "ullong"; break;
    case TB_FLOAT:         s += "float"; break;
    case TB_DOUBLE:        s += "double"; break;
    case TB_LONGDOUBLE:    s += "ldouble"; break;
    case TB_BOOL:          s += "bool"; break;
    case TB_COMPLEX_FLOAT: s += "cfloat"; break;
    case TB_COMPLEX_DOUBLE:s += "cdouble"; break;
    case TB_STRUCT:        s += ts.tag ? std::string("struct ") + ts.tag : "struct<?>"; break;
    case TB_UNION:         s += ts.tag ? std::string("union ") + ts.tag : "union<?>"; break;
    case TB_ENUM:          s += ts.tag ? std::string("enum ") + ts.tag : "enum<?>"; break;
    case TB_FUNC_PTR:      s += "fn"; break;
    default:               s += "?"; break;
  }

  for (int i = 0; i < ts.ptr_level; ++i) s += "*";

  if (ts.array_rank > 0) {
    for (int i = 0; i < ts.array_rank; ++i) {
      s += "[";
      const long long dim = (i == 0) ? ts.array_size : ts.array_dims[i];
      if (dim >= 0) s += std::to_string(dim);
      s += "]";
    }
  }
  return s;
}

// ── Expr pretty-print ─────────────────────────────────────────────────────────

static std::string unary_op_str(UnaryOp op) {
  switch (op) {
    case UnaryOp::Plus:    return "+";
    case UnaryOp::Minus:   return "-";
    case UnaryOp::Not:     return "!";
    case UnaryOp::BitNot:  return "~";
    case UnaryOp::AddrOf:  return "&";
    case UnaryOp::Deref:   return "*";
    case UnaryOp::PreInc:  return "++";
    case UnaryOp::PreDec:  return "--";
    case UnaryOp::PostInc: return "post++";
    case UnaryOp::PostDec: return "post--";
    case UnaryOp::RealPart: return "__real__";
    case UnaryOp::ImagPart: return "__imag__";
  }
  return "?";
}

static std::string binary_op_str(BinaryOp op) {
  switch (op) {
    case BinaryOp::Add:    return "+";
    case BinaryOp::Sub:    return "-";
    case BinaryOp::Mul:    return "*";
    case BinaryOp::Div:    return "/";
    case BinaryOp::Mod:    return "%";
    case BinaryOp::Shl:    return "<<";
    case BinaryOp::Shr:    return ">>";
    case BinaryOp::BitAnd: return "&";
    case BinaryOp::BitOr:  return "|";
    case BinaryOp::BitXor: return "^";
    case BinaryOp::Lt:     return "<";
    case BinaryOp::Le:     return "<=";
    case BinaryOp::Gt:     return ">";
    case BinaryOp::Ge:     return ">=";
    case BinaryOp::Eq:     return "==";
    case BinaryOp::Ne:     return "!=";
    case BinaryOp::LAnd:   return "&&";
    case BinaryOp::LOr:    return "||";
    case BinaryOp::Comma:  return ",";
  }
  return "?";
}

static std::string assign_op_str(AssignOp op) {
  switch (op) {
    case AssignOp::Set:    return "=";
    case AssignOp::Add:    return "+=";
    case AssignOp::Sub:    return "-=";
    case AssignOp::Mul:    return "*=";
    case AssignOp::Div:    return "/=";
    case AssignOp::Mod:    return "%=";
    case AssignOp::Shl:    return "<<=";
    case AssignOp::Shr:    return ">>=";
    case AssignOp::BitAnd: return "&=";
    case AssignOp::BitOr:  return "|=";
    case AssignOp::BitXor: return "^=";
  }
  return "?";
}

class Printer {
 public:
  explicit Printer(const Module& m) : m_(m) {}

  std::string run() {
    std::ostringstream out;

    out << "=== HIR Module ===\n";
    const auto s = m_.summary();
    out << "functions=" << s.functions
        << " globals=" << s.globals
        << " blocks=" << s.blocks
        << " stmts=" << s.statements
        << " exprs=" << s.expressions << "\n";

    if (!m_.globals.empty()) {
      out << "\n--- globals ---\n";
      for (const auto& g : m_.globals) {
        print_global(out, g);
      }
    }

    out << "\n--- functions ---\n";
    for (const auto& fn : m_.functions) {
      print_function(out, fn);
    }
    return out.str();
  }

 private:
  const Module& m_;

  void print_global(std::ostringstream& out, const GlobalVar& g) {
    out << "  global " << g.name << ": " << ts_str(g.type.spec);
    if (g.linkage.is_static) out << " static";
    if (g.linkage.is_extern) out << " extern";
    if (g.is_const) out << " const";
    out << "\n";
  }

  void print_function(std::ostringstream& out, const Function& fn) {
    out << "\nfn " << fn.name << "(";
    for (size_t i = 0; i < fn.params.size(); ++i) {
      if (i) out << ", ";
      out << fn.params[i].name << ": " << ts_str(fn.params[i].type.spec);
    }
    out << ") -> " << ts_str(fn.return_type.spec);
    if (fn.attrs.variadic) out << " variadic";
    if (fn.linkage.is_static) out << " static";
    if (fn.linkage.is_extern) out << " extern";
    out << "  [entry: block#" << fn.entry.value << "]\n";

    for (const auto& blk : fn.blocks) {
      print_block(out, blk);
    }
  }

  void print_block(std::ostringstream& out, const Block& blk) {
    out << "  block#" << blk.id.value;
    if (blk.has_explicit_terminator) out << " [term]";
    out << ":\n";
    for (const auto& stmt : blk.stmts) {
      print_stmt(out, stmt);
    }
  }

  void print_stmt(std::ostringstream& out, const Stmt& stmt) {
    out << "    ";
    std::visit(
        [&](const auto& s) { print_stmt_payload(out, s); },
        stmt.payload);
    out << "\n";
  }

  void print_stmt_payload(std::ostringstream& out, const LocalDecl& d) {
    out << "decl " << d.name << ": " << ts_str(d.type.spec);
    if (d.storage == StorageClass::Static) out << " static";
    if (d.init) {
      out << " = ";
      print_expr_inline(out, *d.init);
    }
  }

  void print_stmt_payload(std::ostringstream& out, const ExprStmt& s) {
    if (s.expr) {
      print_expr_inline(out, *s.expr);
    } else {
      out << "(empty)";
    }
  }

  void print_stmt_payload(std::ostringstream& out, const ReturnStmt& s) {
    out << "return";
    if (s.expr) {
      out << " ";
      print_expr_inline(out, *s.expr);
    }
  }

  void print_stmt_payload(std::ostringstream& out, const IfStmt& s) {
    out << "if (";
    print_expr_inline(out, s.cond);
    out << ") -> block#" << s.then_block.value;
    if (s.else_block) out << " else block#" << s.else_block->value;
  }

  void print_stmt_payload(std::ostringstream& out, const WhileStmt& s) {
    out << "while (";
    print_expr_inline(out, s.cond);
    out << ") body=block#" << s.body_block.value;
  }

  void print_stmt_payload(std::ostringstream& out, const ForStmt& s) {
    out << "for (";
    if (s.init) { print_expr_inline(out, *s.init); }
    out << "; ";
    if (s.cond) { print_expr_inline(out, *s.cond); }
    out << "; ";
    if (s.update) { print_expr_inline(out, *s.update); }
    out << ") body=block#" << s.body_block.value;
  }

  void print_stmt_payload(std::ostringstream& out, const DoWhileStmt& s) {
    out << "do block#" << s.body_block.value << " while (";
    print_expr_inline(out, s.cond);
    out << ")";
  }

  void print_stmt_payload(std::ostringstream& out, const SwitchStmt& s) {
    out << "switch (";
    print_expr_inline(out, s.cond);
    out << ") body=block#" << s.body_block.value;
  }

  void print_stmt_payload(std::ostringstream& out, const GotoStmt& s) {
    out << "goto " << s.target.user_name << " -> block#" << s.target.resolved_block.value;
  }

  void print_stmt_payload(std::ostringstream& out, const LabelStmt& s) {
    out << "label " << s.name << ":";
  }

  void print_stmt_payload(std::ostringstream& out, const CaseStmt& s) {
    out << "case " << s.value << ":";
  }

  void print_stmt_payload(std::ostringstream& out, const CaseRangeStmt& s) {
    out << "case " << s.range.lo << ".." << s.range.hi << ":";
  }

  void print_stmt_payload(std::ostringstream& out, const DefaultStmt&) {
    out << "default:";
  }

  void print_stmt_payload(std::ostringstream& out, const BreakStmt&) {
    out << "break";
  }

  void print_stmt_payload(std::ostringstream& out, const ContinueStmt&) {
    out << "continue";
  }

  // ── Expr inline print ───────────────────────────────────────────────────────

  const Expr* find_expr(ExprId id) const {
    for (const auto& e : m_.expr_pool) {
      if (e.id.value == id.value) return &e;
    }
    return nullptr;
  }

  void print_expr_inline(std::ostringstream& out, ExprId id) {
    const Expr* e = find_expr(id);
    if (!e) {
      out << "expr#" << id.value;
      return;
    }
    std::visit([&](const auto& p) { print_expr_payload(out, p); }, e->payload);
  }

  void print_expr_payload(std::ostringstream& out, const IntLiteral& x) {
    out << x.value;
    if (x.is_unsigned) out << "u";
  }

  void print_expr_payload(std::ostringstream& out, const FloatLiteral& x) {
    out << x.value;
  }

  void print_expr_payload(std::ostringstream& out, const StringLiteral& x) {
    out << (x.is_wide ? "L\"" : "\"");
    for (char c : x.raw) {
      if (c == '"') out << "\\\"";
      else if (c == '\n') out << "\\n";
      else out << c;
    }
    out << "\"";
  }

  void print_expr_payload(std::ostringstream& out, const CharLiteral& x) {
    out << "'" << static_cast<char>(x.value) << "'";
  }

  void print_expr_payload(std::ostringstream& out, const DeclRef& x) {
    out << x.name;
    if (x.local) out << "#L" << x.local->value;
    else if (x.param_index) out << "#P" << *x.param_index;
    else if (x.global) out << "#G" << x.global->value;
  }

  void print_expr_payload(std::ostringstream& out, const UnaryExpr& x) {
    const std::string op = unary_op_str(x.op);
    if (x.op == UnaryOp::PostInc || x.op == UnaryOp::PostDec) {
      out << "(";
      print_expr_inline(out, x.operand);
      out << op << ")";
    } else {
      out << "(" << op;
      print_expr_inline(out, x.operand);
      out << ")";
    }
  }

  void print_expr_payload(std::ostringstream& out, const BinaryExpr& x) {
    out << "(";
    print_expr_inline(out, x.lhs);
    out << " " << binary_op_str(x.op) << " ";
    print_expr_inline(out, x.rhs);
    out << ")";
  }

  void print_expr_payload(std::ostringstream& out, const AssignExpr& x) {
    out << "(";
    print_expr_inline(out, x.lhs);
    out << " " << assign_op_str(x.op) << " ";
    print_expr_inline(out, x.rhs);
    out << ")";
  }

  void print_expr_payload(std::ostringstream& out, const CastExpr& x) {
    out << "((" << ts_str(x.to_type.spec) << ")";
    print_expr_inline(out, x.expr);
    out << ")";
  }

  void print_expr_payload(std::ostringstream& out, const CallExpr& x) {
    print_expr_inline(out, x.callee);
    out << "(";
    for (size_t i = 0; i < x.args.size(); ++i) {
      if (i) out << ", ";
      print_expr_inline(out, x.args[i]);
    }
    out << ")";
  }

  void print_expr_payload(std::ostringstream& out, const IndexExpr& x) {
    print_expr_inline(out, x.base);
    out << "[";
    print_expr_inline(out, x.index);
    out << "]";
  }

  void print_expr_payload(std::ostringstream& out, const MemberExpr& x) {
    print_expr_inline(out, x.base);
    out << (x.is_arrow ? "->" : ".") << x.field;
  }

  void print_expr_payload(std::ostringstream& out, const TernaryExpr& x) {
    out << "(";
    print_expr_inline(out, x.cond);
    out << " ? ";
    print_expr_inline(out, x.then_expr);
    out << " : ";
    print_expr_inline(out, x.else_expr);
    out << ")";
  }

  void print_expr_payload(std::ostringstream& out, const SizeofExpr& x) {
    out << "sizeof(";
    print_expr_inline(out, x.expr);
    out << ")";
  }

  void print_expr_payload(std::ostringstream& out, const SizeofTypeExpr& x) {
    out << "sizeof(" << ts_str(x.type.spec) << ")";
  }
};

}  // namespace

std::string format_hir(const Module& module) {
  Printer p(module);
  return p.run();
}

}  // namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir
