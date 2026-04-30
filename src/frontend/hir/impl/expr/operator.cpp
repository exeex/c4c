#include "expr.hpp"
#include "consteval.hpp"

#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace c4c::hir {

std::optional<ExprId> Lowerer::try_lower_rvalue_ref_storage_addr(
    FunctionCtx* ctx,
    const Node* n,
    const TypeSpec& storage_ts) {
  if (!ctx || !n) return std::nullopt;

  ExprId storage_expr{};
  switch (n->kind) {
    case NK_CAST:
      // Preserve callable/function-pointer casts as values. Unwrapping them to
      // the operand storage address turns `(T (*)(...))raw` into `&raw`.
      if (!n->type.is_rvalue_ref || !n->left || n->type.is_fn_ptr) {
        return std::nullopt;
      }
      storage_expr = lower_expr(ctx, n->left);
      break;
    case NK_MEMBER:
      if (n->is_arrow || !n->left || is_ast_lvalue(n->left, ctx)) {
        return std::nullopt;
      }
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
  const TypeBindings* tpl_bindings = ctx ? &ctx->tpl_bindings : nullptr;
  const NttpBindings* nttp_bindings = ctx ? &ctx->nttp_bindings : nullptr;
  const std::string* current_struct_tag =
      (ctx && !ctx->method_struct_tag.empty()) ? &ctx->method_struct_tag : nullptr;
  const std::string owner_tag = resolve_struct_method_lookup_owner_tag(
      obj_ts, false, tpl_bindings, nttp_bindings, current_struct_tag, result_node,
      std::string("operator-call:") + op_method_name);
  if (owner_tag.empty()) return ExprId::invalid();

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
      find_struct_method_mangled(owner_tag, op_method_name, obj_ts.is_const);
  if (!resolved) return ExprId::invalid();

  std::string resolved_mangled = *resolved;
  std::string base_key = owner_tag + "::" + op_method_name;
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
  dr.link_name_id = module_->link_names.find(dr.name);
  TypeSpec fn_ts{};
  fn_ts.base = TB_VOID;
  const Function* callee_fn = module_->resolve_operator_callee(dr);
  if (callee_fn) {
    fn_ts = callee_fn->return_type.spec;
  }
  if (fn_ts.base == TB_VOID) {
    if (auto rit = find_struct_method_return_type(
            owner_tag, op_method_name, obj_ts.is_const)) {
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

  const Node* method_ast = nullptr;
  if (!callee_fn) method_ast = find_pending_method_by_mangled(resolved_mangled);

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
      ctx->local_types.insert(tmp.id, val_ts);
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
  auto resolved_member_type = [&](const MemberExpr& member,
                                  const TypeSpec& fallback) -> TypeSpec {
    if (member.resolved_owner_tag.empty()) return fallback;

    std::function<const HirStructField*(const std::string&, const std::string&)> find_field =
        [&](const std::string& tag, const std::string& field) -> const HirStructField* {
      auto it = module_->struct_defs.find(tag);
      if (it == module_->struct_defs.end()) return nullptr;
      for (const auto& candidate : it->second.fields) {
        if (candidate.name == field) return &candidate;
      }
      for (const auto& base_tag : it->second.base_tags) {
        if (const HirStructField* inherited = find_field(base_tag, field)) {
          return inherited;
        }
      }
      return nullptr;
    };

    if (const HirStructField* field =
            find_field(member.resolved_owner_tag, member.field)) {
      return field_type_of(*field);
    }
    return fallback;
  };

  if (n->is_arrow && n->left) {
    ExprId arrow_ptr = try_lower_operator_call(ctx, n, n->left, "operator_arrow", {});
    if (arrow_ptr.valid()) {
      for (int depth = 0; depth < 8; ++depth) {
        const Expr* res_expr = module_->find_expr(arrow_ptr);
        if (!res_expr) break;
        TypeSpec rts = res_expr->type.spec;
        if (rts.ptr_level > 0) break;
        if (rts.base != TB_STRUCT || !rts.tag) break;
        const TypeBindings* tpl_bindings = ctx ? &ctx->tpl_bindings : nullptr;
        const NttpBindings* nttp_bindings = ctx ? &ctx->nttp_bindings : nullptr;
        const std::string* current_struct_tag =
            (ctx && !ctx->method_struct_tag.empty()) ? &ctx->method_struct_tag : nullptr;
        const std::string owner_tag = resolve_struct_method_lookup_owner_tag(
            rts, false, tpl_bindings, nttp_bindings, current_struct_tag, n,
            "operator-arrow-chain");
        if (owner_tag.empty()) break;
        auto method = find_struct_method_mangled(
            owner_tag, "operator_arrow", rts.is_const);
        if (!method) break;
        DeclRef arrow_method_ref{};
        arrow_method_ref.name = *method;
        arrow_method_ref.link_name_id = module_->link_names.find(arrow_method_ref.name);
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        if (const Function* fn = module_->resolve_operator_callee(arrow_method_ref)) {
          fn_ts = fn->return_type.spec;
        }
        if (fn_ts.base == TB_VOID) {
          if (auto rit2 = find_struct_method_return_type(
                  owner_tag, "operator_arrow", false)) {
            fn_ts = *rit2;
          }
        }
        LocalDecl tmp{};
        tmp.id = next_local_id();
        tmp.name = "__arrow_tmp";
        tmp.type = qtype_from(rts, ValueCategory::LValue);
        tmp.init = arrow_ptr;
        const LocalId tmp_lid = tmp.id;
        ctx->locals[tmp.name] = tmp.id;
        ctx->local_types.insert(tmp.id, rts);
        append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
        DeclRef tmp_ref{};
        tmp_ref.name = "__arrow_tmp";
        tmp_ref.local = tmp_lid;
        ExprId tmp_id = append_expr(n, tmp_ref, rts, ValueCategory::LValue);
        CallExpr cc{};
        DeclRef dr{};
        dr.name = *method;
        dr.link_name_id = arrow_method_ref.link_name_id;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        cc.callee = append_expr(n, dr, callee_ts);
        UnaryExpr addr2{};
        addr2.op = UnaryOp::AddrOf;
        addr2.operand = tmp_id;
        TypeSpec ptr_ts = rts;
        ptr_ts.ptr_level++;
        cc.args.push_back(append_expr(n, addr2, ptr_ts));
        arrow_ptr = append_expr(n, cc, fn_ts);
      }
      MemberExpr m{};
      m.base = arrow_ptr;
      m.field = n->name ? n->name : "<anon_field>";
      m.field_text_id = make_unqualified_text_id(
          n, module_ ? module_->link_name_texts.get() : nullptr);
      m.is_arrow = true;
      const TypeSpec base_ts = module_->expr_pool[arrow_ptr.value].type.spec;
      const TypeBindings* tpl_bindings = ctx ? &ctx->tpl_bindings : nullptr;
      const NttpBindings* nttp_bindings = ctx ? &ctx->nttp_bindings : nullptr;
      const std::string* current_struct_tag =
          (ctx && !ctx->method_struct_tag.empty()) ? &ctx->method_struct_tag : nullptr;
      if (auto owner_tag = resolve_member_lookup_owner_tag(
              base_ts, m.is_arrow, tpl_bindings, nttp_bindings, current_struct_tag,
              n, std::string("member-owner:") + m.field)) {
        m.resolved_owner_tag = *owner_tag;
        m.member_symbol_id = find_struct_member_symbol_id(
            base_ts, *owner_tag, m.field, m.field_text_id);
      } else if (n->left) {
        const TypeSpec ast_base_ts = infer_generic_ctrl_type(ctx, n->left);
        if (auto owner_tag = resolve_member_lookup_owner_tag(
                ast_base_ts, m.is_arrow, tpl_bindings, nttp_bindings, current_struct_tag,
                n, std::string("member-owner-ast:") + m.field)) {
          m.resolved_owner_tag = *owner_tag;
          m.member_symbol_id = find_struct_member_symbol_id(
              ast_base_ts, *owner_tag, m.field, m.field_text_id);
        } else if (auto owner_tag = resolve_member_lookup_owner_tag(
                       n->left->type, m.is_arrow, tpl_bindings, nttp_bindings,
                       current_struct_tag, n,
                       std::string("member-owner-raw-ast:") + m.field)) {
          m.resolved_owner_tag = *owner_tag;
          m.member_symbol_id = find_struct_member_symbol_id(
              n->left->type, *owner_tag, m.field, m.field_text_id);
        }
      }
      return append_expr(n, m, resolved_member_type(m, n->type), ValueCategory::LValue);
    }
  }

  MemberExpr m{};
  m.base = lower_expr(ctx, n->left);
  m.field = n->name ? n->name : "<anon_field>";
  m.field_text_id = make_unqualified_text_id(
      n, module_ ? module_->link_name_texts.get() : nullptr);
  m.is_arrow = n->is_arrow;
  const TypeSpec base_ts = module_->expr_pool[m.base.value].type.spec;
  const TypeBindings* tpl_bindings = ctx ? &ctx->tpl_bindings : nullptr;
  const NttpBindings* nttp_bindings = ctx ? &ctx->nttp_bindings : nullptr;
  const std::string* current_struct_tag =
      (ctx && !ctx->method_struct_tag.empty()) ? &ctx->method_struct_tag : nullptr;
  if (auto owner_tag = resolve_member_lookup_owner_tag(
          base_ts, m.is_arrow, tpl_bindings, nttp_bindings, current_struct_tag,
          n, std::string("member-owner:") + m.field)) {
    m.resolved_owner_tag = *owner_tag;
    m.member_symbol_id = find_struct_member_symbol_id(
        base_ts, *owner_tag, m.field, m.field_text_id);
  } else if (n->left) {
    const TypeSpec ast_base_ts = infer_generic_ctrl_type(ctx, n->left);
    if (auto owner_tag = resolve_member_lookup_owner_tag(
            ast_base_ts, m.is_arrow, tpl_bindings, nttp_bindings, current_struct_tag,
            n, std::string("member-owner-ast:") + m.field)) {
      m.resolved_owner_tag = *owner_tag;
      m.member_symbol_id = find_struct_member_symbol_id(
          ast_base_ts, *owner_tag, m.field, m.field_text_id);
    } else if (auto owner_tag = resolve_member_lookup_owner_tag(
                   n->left->type, m.is_arrow, tpl_bindings, nttp_bindings,
                   current_struct_tag, n,
                   std::string("member-owner-raw-ast:") + m.field)) {
      m.resolved_owner_tag = *owner_tag;
      m.member_symbol_id = find_struct_member_symbol_id(
          n->left->type, *owner_tag, m.field, m.field_text_id);
    }
  }
  const ValueCategory category =
      (n->is_arrow || is_ast_lvalue(n->left, ctx))
          ? ValueCategory::LValue
          : ValueCategory::RValue;
  return append_expr(n, m, resolved_member_type(m, n->type), category);
}

ExprId Lowerer::maybe_bool_convert(FunctionCtx* ctx, ExprId expr, const Node* n) {
  if (!expr.valid() || !n) return expr;
  TypeSpec ts = infer_generic_ctrl_type(ctx, n);
  if (ts.ptr_level != 0 || ts.base != TB_STRUCT || !ts.tag) return expr;
  const TypeBindings* tpl_bindings = ctx ? &ctx->tpl_bindings : nullptr;
  const NttpBindings* nttp_bindings = ctx ? &ctx->nttp_bindings : nullptr;
  const std::string* current_struct_tag =
      (ctx && !ctx->method_struct_tag.empty()) ? &ctx->method_struct_tag : nullptr;
  const std::string owner_tag = resolve_struct_method_lookup_owner_tag(
      ts, false, tpl_bindings, nttp_bindings, current_struct_tag, n,
      "operator-bool-convert");
  if (owner_tag.empty()) return expr;
  auto method = find_struct_method_mangled(owner_tag, "operator_bool", false);
  if (!method) return expr;

  CallExpr c{};
  DeclRef dr{};
  dr.name = *method;
  dr.link_name_id = module_->link_names.find(dr.name);
  TypeSpec fn_ts{};
  fn_ts.base = TB_BOOL;
  if (const Function* fn = module_->resolve_operator_callee(dr)) {
    fn_ts = fn->return_type.spec;
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

}  // namespace c4c::hir
