// Draft-only HIR expression lowering split extracted from hir_lowering_core.cpp.
// This file is not yet wired into the build and is intended as a staging
// artifact for the eventual multi-translation-unit split.

#include "hir_lowering.hpp"
#include "hir_lowerer_internal.hpp"
#include "consteval.hpp"

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

#include "type_utils.hpp"
#include "../parser/parser.hpp"

namespace c4c::hir {

namespace {

const HirStructField* find_struct_instance_field_including_bases(
    const hir::Module& module, const std::string& tag, const std::string& field) {
  auto sit = module.struct_defs.find(tag);
  if (sit == module.struct_defs.end()) return nullptr;
  for (const auto& fld : sit->second.fields) {
    if (fld.name == field) return &fld;
  }
  for (const auto& base_tag : sit->second.base_tags) {
    if (const HirStructField* inherited =
            find_struct_instance_field_including_bases(module, base_tag, field)) {
      return inherited;
    }
  }
  return nullptr;
}

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
      if (ctx && cast_ts.base == TB_TYPEDEF && cast_ts.tag) {
        const int outer_ptr_level = cast_ts.ptr_level;
        const bool outer_lref = cast_ts.is_lvalue_ref;
        const bool outer_rref = cast_ts.is_rvalue_ref;
        const bool outer_const = cast_ts.is_const;
        const bool outer_volatile = cast_ts.is_volatile;
        auto it = ctx->tpl_bindings.find(cast_ts.tag);
        if (it != ctx->tpl_bindings.end()) {
          cast_ts = it->second;
          cast_ts.ptr_level += outer_ptr_level;
          cast_ts.is_lvalue_ref = cast_ts.is_lvalue_ref || outer_lref;
          cast_ts.is_rvalue_ref =
              !cast_ts.is_lvalue_ref && (cast_ts.is_rvalue_ref || outer_rref);
          cast_ts.is_const = cast_ts.is_const || outer_const;
          cast_ts.is_volatile = cast_ts.is_volatile || outer_volatile;
        }
      }
      return cast_ts.is_lvalue_ref;
    }
    default:
      return false;
  }
}

std::optional<ExprId> Lowerer::try_lower_rvalue_ref_storage_addr(
    FunctionCtx* ctx,
    const Node* n,
    const TypeSpec& storage_ts) {
  if (!ctx || !n) return std::nullopt;

  ExprId storage_expr{};
  switch (n->kind) {
    case NK_CAST:
      if (!n->type.is_rvalue_ref || !n->left) return std::nullopt;
      storage_expr = lower_expr(ctx, n->left);
      break;
    case NK_MEMBER:
      if (n->is_arrow || !n->left || is_ast_lvalue(n->left, ctx))
        return std::nullopt;
      storage_expr = lower_expr(ctx, n);
      break;
    default:
      return std::nullopt;
  }

  UnaryExpr addr{};
  addr.op = UnaryOp::AddrOf;
  addr.operand = storage_expr;
  return append_expr(n, addr, storage_ts);
}

std::string Lowerer::resolve_ref_overload(const std::string& base_name,
                                          const Node* call_node,
                                          const FunctionCtx* ctx) {
  auto expr_is_lvalue = [&](const Node* expr) -> bool {
    if (is_ast_lvalue(expr, ctx)) return true;
    if (auto ts = infer_call_result_type(ctx, expr)) return ts->is_lvalue_ref;
    return false;
  };
  auto ovit = ref_overload_set_.find(base_name);
  if (ovit == ref_overload_set_.end()) return {};
  const auto& overloads = ovit->second;
  const Node* object_node =
      (call_node && call_node->left && call_node->left->kind == NK_MEMBER)
          ? call_node->left->left
          : nullptr;
  const bool object_is_lvalue = object_node && expr_is_lvalue(object_node);
  const std::string* best_name = nullptr;
  int best_score = -1;
  for (const auto& ov : overloads) {
    bool viable = true;
    int score = 0;
    if (object_node) {
      if (ov.method_is_lvalue_ref && !object_is_lvalue) viable = false;
      if (ov.method_is_rvalue_ref && object_is_lvalue) viable = false;
      if (!viable) continue;
      if (ov.method_is_rvalue_ref && !object_is_lvalue) score += 2;
      else if (ov.method_is_lvalue_ref && object_is_lvalue) score += 2;
      else score += 1;
    }
    for (int i = 0;
         i < call_node->n_children &&
         i < static_cast<int>(ov.param_is_rvalue_ref.size());
         ++i) {
      bool arg_is_lvalue = expr_is_lvalue(call_node->children[i]);
      if (ov.param_is_lvalue_ref[static_cast<size_t>(i)] && !arg_is_lvalue) {
        viable = false;
        break;
      }
      if (ov.param_is_rvalue_ref[static_cast<size_t>(i)] && arg_is_lvalue) {
        viable = false;
        break;
      }
      if (ov.param_is_rvalue_ref[static_cast<size_t>(i)] && !arg_is_lvalue) {
        score += 2;
      } else if (ov.param_is_lvalue_ref[static_cast<size_t>(i)] && arg_is_lvalue) {
        score += 2;
      } else {
        score += 1;
      }
    }
    if (viable && score > best_score) {
      best_name = &ov.mangled_name;
      best_score = score;
    }
  }
  return best_name ? *best_name : base_name;
}

