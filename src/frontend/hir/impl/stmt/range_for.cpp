#include "stmt.hpp"

#include <stdexcept>

namespace c4c::hir {

void Lowerer::lower_range_for_stmt(FunctionCtx& ctx, const Node* n) {
  const Node* decl_node = n->init;
  const Node* range_node = n->right;

  TypeSpec range_ts = infer_generic_ctrl_type(&ctx, range_node);
  if (range_ts.base != TB_STRUCT || !range_ts.tag) {
    throw std::runtime_error(
        std::string("error: range-for expression is not a struct type (line ") +
        std::to_string(n->line) + ")");
  }

  auto find_method = [&](const char* method_name) -> std::string {
    std::string base_key = std::string(range_ts.tag) + "::" + method_name;
    std::string const_key = base_key + "_const";
    decltype(struct_methods_)::iterator mit;
    if (range_ts.is_const) {
      mit = struct_methods_.find(const_key);
      if (mit == struct_methods_.end()) mit = struct_methods_.find(base_key);
    } else {
      mit = struct_methods_.find(base_key);
      if (mit == struct_methods_.end()) mit = struct_methods_.find(const_key);
    }
    if (mit == struct_methods_.end()) {
      throw std::runtime_error(
          std::string("error: range-for: no ") + method_name +
          "() method on struct " + range_ts.tag + " (line " +
          std::to_string(n->line) + ")");
    }
    return mit->second;
  };

  std::string begin_mangled = find_method("begin");
  std::string end_mangled = find_method("end");

  TypeSpec iter_ts{};
  iter_ts.base = TB_VOID;
  {
    if (const Function* fn = module_->find_function_by_name_legacy(begin_mangled)) {
      iter_ts = fn->return_type.spec;
    }
  }
  if (iter_ts.base == TB_VOID) {
    auto rit = struct_method_ret_types_.find(std::string(range_ts.tag) + "::begin");
    if (rit == struct_method_ret_types_.end()) {
      rit = struct_method_ret_types_.find(std::string(range_ts.tag) + "::begin_const");
    }
    if (rit != struct_method_ret_types_.end()) iter_ts = rit->second;
  }
  if (iter_ts.base == TB_VOID) {
    throw std::runtime_error(
        std::string("error: range-for: cannot determine iterator type from begin() (line ") +
        std::to_string(n->line) + ")");
  }

  ExprId range_id = lower_expr(&ctx, range_node);

  auto build_method_call = [&](const std::string& mangled, ExprId obj_id) -> ExprId {
    CallExpr cc{};
    DeclRef dr{};
    dr.name = mangled;
    dr.link_name_id = module_->link_names.find(dr.name);
    TypeSpec callee_ts = iter_ts;
    callee_ts.ptr_level++;
    cc.callee = append_expr(n, dr, callee_ts);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = obj_id;
    TypeSpec ptr_ts = range_ts;
    ptr_ts.ptr_level++;
    cc.args.push_back(append_expr(n, addr, ptr_ts));
    return append_expr(n, cc, iter_ts);
  };

  ExprId begin_call = build_method_call(begin_mangled, range_id);
  ExprId end_call = build_method_call(end_mangled, range_id);

  auto make_iter_local = [&](const char* name, ExprId init_expr) -> LocalId {
    LocalDecl ld{};
    ld.id = next_local_id();
    ld.name = name;
    ld.type = qtype_from(iter_ts, ValueCategory::LValue);
    ld.init = init_expr;
    LocalId lid = ld.id;
    ctx.locals[name] = lid;
    ctx.local_types.insert(lid, iter_ts);
    append_stmt(ctx, Stmt{StmtPayload{std::move(ld)}, make_span(n)});
    return lid;
  };

  LocalId begin_lid = make_iter_local("__range_begin", begin_call);
  LocalId end_lid = make_iter_local("__range_end", end_call);

  auto ref_local = [&](const char* name, LocalId lid) -> ExprId {
    DeclRef dr{};
    dr.name = name;
    dr.local = lid;
    return append_expr(n, dr, iter_ts, ValueCategory::LValue);
  };

  ExprId cond_expr;
  {
    std::string neq_base = std::string(iter_ts.tag) + "::operator_neq";
    std::string neq_const = neq_base + "_const";
    auto mit = struct_methods_.find(neq_base);
    if (mit == struct_methods_.end()) mit = struct_methods_.find(neq_const);
    if (mit == struct_methods_.end()) {
      throw std::runtime_error(
          std::string("error: range-for: iterator type ") + iter_ts.tag +
          " has no operator!= (line " + std::to_string(n->line) + ")");
    }
    CallExpr cc{};
    DeclRef dr{};
    dr.name = mit->second;
    dr.link_name_id = module_->link_names.find(dr.name);
    TypeSpec bool_ts{};
    bool_ts.base = TB_BOOL;
    TypeSpec callee_ts = bool_ts;
    callee_ts.ptr_level++;
    cc.callee = append_expr(n, dr, callee_ts);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = ref_local("__range_begin", begin_lid);
    TypeSpec ptr_ts = iter_ts;
    ptr_ts.ptr_level++;
    cc.args.push_back(append_expr(n, addr, ptr_ts));
    cc.args.push_back(ref_local("__range_end", end_lid));
    cond_expr = append_expr(n, cc, bool_ts);
  }

  ExprId update_expr;
  {
    std::string inc_base = std::string(iter_ts.tag) + "::operator_preinc";
    std::string inc_const = inc_base + "_const";
    auto mit = struct_methods_.find(inc_base);
    if (mit == struct_methods_.end()) mit = struct_methods_.find(inc_const);
    if (mit == struct_methods_.end()) {
      throw std::runtime_error(
          std::string("error: range-for: iterator type ") + iter_ts.tag +
          " has no prefix operator++ (line " + std::to_string(n->line) + ")");
    }
    CallExpr cc{};
    DeclRef dr{};
    dr.name = mit->second;
    dr.link_name_id = module_->link_names.find(dr.name);
    TypeSpec inc_ret_ts = iter_ts;
    {
      if (const Function* fn = module_->find_function_by_name_legacy(mit->second)) {
        inc_ret_ts = fn->return_type.spec;
      }
    }
    TypeSpec callee_ts = inc_ret_ts;
    callee_ts.ptr_level++;
    cc.callee = append_expr(n, dr, callee_ts);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = ref_local("__range_begin", begin_lid);
    TypeSpec ptr_ts = iter_ts;
    ptr_ts.ptr_level++;
    cc.args.push_back(append_expr(n, addr, ptr_ts));
    update_expr = append_expr(n, cc, inc_ret_ts);
  }

  ForStmt fs{};
  fs.cond = cond_expr;
  fs.update = update_expr;
  const BlockId body_b = create_block(ctx);
  const BlockId after_b = create_block(ctx);
  fs.body_block = body_b;
  fs.continue_target = body_b;
  fs.break_target = after_b;
  append_stmt(ctx, Stmt{StmtPayload{fs}, make_span(n)});
  ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

  ctx.break_stack.push_back(after_b);
  ctx.continue_stack.push_back(body_b);
  ctx.current_block = body_b;

  {
    ExprId deref_expr;
    TypeSpec deref_ret_ts{};
    deref_ret_ts.base = TB_INT;
    {
      std::string deref_base = std::string(iter_ts.tag) + "::operator_deref";
      std::string deref_const = deref_base + "_const";
      auto mit = struct_methods_.find(deref_base);
      if (mit == struct_methods_.end()) mit = struct_methods_.find(deref_const);
      if (mit == struct_methods_.end()) {
        throw std::runtime_error(
            std::string("error: range-for: iterator type ") + iter_ts.tag +
            " has no operator* (line " + std::to_string(n->line) + ")");
      }
      CallExpr cc{};
      DeclRef dr{};
      dr.name = mit->second;
      dr.link_name_id = module_->link_names.find(dr.name);
      {
        if (const Function* fn = module_->find_function_by_name_legacy(mit->second)) {
          deref_ret_ts = fn->return_type.spec;
        }
      }
      if (deref_ret_ts.base == TB_VOID || deref_ret_ts.base == TB_INT) {
        auto rit = struct_method_ret_types_.find(
            std::string(iter_ts.tag) + "::operator_deref");
        if (rit == struct_method_ret_types_.end()) {
          rit = struct_method_ret_types_.find(
              std::string(iter_ts.tag) + "::operator_deref_const");
        }
        if (rit != struct_method_ret_types_.end()) deref_ret_ts = rit->second;
      }
      TypeSpec callee_ts = deref_ret_ts;
      callee_ts.ptr_level++;
      cc.callee = append_expr(n, dr, callee_ts);
      UnaryExpr addr{};
      addr.op = UnaryOp::AddrOf;
      addr.operand = ref_local("__range_begin", begin_lid);
      TypeSpec ptr_ts = iter_ts;
      ptr_ts.ptr_level++;
      cc.args.push_back(append_expr(n, addr, ptr_ts));
      deref_expr = append_expr(n, cc, reference_storage_ts(deref_ret_ts));
    }

    if (decl_node) {
      LocalDecl ld{};
      ld.id = next_local_id();
      ld.name = decl_node->name ? decl_node->name : "__range_var";
      TypeSpec var_ts = decl_node->type;
      bool is_ref = var_ts.is_lvalue_ref;
      if (var_ts.base == TB_AUTO) {
        bool was_const = var_ts.is_const;
        var_ts = deref_ret_ts;
        var_ts.is_lvalue_ref = false;
        if (was_const) var_ts.is_const = true;
        if (is_ref) var_ts.is_lvalue_ref = true;
      }
      resolve_typedef_to_struct(var_ts);
      ld.type = qtype_from(reference_storage_ts(var_ts), ValueCategory::LValue);
      if (is_ref) {
        if (deref_ret_ts.is_lvalue_ref) {
          ld.init = deref_expr;
        } else {
          UnaryExpr addr{};
          addr.op = UnaryOp::AddrOf;
          addr.operand = deref_expr;
          ld.init = append_expr(n, addr, ld.type.spec);
        }
      } else {
        ld.init = deref_expr;
      }
      ctx.locals[ld.name] = ld.id;
      ctx.local_types.insert(ld.id, var_ts);
      append_stmt(ctx, Stmt{StmtPayload{std::move(ld)}, make_span(n)});
    }
  }

  if (n->body) lower_stmt_node(ctx, n->body);

  if (ctx.current_block.value != after_b.value &&
      !ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
    ContinueStmt cont{};
    cont.target = body_b;
    append_stmt(ctx, Stmt{StmtPayload{cont}, make_span(n)});
    ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
  }
  ctx.break_stack.pop_back();
  ctx.continue_stack.pop_back();
  ctx.current_block = after_b;
}

}  // namespace c4c::hir
