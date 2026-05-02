// Core expression lowering implementation. Specialized expression helpers live
// beside this file under impl/expr/.

#include "expr.hpp"
#include <cctype>
#include <sstream>
#include <string_view>

namespace c4c::hir {

namespace {

template <typename T>
auto typespec_legacy_tag_if_present(const T& ts, int)
    -> decltype(ts.tag, std::string_view{}) {
  return ts.tag ? std::string_view(ts.tag) : std::string_view{};
}

std::string_view typespec_legacy_tag_if_present(const TypeSpec&, long) {
  return {};
}

void apply_lvalue_template_concrete(TypeSpec& target, const TypeSpec& concrete) {
  const int outer_ptr_level = target.ptr_level;
  const bool outer_lref = target.is_lvalue_ref;
  const bool outer_rref = target.is_rvalue_ref;
  const bool outer_const = target.is_const;
  const bool outer_volatile = target.is_volatile;

  target = concrete;
  target.ptr_level += outer_ptr_level;
  target.is_lvalue_ref = target.is_lvalue_ref || outer_lref;
  target.is_rvalue_ref =
      !target.is_lvalue_ref && (target.is_rvalue_ref || outer_rref);
  target.is_const = target.is_const || outer_const;
  target.is_volatile = target.is_volatile || outer_volatile;
}

bool apply_lvalue_template_binding_by_text(
    TypeSpec& ts,
    const std::unordered_map<TextId, TypeSpec>& tpl_bindings_by_text) {
  if (ts.template_param_text_id != kInvalidText) {
    auto it = tpl_bindings_by_text.find(ts.template_param_text_id);
    if (it != tpl_bindings_by_text.end()) {
      apply_lvalue_template_concrete(ts, it->second);
      return true;
    }
  }
  if (ts.tag_text_id != kInvalidText) {
    auto it = tpl_bindings_by_text.find(ts.tag_text_id);
    if (it != tpl_bindings_by_text.end()) {
      apply_lvalue_template_concrete(ts, it->second);
      return true;
    }
  }
  return false;
}

bool apply_lvalue_template_binding_by_compatibility_tag(
    TypeSpec& ts,
    const TypeBindings& tpl_bindings) {
  const std::string_view legacy_tag = typespec_legacy_tag_if_present(ts, 0);
  if (legacy_tag.empty()) return false;
  auto it = tpl_bindings.find(std::string(legacy_tag));
  if (it == tpl_bindings.end()) return false;
  apply_lvalue_template_concrete(ts, it->second);
  return true;
}

bool has_lvalue_template_binding_text_carrier(const TypeSpec& ts) {
  return ts.template_param_text_id != kInvalidText ||
         ts.tag_text_id != kInvalidText;
}

}  // namespace

bool Lowerer::is_ast_lvalue(const Node* n, const FunctionCtx* ctx) {
  if (!n) return false;
  switch (n->kind) {
    case NK_VAR:
    case NK_INDEX:
    case NK_DEREF:
    case NK_ASSIGN:
    case NK_COMPOUND_ASSIGN:
      return true;
    case NK_MEMBER:
      return n->is_arrow || is_ast_lvalue(n->left, ctx);
    case NK_CAST: {
      TypeSpec cast_ts = n->type;
      if (ctx && cast_ts.base == TB_TYPEDEF) {
        if (!apply_lvalue_template_binding_by_text(
                cast_ts, ctx->tpl_bindings_by_text) &&
            !has_lvalue_template_binding_text_carrier(cast_ts)) {
          apply_lvalue_template_binding_by_compatibility_tag(
              cast_ts, ctx->tpl_bindings);
        }
      }
      return cast_ts.is_lvalue_ref;
    }
    default:
      return false;
  }
}

ExprId Lowerer::lower_expr(FunctionCtx* ctx, const Node* n) {
  if (!n) {
    TypeSpec ts{};
    ts.base = TB_INT;
    return append_expr(nullptr, IntLiteral{0, false}, ts);
  }

  switch (n->kind) {
    case NK_INT_LIT: {
      if (n->name && n->name[0]) {
        LabelAddrExpr la{};
        la.label_name = n->name;
        la.fn_name = ctx ? ctx->fn->name : "";
        la.fn_link_name_id = ctx ? ctx->fn->link_name_id : kInvalidLinkName;
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
      TypeSpec ts = n->type;
      ts.base = TB_INT;
      return append_expr(n, CharLiteral{n->ival}, ts);
    }
    case NK_PACK_EXPANSION:
      return lower_expr(ctx, n->left);
    case NK_VAR:
      return lower_var_expr(ctx, n);
    case NK_UNARY:
      return lower_unary_expr(ctx, n);
    case NK_POSTFIX:
      return lower_postfix_expr(ctx, n);
    case NK_ADDR:
      return lower_addr_expr(ctx, n);
    case NK_DEREF:
      return lower_deref_expr(ctx, n);
    case NK_COMMA_EXPR:
      return lower_comma_expr(ctx, n);
    case NK_BINOP:
      return lower_binary_expr(ctx, n);
    case NK_ASSIGN:
      return lower_assign_expr(ctx, n);
    case NK_COMPOUND_ASSIGN:
      return lower_compound_assign_expr(ctx, n);
    case NK_CAST:
      return lower_cast_expr(ctx, n);
    case NK_CALL:
    case NK_BUILTIN_CALL:
      return lower_call_expr(ctx, n);
    case NK_VA_ARG:
      return lower_va_arg_expr(ctx, n);
    case NK_INDEX:
      return lower_index_expr(ctx, n);
    case NK_MEMBER:
      return lower_member_expr(ctx, n);
    case NK_TERNARY:
      return lower_ternary_expr(ctx, n);
    case NK_GENERIC:
      return lower_generic_expr(ctx, n);
    case NK_STMT_EXPR:
      return lower_stmt_expr(ctx, n);
    case NK_REAL_PART:
    case NK_IMAG_PART:
      return lower_complex_part_expr(ctx, n);
    case NK_SIZEOF_EXPR:
      return lower_sizeof_expr(ctx, n);
    case NK_SIZEOF_PACK:
      return lower_sizeof_pack_expr(ctx, n);
    case NK_SIZEOF_TYPE:
      return lower_builtin_sizeof_type(ctx, n);
    case NK_ALIGNOF_TYPE:
      return lower_builtin_alignof_type(ctx, n);
    case NK_ALIGNOF_EXPR:
      return lower_builtin_alignof_expr(ctx, n);
    case NK_COMPOUND_LIT:
      return lower_compound_literal_expr(ctx, n);
    case NK_NEW_EXPR:
      return lower_new_expr(ctx, n);
    case NK_DELETE_EXPR:
      return lower_delete_expr(ctx, n);
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

}  // namespace c4c::hir