const Node* Lowerer::find_pending_method_by_mangled(
    const std::string& mangled_name) const {
  for (const auto& pm : pending_methods_) {
    if (pm.mangled == mangled_name) return pm.method_node;
  }
  return nullptr;
}

ExprId Lowerer::try_lower_operator_call(FunctionCtx* ctx,
                                        const Node* result_node,
                                        const Node* obj_node,
                                        const char* op_method_name,
                                        const std::vector<const Node*>& arg_nodes,
                                        const std::vector<ExprId>& extra_args) {
  TypeSpec obj_ts = infer_generic_ctrl_type(ctx, obj_node);
  if (obj_ts.ptr_level != 0 || obj_ts.base != TB_STRUCT || !obj_ts.tag) {
    return ExprId::invalid();
  }

  if (std::strcmp(op_method_name, "operator_call") == 0 && arg_nodes.empty()) {
    if (auto value = find_struct_static_member_const_value(obj_ts.tag, "value")) {
      TypeSpec value_ts{};
      if (const Node* decl = find_struct_static_member_decl(obj_ts.tag, "value")) {
        value_ts = decl->type;
      } else {
        value_ts.base = TB_BOOL;
      }
      if (value_ts.base == TB_VOID) value_ts.base = TB_INT;
      return append_expr(result_node, IntLiteral{*value, false}, value_ts);
    }
  }

  std::optional<std::string> resolved =
      find_struct_method_mangled(obj_ts.tag, op_method_name, obj_ts.is_const);
  if (!resolved) return ExprId::invalid();

  std::string resolved_mangled = *resolved;
  std::string base_key = std::string(obj_ts.tag) + "::" + op_method_name;
  auto ovit = ref_overload_set_.find(base_key);
  if (ovit != ref_overload_set_.end() && !ovit->second.empty()) {
    const auto& overloads = ovit->second;
    auto expr_is_lvalue = [&](const Node* expr) -> bool {
      if (is_ast_lvalue(expr, ctx)) return true;
      if (auto ts = infer_call_result_type(ctx, expr)) return ts->is_lvalue_ref;
      return false;
    };
    const bool object_is_lvalue = expr_is_lvalue(obj_node);
    const std::string* best_name = nullptr;
    int best_score = -1;
    for (const auto& ov : overloads) {
      bool viable = true;
      int score = 0;
      if (ov.method_is_lvalue_ref && !object_is_lvalue) viable = false;
      if (ov.method_is_rvalue_ref && object_is_lvalue) viable = false;
      if (!viable) continue;
      if (ov.method_is_rvalue_ref && !object_is_lvalue) score += 2;
      else if (ov.method_is_lvalue_ref && object_is_lvalue) score += 2;
      else score += 1;
      for (size_t i = 0; i < arg_nodes.size() && i < ov.param_is_rvalue_ref.size(); ++i) {
        bool arg_is_lvalue = expr_is_lvalue(arg_nodes[i]);
        if (ov.param_is_lvalue_ref[i] && !arg_is_lvalue) {
          viable = false;
          break;
        }
        if (ov.param_is_rvalue_ref[i] && arg_is_lvalue) {
          viable = false;
          break;
        }
        if (ov.param_is_rvalue_ref[i] && !arg_is_lvalue) score += 2;
        else if (ov.param_is_lvalue_ref[i] && arg_is_lvalue) score += 2;
        else score += 1;
      }
      if (viable && score > best_score) {
        best_name = &ov.mangled_name;
        best_score = score;
      }
    }
    if (best_name) resolved_mangled = *best_name;
  }

  CallExpr c{};
  DeclRef dr{};
  dr.name = resolved_mangled;
  auto fit = module_->fn_index.find(dr.name);
  TypeSpec fn_ts{};
  fn_ts.base = TB_VOID;
  if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
    fn_ts = module_->functions[fit->second.value].return_type.spec;
  }
  if (fn_ts.base == TB_VOID) {
    if (auto rit = find_struct_method_return_type(obj_ts.tag, op_method_name, obj_ts.is_const)) {
      fn_ts = *rit;
    }
  }
  TypeSpec callee_ts = fn_ts;
  callee_ts.ptr_level++;
  c.callee = append_expr(result_node, dr, callee_ts);

  ExprId obj_id = lower_expr(ctx, obj_node);
  if (ctx && module_->expr_pool[obj_id.value].type.category != ValueCategory::LValue) {
    LocalDecl tmp{};
    tmp.id = next_local_id();
    std::string tmp_name = "__op_call_tmp_" + std::to_string(tmp.id.value);
    tmp.name = tmp_name;
    tmp.type = qtype_from(obj_ts, ValueCategory::RValue);
    const TypeSpec init_ts = module_->expr_pool[obj_id.value].type.spec;
    if (init_ts.base == obj_ts.base && init_ts.ptr_level == obj_ts.ptr_level &&
        init_ts.tag == obj_ts.tag) {
      tmp.init = obj_id;
    }
    const LocalId tmp_lid = tmp.id;
    append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(obj_node)});

    DeclRef tmp_ref{};
    tmp_ref.name = tmp_name;
    tmp_ref.local = tmp_lid;
    obj_id = append_expr(obj_node, tmp_ref, obj_ts, ValueCategory::LValue);
  }
  UnaryExpr addr{};
  addr.op = UnaryOp::AddrOf;
  addr.operand = obj_id;
  TypeSpec ptr_ts = obj_ts;
  ptr_ts.ptr_level++;
  c.args.push_back(append_expr(obj_node, addr, ptr_ts));

  const Function* callee_fn = nullptr;
  if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
    callee_fn = &module_->functions[fit->second.value];
  }
  const Node* method_ast = nullptr;
  if (!callee_fn) {
    for (const auto& pm : pending_methods_) {
      if (pm.mangled == resolved_mangled) {
        method_ast = pm.method_node;
        break;
      }
    }
  }

  auto lower_operator_ref_arg = [&](const Node* arg_node, const TypeSpec* param_ts) -> ExprId {
    if (!param_ts || (!param_ts->is_lvalue_ref && !param_ts->is_rvalue_ref)) {
      return lower_expr(ctx, arg_node);
    }
    if (auto ts = infer_call_result_type(ctx, arg_node)) {
      if (is_any_ref_ts(*ts)) return lower_expr(ctx, arg_node);
    }
    TypeSpec storage_ts = reference_storage_ts(*param_ts);
    if (param_ts->is_rvalue_ref) {
      if (auto storage_addr =
              try_lower_rvalue_ref_storage_addr(ctx, arg_node, storage_ts)) {
        return *storage_addr;
      }
      ExprId arg_val = lower_expr(ctx, arg_node);
      TypeSpec val_ts = reference_value_ts(*param_ts);
      resolve_typedef_to_struct(val_ts);
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
      UnaryExpr addr_e{};
      addr_e.op = UnaryOp::AddrOf;
      addr_e.operand = var_id;
      return append_expr(arg_node, addr_e, storage_ts);
    }
    UnaryExpr addr_e{};
    addr_e.op = UnaryOp::AddrOf;
    addr_e.operand = lower_expr(ctx, arg_node);
    return append_expr(arg_node, addr_e, storage_ts);
  };

  for (size_t i = 0; i < arg_nodes.size(); ++i) {
    const TypeSpec* param_ts = (callee_fn && (i + 1) < callee_fn->params.size())
                                   ? &callee_fn->params[i + 1].type.spec
                                   : nullptr;
    TypeSpec ast_param_ts{};
    if (!param_ts && method_ast && static_cast<int>(i) < method_ast->n_params) {
      ast_param_ts = method_ast->params[i]->type;
      param_ts = &ast_param_ts;
    }
    if (param_ts && (param_ts->is_rvalue_ref || param_ts->is_lvalue_ref)) {
      c.args.push_back(lower_operator_ref_arg(arg_nodes[i], param_ts));
    } else {
      c.args.push_back(lower_expr(ctx, arg_nodes[i]));
    }
  }
  for (auto ea : extra_args) c.args.push_back(ea);

  return append_expr(result_node, c, fn_ts);
}

