#include "expr.hpp"

#include <algorithm>

namespace c4c::hir {

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
      natural = struct_align_bytes(ts.tag);
    } else {
      natural = std::max(1, static_cast<int>(alignof_type_spec(ts)));
    }
    if (ts.align_bytes > 0) natural = std::max(natural, ts.align_bytes);
    return natural;
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

}  // namespace

TypeSpec Lowerer::builtin_query_result_type() const {
  TypeSpec ts{};
  ts.base = TB_ULONG;
  return ts;
}

TypeSpec Lowerer::resolve_builtin_query_type(FunctionCtx* ctx, TypeSpec target) const {
  if (!ctx || ctx->tpl_bindings.empty() || target.base != TB_TYPEDEF || !target.tag) {
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
    DeclRef ref{};
    ref.name = expr->name;
    ref.name_text_id = make_unqualified_text_id(
        expr, module_ ? module_->link_name_texts.get() : nullptr);
    ref.ns_qual = make_ns_qual(
        expr, module_ ? module_->link_name_texts.get() : nullptr);
    attach_decl_ref_link_name_id(ref);
    if (const Function* fn = module_->resolve_function_decl(ref)) {
      int fn_align = fn->attrs.align_bytes;
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

}  // namespace c4c::hir