ExprId Lowerer::lower_member_expr(FunctionCtx* ctx, const Node* n) {
  if (n->is_arrow && n->left) {
    ExprId arrow_ptr = try_lower_operator_call(ctx, n, n->left, "operator_arrow", {});
    if (arrow_ptr.valid()) {
      for (int depth = 0; depth < 8; ++depth) {
        const Expr* res_expr = module_->find_expr(arrow_ptr);
        if (!res_expr) break;
        TypeSpec rts = res_expr->type.spec;
        if (rts.ptr_level > 0) break;
        if (rts.base != TB_STRUCT || !rts.tag) break;
        std::string base_key = std::string(rts.tag) + "::operator_arrow";
        std::string const_key = base_key + "_const";
        auto mit = rts.is_const ? struct_methods_.find(const_key) : struct_methods_.find(base_key);
        if (mit == struct_methods_.end()) {
          mit = rts.is_const ? struct_methods_.find(base_key) : struct_methods_.find(const_key);
        }
        if (mit == struct_methods_.end()) break;
        auto fit = module_->fn_index.find(mit->second);
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
          fn_ts = module_->functions[fit->second.value].return_type.spec;
        }
        if (fn_ts.base == TB_VOID) {
          auto rit2 = struct_method_ret_types_.find(std::string(rts.tag) + "::operator_arrow");
          if (rit2 == struct_method_ret_types_.end()) {
            rit2 = struct_method_ret_types_.find(std::string(rts.tag) + "::operator_arrow_const");
          }
          if (rit2 != struct_method_ret_types_.end()) fn_ts = rit2->second;
        }
        LocalDecl tmp{};
        tmp.id = next_local_id();
        tmp.name = "__arrow_tmp";
        tmp.type = qtype_from(rts, ValueCategory::LValue);
        tmp.init = arrow_ptr;
        const LocalId tmp_lid = tmp.id;
        ctx->locals[tmp.name] = tmp.id;
        ctx->local_types[tmp.id.value] = rts;
        append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
        DeclRef tmp_ref{};
        tmp_ref.name = "__arrow_tmp";
        tmp_ref.local = tmp_lid;
        ExprId tmp_id = append_expr(n, tmp_ref, rts, ValueCategory::LValue);
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
  const ValueCategory category =
      (n->is_arrow || is_ast_lvalue(n->left, ctx))
          ? ValueCategory::LValue
          : ValueCategory::RValue;
  return append_expr(n, m, n->type, category);
}

ExprId Lowerer::maybe_bool_convert(FunctionCtx* ctx, ExprId expr, const Node* n) {
  if (!expr.valid() || !n) return expr;
  TypeSpec ts = infer_generic_ctrl_type(ctx, n);
  if (ts.ptr_level != 0 || ts.base != TB_STRUCT || !ts.tag) return expr;
  std::string base_key = std::string(ts.tag) + "::operator_bool";
  std::string const_key = base_key + "_const";
  auto mit = struct_methods_.find(base_key);
  if (mit == struct_methods_.end()) mit = struct_methods_.find(const_key);
  if (mit == struct_methods_.end()) return expr;

  CallExpr c{};
  DeclRef dr{};
  dr.name = mit->second;
  auto fit = module_->fn_index.find(dr.name);
  TypeSpec fn_ts{};
  fn_ts.base = TB_BOOL;
  if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
    fn_ts = module_->functions[fit->second.value].return_type.spec;
  }
  TypeSpec callee_ts = fn_ts;
  callee_ts.ptr_level++;
  c.callee = append_expr(n, dr, callee_ts);

  UnaryExpr addr{};
  addr.op = UnaryOp::AddrOf;
  addr.operand = expr;
  TypeSpec ptr_ts = ts;
  ptr_ts.ptr_level++;
  c.args.push_back(append_expr(n, addr, ptr_ts));

  return append_expr(n, c, fn_ts);
}

TypeSpec Lowerer::builtin_query_result_type() const {
  TypeSpec ts{};
  ts.base = TB_ULONG;
  return ts;
}

TypeSpec Lowerer::resolve_builtin_query_type(FunctionCtx* ctx, TypeSpec target) const {
  if (!ctx || ctx->tpl_bindings.empty() ||
      target.base != TB_TYPEDEF || !target.tag) {
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
    const std::string fn_name(expr->name);
    auto fit = module_->fn_index.find(fn_name);
    if (fit != module_->fn_index.end() &&
        fit->second.value < module_->functions.size()) {
      int fn_align = module_->functions[fit->second.value].attrs.align_bytes;
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
    case NK_VAR: {
      if (n->is_concept_id) {
        TypeSpec ts{};
        ts.base = TB_INT;
        ts.array_size = -1;
        ts.inner_rank = -1;
        return append_expr(n, IntLiteral{1, false}, ts);
      }
      if (n->name && n->name[0]) {
        if (n->has_template_args && find_template_struct_primary(n->name)) {
          TypeSpec tmp_ts{};
          tmp_ts.base = TB_STRUCT;
          tmp_ts.array_size = -1;
          tmp_ts.inner_rank = -1;
          tmp_ts.tpl_struct_origin = n->name;
          const Node* primary_tpl = find_template_struct_primary(n->name);
          assign_template_arg_refs_from_ast_args(
              &tmp_ts, n, primary_tpl, ctx, n, PendingTemplateTypeKind::OwnerStruct,
              "nameref-tpl-ctor-arg");
          TypeBindings tpl_empty;
          NttpBindings nttp_empty;
          seed_and_resolve_pending_template_type_if_needed(
              tmp_ts, ctx ? ctx->tpl_bindings : tpl_empty,
              ctx ? ctx->nttp_bindings : nttp_empty, n,
              PendingTemplateTypeKind::OwnerStruct, "nameref-tpl-ctor",
              primary_tpl);
          if (tmp_ts.tag && module_->struct_defs.count(tmp_ts.tag)) {
            const LocalId tmp_lid = next_local_id();
            const std::string tmp_name = "__tmp_struct_" + std::to_string(tmp_lid.value);
            LocalDecl tmp{};
            tmp.id = tmp_lid;
            tmp.name = tmp_name;
            tmp.type = qtype_from(tmp_ts, ValueCategory::LValue);
            tmp.storage = StorageClass::Auto;
            tmp.span = make_span(n);
            if (ctx) {
              ctx->locals[tmp.name] = tmp.id;
              ctx->local_types[tmp.id.value] = tmp_ts;
              append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
              DeclRef tmp_ref{};
              tmp_ref.name = tmp_name;
              tmp_ref.local = tmp_lid;
              return append_expr(n, tmp_ref, tmp_ts, ValueCategory::LValue);
            }
          }
        }
        std::string qname = n->name;
        size_t scope_pos = qname.rfind("::");
        if (scope_pos != std::string::npos) {
          std::string struct_tag = qname.substr(0, scope_pos);
          std::string member = qname.substr(scope_pos + 2);
          const bool has_template_args =
              n->has_template_args || n->n_template_args > 0;
          if (has_template_args) {
            if (auto v = try_eval_template_static_member_const(
                    ctx, struct_tag, n, member)) {
              TypeSpec ts{};
              if (const Node* decl = find_struct_static_member_decl(struct_tag, member))
                ts = decl->type;
              if (ts.base == TB_VOID) ts.base = TB_INT;
              return append_expr(n, IntLiteral{*v, false}, ts);
            }
          }
          if (!find_struct_static_member_decl(struct_tag, member) &&
              has_template_args && find_template_struct_primary(struct_tag)) {
            TypeSpec pending_ts{};
            pending_ts.base = TB_STRUCT;
            pending_ts.array_size = -1;
            pending_ts.inner_rank = -1;
            pending_ts.tpl_struct_origin = ::strdup(struct_tag.c_str());
            const Node* primary_tpl = find_template_struct_primary(struct_tag);
            assign_template_arg_refs_from_ast_args(
                &pending_ts, n, primary_tpl, ctx, n,
                PendingTemplateTypeKind::OwnerStruct,
                "nameref-scope-tpl-arg");
            TypeBindings tpl_empty;
            NttpBindings nttp_empty;
            seed_and_resolve_pending_template_type_if_needed(
                pending_ts, ctx ? ctx->tpl_bindings : tpl_empty,
                ctx ? ctx->nttp_bindings : nttp_empty, n,
                PendingTemplateTypeKind::OwnerStruct, "nameref-scope-tpl",
                primary_tpl);
            if (pending_ts.tag && pending_ts.tag[0]) struct_tag = pending_ts.tag;
          }
          if (auto v = find_struct_static_member_const_value(struct_tag, member)) {
            TypeSpec ts{};
            if (const Node* decl = find_struct_static_member_decl(struct_tag, member))
              ts = decl->type;
            if (ts.base == TB_VOID) ts.base = TB_INT;
            return append_expr(n, IntLiteral{*v, false}, ts);
          }
          if (const Node* decl = find_struct_static_member_decl(struct_tag, member)) {
            if (decl->init) {
              long long v = static_eval_int(decl->init, enum_consts_);
              TypeSpec ts = decl->type;
              if (ts.base == TB_VOID) ts.base = TB_INT;
              return append_expr(n, IntLiteral{v, false}, ts);
            }
          }
        }
        auto it = enum_consts_.find(n->name);
        if (it != enum_consts_.end()) {
          TypeSpec ts{};
          ts.base = TB_INT;
          return append_expr(n, IntLiteral{it->second, false}, ts);
        }
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
        if (auto instantiated = ensure_template_global_instance(ctx, n)) {
          r.global = *instantiated;
          if (const GlobalVar* gv = module_->find_global(*r.global)) r.name = gv->name;
        }
      }
      if (!has_local_binding) {
        auto git = module_->global_index.find(r.name);
        if (git != module_->global_index.end()) r.global = git->second;
      }
      if (ctx && !ctx->method_struct_tag.empty() && !has_local_binding &&
          !r.param_index && !r.global) {
        if (auto v = find_struct_static_member_const_value(ctx->method_struct_tag, r.name)) {
          TypeSpec ts{};
          if (const Node* decl =
                  find_struct_static_member_decl(ctx->method_struct_tag, r.name)) {
            ts = decl->type;
          }
          if (ts.base == TB_VOID) ts.base = TB_INT;
          return append_expr(n, IntLiteral{*v, false}, ts);
        }
        auto sit = module_->struct_defs.find(ctx->method_struct_tag);
        if (sit != module_->struct_defs.end() &&
            find_struct_instance_field_including_bases(*module_, ctx->method_struct_tag,
                                                       r.name)) {
          DeclRef this_ref{};
          this_ref.name = "this";
          auto pit = ctx->params.find("this");
          if (pit != ctx->params.end()) this_ref.param_index = pit->second;
          TypeSpec this_ts{};
          this_ts.base = TB_STRUCT;
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
      TypeSpec var_ts = n->type;
      if (var_ts.base == TB_VOID && var_ts.ptr_level == 0 && var_ts.array_rank == 0 &&
          !r.local && !r.param_index && !r.global) {
        auto fit = module_->fn_index.find(r.name);
        if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
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
      if (n->op) {
        const char* op_name = nullptr;
        if (std::string(n->op) == "++pre") op_name = "operator_preinc";
        else if (std::string(n->op) == "--pre") op_name = "operator_predec";
        else if (std::string(n->op) == "+") op_name = "operator_plus";
        else if (std::string(n->op) == "-") op_name = "operator_minus";
        else if (std::string(n->op) == "!") op_name = "operator_not";
        else if (std::string(n->op) == "~") op_name = "operator_bitnot";
        if (op_name) {
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {});
          if (op.valid()) return op;
        }
      }
      UnaryExpr u{};
      u.op = map_unary_op(n->op);
      u.operand = lower_expr(ctx, n->left);
      if (u.op == UnaryOp::Not) u.operand = maybe_bool_convert(ctx, u.operand, n->left);
      return append_expr(n, u, n->type);
    }
    case NK_POSTFIX: {
      if (n->op) {
        const char* op_name = nullptr;
        std::string op_str(n->op);
        if (op_str == "++") op_name = "operator_postinc";
        else if (op_str == "--") op_name = "operator_postdec";
        if (op_name) {
          TypeSpec int_ts{};
          int_ts.base = TB_INT;
          ExprId dummy = append_expr(n, IntLiteral{0, false}, int_ts);
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {}, {dummy});
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
        ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_addr", {});
        if (op.valid()) return op;
      }
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
      {
        ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_deref", {});
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
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {n->right});
          if (op.valid()) return op;
        }
      }
      BinaryExpr b{};
      b.op = map_binary_op(n->op);
      b.lhs = lower_expr(ctx, n->left);
      b.rhs = lower_expr(ctx, n->right);
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
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {n->right});
          if (op.valid()) return op;
        }
      }
      AssignExpr a{};
      a.op = map_assign_op(n->op, n->kind);
      a.lhs = lower_expr(ctx, n->left);
      a.rhs = lower_expr(ctx, n->right);
      return append_expr(n, a, n->type, ValueCategory::LValue);
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
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {n->right});
          if (op.valid()) return op;
        }
      }
      AssignExpr a{};
      a.op = map_assign_op(n->op, n->kind);
      a.lhs = lower_expr(ctx, n->left);
      a.rhs = lower_expr(ctx, n->right);
      return append_expr(n, a, n->type, ValueCategory::LValue);
    }
    case NK_CAST: {
      CastExpr c{};
      TypeSpec cast_ts =
          substitute_signature_template_type(
              n->type, ctx ? &ctx->tpl_bindings : nullptr);
      if (ctx && !ctx->tpl_bindings.empty() && cast_ts.tpl_struct_origin) {
        seed_and_resolve_pending_template_type_if_needed(
            cast_ts, ctx->tpl_bindings, ctx->nttp_bindings, n,
            PendingTemplateTypeKind::CastTarget, "cast-target");
      }
      c.to_type = qtype_from(cast_ts);
      c.expr = lower_expr(ctx, n->left);
      if (cast_ts.is_fn_ptr) c.fn_ptr_sig = fn_ptr_sig_from_decl_node(n);
      return append_expr(n, c, cast_ts);
    }
    case NK_CALL:
    case NK_BUILTIN_CALL:
      return lower_call_expr(ctx, n);
    case NK_VA_ARG: {
      VaArgExpr v{};
      v.ap = lower_expr(ctx, n->left);
      return append_expr(n, v, n->type);
    }
    case NK_INDEX: {
      if (n->right) {
        ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_subscript", {n->right});
        if (op.valid()) return op;
      }
      IndexExpr idx{};
      idx.base = lower_expr(ctx, n->left);
      idx.index = lower_expr(ctx, n->right);
      return append_expr(n, idx, n->type, ValueCategory::LValue);
    }
    case NK_MEMBER: {
      return lower_member_expr(ctx, n);
    }
    case NK_TERNARY: {
      if (const Node* cond = (n->cond ? n->cond : n->left)) {
        ConstEvalEnv env{&enum_consts_, nullptr, ctx ? &ctx->local_const_bindings : nullptr};
        if (auto r = evaluate_constant_expr(cond, env); r.ok()) {
          return lower_expr(ctx, (r.as_int() != 0) ? n->then_ : n->else_);
        }
      }
      if (ctx && (contains_stmt_expr(n->then_) || contains_stmt_expr(n->else_))) {
        TypeSpec result_ts = n->type;
        if (result_ts.base == TB_VOID) result_ts.base = TB_INT;
        LocalDecl tmp{};
        tmp.id = next_local_id();
        tmp.name = "__tern_tmp";
        tmp.type = qtype_from(result_ts, ValueCategory::LValue);
        TypeSpec zero_ts{};
        zero_ts.base = TB_INT;
        tmp.init = append_expr(n, IntLiteral{0, false}, zero_ts);
        const LocalId tmp_lid = tmp.id;
        ctx->locals[tmp.name] = tmp.id;
        ctx->local_types[tmp.id.value] = result_ts;
        append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
        const Node* cond_n = n->cond ? n->cond : n->left;
        ExprId cond_expr = maybe_bool_convert(ctx, lower_expr(ctx, cond_n), cond_n);
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
      TypeSpec ts{};
      ts.base = TB_ULONG;
      return append_expr(n, s, ts);
    }
    case NK_SIZEOF_PACK: {
      long long pack_size = 0;
      std::string pack_name = n->sval ? n->sval : "";
      if (pack_name.empty() && n->left && n->left->kind == NK_VAR && n->left->name) {
        pack_name = n->left->name;
      }
      if (ctx && !pack_name.empty()) {
        pack_size = count_pack_bindings_for_name(ctx->tpl_bindings, ctx->nttp_bindings, pack_name);
      }
      IntLiteral lit{};
      lit.value = pack_size;
      TypeSpec ts{};
      ts.base = TB_ULONG;
      return append_expr(n, lit, ts);
    }
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
