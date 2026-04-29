#include "stmt.hpp"
#include "consteval.hpp"

namespace c4c::hir {

namespace {

DeclRef make_link_name_decl_ref(Module& module, std::string name) {
  DeclRef ref{};
  ref.name = std::move(name);
  ref.name_text_id = make_text_id(ref.name, module.link_name_texts.get());
  ref.link_name_id = module.link_names.find(ref.name);
  return ref;
}

}  // namespace

bool Lowerer::struct_has_member_dtors(const std::string& tag) {
  auto sit = module_->struct_defs.find(tag);
  if (sit == module_->struct_defs.end()) return false;
  for (auto it = sit->second.fields.rbegin(); it != sit->second.fields.rend(); ++it) {
    if (it->elem_type.base == TB_STRUCT && it->elem_type.ptr_level == 0 &&
        it->elem_type.tag) {
      std::string ftag = it->elem_type.tag;
      if (struct_destructors_.count(ftag) || struct_has_member_dtors(ftag)) return true;
    }
  }
  return false;
}

void Lowerer::emit_defaulted_method_body(FunctionCtx& ctx,
                                         Function& fn,
                                         const std::string& struct_tag,
                                         const Node* method_node) {
  auto sit = module_->struct_defs.find(struct_tag);
  bool is_copy_or_move_ctor = method_node->is_constructor && method_node->n_params == 1;
  bool is_copy_or_move_assign =
      (method_node->operator_kind == OP_ASSIGN) && method_node->n_params == 1;

  if (is_copy_or_move_ctor || is_copy_or_move_assign) {
    if (sit != module_->struct_defs.end()) {
      DeclRef this_ref{};
      this_ref.name = "this";
      this_ref.name_text_id = make_text_id(
          this_ref.name, module_ ? module_->link_name_texts.get() : nullptr);
      auto pit = ctx.params.find("this");
      if (pit != ctx.params.end()) this_ref.param_index = pit->second;
      TypeSpec this_ts{};
      this_ts.base = TB_STRUCT;
      this_ts.tag = sit->second.tag.c_str();
      this_ts.ptr_level = 1;
      auto owner_sdit = struct_def_nodes_.find(struct_tag);
      if (owner_sdit != struct_def_nodes_.end()) {
        this_ts.record_def = const_cast<Node*>(owner_sdit->second);
      }
      ExprId this_id = append_expr(method_node, this_ref, this_ts, ValueCategory::LValue);

      std::string other_name =
          method_node->params[0]->name ? method_node->params[0]->name : "<anon_param>";
      DeclRef other_ref{};
      other_ref.name = other_name;
      other_ref.name_text_id = make_text_id(
          other_ref.name, module_ ? module_->link_name_texts.get() : nullptr);
      auto opit = ctx.params.find(other_name);
      if (opit != ctx.params.end()) other_ref.param_index = opit->second;
      TypeSpec other_ts = this_ts;
      ExprId other_id =
          append_expr(method_node, other_ref, other_ts, ValueCategory::LValue);

      for (const auto& field : sit->second.fields) {
        TypeSpec field_ts = field.elem_type;
        if (field.array_first_dim >= 0) {
          field_ts.array_rank = 1;
          field_ts.array_size = field.array_first_dim;
        }

        MemberExpr lhs_me{};
        lhs_me.base = this_id;
        lhs_me.field = field.name;
        lhs_me.field_text_id = make_text_id(
            lhs_me.field, module_ ? module_->link_name_texts.get() : nullptr);
        const std::optional<std::string> owner_tag = resolve_member_lookup_owner_tag(
            this_ts, true, &ctx.tpl_bindings, &ctx.nttp_bindings,
            &ctx.method_struct_tag, method_node,
            std::string("defaulted-method-member-owner:") + field.name);
        lhs_me.resolved_owner_tag = owner_tag.value_or(struct_tag);
        lhs_me.member_symbol_id =
            find_struct_member_symbol_id(lhs_me.resolved_owner_tag, field.name);
        if (lhs_me.member_symbol_id == kInvalidMemberSymbol) {
          lhs_me.member_symbol_id = field.member_symbol_id;
        }
        lhs_me.is_arrow = true;
        ExprId lhs_member =
            append_expr(method_node, lhs_me, field_ts, ValueCategory::LValue);

        MemberExpr rhs_me{};
        rhs_me.base = other_id;
        rhs_me.field = field.name;
        rhs_me.field_text_id = make_text_id(
            rhs_me.field, module_ ? module_->link_name_texts.get() : nullptr);
        rhs_me.resolved_owner_tag = lhs_me.resolved_owner_tag;
        rhs_me.member_symbol_id = lhs_me.member_symbol_id;
        rhs_me.is_arrow = true;
        ExprId rhs_member =
            append_expr(method_node, rhs_me, field_ts, ValueCategory::LValue);

        AssignExpr ae{};
        ae.lhs = lhs_member;
        ae.rhs = rhs_member;
        ExprId assign_id = append_expr(method_node, ae, field_ts);
        ExprStmt es{};
        es.expr = assign_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
      }
    }
  }

  if (is_copy_or_move_assign) {
    DeclRef this_ref2{};
    this_ref2.name = "this";
    this_ref2.name_text_id = make_text_id(
        this_ref2.name, module_ ? module_->link_name_texts.get() : nullptr);
    auto pit2 = ctx.params.find("this");
    if (pit2 != ctx.params.end()) this_ref2.param_index = pit2->second;
    TypeSpec this_ts2{};
    this_ts2.base = TB_STRUCT;
    this_ts2.tag =
        sit != module_->struct_defs.end() ? sit->second.tag.c_str() : struct_tag.c_str();
    this_ts2.ptr_level = 1;
    ExprId this_ret =
        append_expr(method_node, this_ref2, this_ts2, ValueCategory::LValue);
    ReturnStmt rs{};
    rs.expr = this_ret;
    append_stmt(ctx, Stmt{StmtPayload{rs}, make_span(method_node)});
  } else if (!method_node->is_destructor) {
    ReturnStmt rs{};
    append_stmt(ctx, Stmt{StmtPayload{rs}, make_span(method_node)});
  }
}

void Lowerer::emit_member_dtor_calls(FunctionCtx& ctx,
                                     const std::string& struct_tag,
                                     ExprId this_ptr_id,
                                     const Node* span_node) {
  auto sit = module_->struct_defs.find(struct_tag);
  if (sit == module_->struct_defs.end()) return;
  TypeSpec owner_ts{};
  owner_ts.base = TB_STRUCT;
  owner_ts.tag = sit->second.tag.c_str();
  owner_ts.ptr_level = 1;
  auto owner_sdit = struct_def_nodes_.find(struct_tag);
  if (owner_sdit != struct_def_nodes_.end()) {
    owner_ts.record_def = const_cast<Node*>(owner_sdit->second);
  }
  const auto& fields = sit->second.fields;
  for (auto it = fields.rbegin(); it != fields.rend(); ++it) {
    const auto& field = *it;
    if (field.elem_type.base != TB_STRUCT || field.elem_type.ptr_level != 0 ||
        !field.elem_type.tag) {
      continue;
    }
    std::string ftag = field.elem_type.tag;
    bool has_explicit_dtor = struct_destructors_.count(ftag) > 0;
    bool has_member_dtors = struct_has_member_dtors(ftag);
    if (!has_explicit_dtor && !has_member_dtors) continue;

    MemberExpr me{};
    me.base = this_ptr_id;
    me.field = field.name;
    me.field_text_id = make_text_id(
        me.field, module_ ? module_->link_name_texts.get() : nullptr);
    const std::optional<std::string> owner_tag = resolve_member_lookup_owner_tag(
        owner_ts, true, &ctx.tpl_bindings, &ctx.nttp_bindings,
        &ctx.method_struct_tag, span_node,
        std::string("member-dtor-member-owner:") + field.name);
    me.resolved_owner_tag = owner_tag.value_or(struct_tag);
    me.member_symbol_id = find_struct_member_symbol_id(me.resolved_owner_tag, field.name);
    if (me.member_symbol_id == kInvalidMemberSymbol) {
      me.member_symbol_id = field.member_symbol_id;
    }
    me.is_arrow = true;
    TypeSpec field_ts = field.elem_type;
    ExprId member_id = append_expr(span_node, me, field_ts, ValueCategory::LValue);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = member_id;
    TypeSpec ptr_ts = field_ts;
    ptr_ts.ptr_level++;
    ExprId member_ptr_id = append_expr(span_node, addr, ptr_ts);

    if (has_explicit_dtor) {
      auto dit = struct_destructors_.find(ftag);
      CallExpr c{};
      DeclRef callee_ref{};
      callee_ref.name = dit->second.mangled_name;
      callee_ref.name_text_id = make_text_id(
          callee_ref.name, module_ ? module_->link_name_texts.get() : nullptr);
      callee_ref.link_name_id = module_->link_names.find(callee_ref.name);
      TypeSpec fn_ts{};
      fn_ts.base = TB_VOID;
      TypeSpec callee_ts = fn_ts;
      callee_ts.ptr_level++;
      c.callee = append_expr(span_node, callee_ref, callee_ts);
      c.args.push_back(member_ptr_id);
      ExprId call_id = append_expr(span_node, c, fn_ts);
      ExprStmt es{};
      es.expr = call_id;
      append_stmt(ctx, Stmt{StmtPayload{es}, make_span(span_node)});
    } else {
      emit_member_dtor_calls(ctx, ftag, member_ptr_id, span_node);
    }
  }
}

void Lowerer::emit_dtor_calls(FunctionCtx& ctx, size_t since, const Node* span_node) {
  for (size_t i = ctx.dtor_stack.size(); i > since; --i) {
    const auto& dl = ctx.dtor_stack[i - 1];
    auto dit = struct_destructors_.find(dl.struct_tag);

    DeclRef var_ref{};
    var_ref.local = dl.local_id;
    TypeSpec var_ts{};
    if (ctx.local_types.contains(dl.local_id)) var_ts = ctx.local_types.at(dl.local_id);
    ExprId var_id = append_expr(span_node, var_ref, var_ts, ValueCategory::LValue);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = var_id;
    TypeSpec ptr_ts = var_ts;
    ptr_ts.ptr_level++;
    ExprId addr_id = append_expr(span_node, addr, ptr_ts);

    if (dit != struct_destructors_.end()) {
      CallExpr c{};
      DeclRef callee_ref{};
      callee_ref.name = dit->second.mangled_name;
      callee_ref.name_text_id = make_text_id(
          callee_ref.name, module_ ? module_->link_name_texts.get() : nullptr);
      callee_ref.link_name_id = module_->link_names.find(callee_ref.name);
      TypeSpec fn_ts{};
      fn_ts.base = TB_VOID;
      TypeSpec callee_ts = fn_ts;
      callee_ts.ptr_level++;
      c.callee = append_expr(span_node, callee_ref, callee_ts);
      c.args.push_back(addr_id);
      ExprId call_id = append_expr(span_node, c, fn_ts);
      ExprStmt es{};
      es.expr = call_id;
      append_stmt(ctx, Stmt{StmtPayload{es}, make_span(span_node)});
    } else {
      emit_member_dtor_calls(ctx, dl.struct_tag, addr_id, span_node);
    }
  }
}

void Lowerer::lower_stmt_node(FunctionCtx& ctx, const Node* n) {
  if (!n) return;

  switch (n->kind) {
    case NK_BLOCK: {
      // ival==1 marks synthetic multi-declarator blocks (e.g. int a, b;).
      // They share the current scope and must not discard bindings.
      const bool new_scope = (n->ival != 1);
      const auto saved_locals = ctx.locals;
      const auto saved_static_globals = ctx.static_globals;
      const auto saved_enum_consts = enum_consts_;
      const auto saved_local_consts = ctx.local_const_bindings;
      const size_t saved_dtor_depth = ctx.dtor_stack.size();
      for (int i = 0; i < n->n_children; ++i) {
        lower_stmt_node(ctx, n->children[i]);
      }
      if (new_scope) {
        emit_dtor_calls(ctx, saved_dtor_depth, n);
        ctx.dtor_stack.resize(saved_dtor_depth);
        ctx.locals = saved_locals;
        ctx.static_globals = saved_static_globals;
        enum_consts_ = saved_enum_consts;
        ctx.local_const_bindings = saved_local_consts;
      }
      return;
    }
    case NK_DECL: {
      lower_local_decl_stmt(ctx, n);
      return;
    }
    case NK_EXPR_STMT: {
      ExprStmt s{};
      if (n->left) s.expr = lower_expr(&ctx, n->left);
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      return;
    }
    case NK_ASM: {
      InlineAsmStmt s{};
      s.asm_template = rewrite_gcc_asm_template(decode_string_node(n->left));
      s.has_side_effects = true;
      if (n->asm_num_outputs == 1 && n->children[0]) {
        s.output = lower_expr(&ctx, n->children[0]);
        s.output_type.spec = n->children[0]->type;
        s.output_type.category = ValueCategory::LValue;
        if (n->asm_n_constraints > 0 && n->asm_constraints && n->asm_constraints[0]) {
          s.output_is_readwrite =
              strip_quoted_string(n->asm_constraints[0]).find('+') != std::string::npos;
        }
      }
      for (int i = 0; i < n->asm_num_inputs; ++i) {
        const Node* input = n->children[n->asm_num_outputs + i];
        if (!input) continue;
        s.inputs.push_back(lower_expr(&ctx, input));
      }
      for (int i = 0; i < n->asm_n_constraints; ++i) {
        const std::string constraint = strip_quoted_string(n->asm_constraints[i]);
        if (!s.constraints.empty()) s.constraints += ",";
        s.constraints += constraint;
      }
      for (int i = 0; i < n->asm_num_clobbers; ++i) {
        const Node* clobber =
            n->children[n->asm_num_outputs + n->asm_num_inputs + i];
        std::string name = decode_string_node(clobber);
        if (name.empty()) continue;
        if (!s.constraints.empty()) s.constraints += ",";
        s.constraints += "~{" + name + "}";
      }
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      return;
    }
    case NK_RETURN: {
      ReturnStmt s{};
      if (n->left) {
        ExprId val = lower_expr(&ctx, n->left);
        if (ctx.fn && is_any_ref_ts(ctx.fn->return_type.spec)) {
          UnaryExpr addr{};
          addr.op = UnaryOp::AddrOf;
          addr.operand = val;
          TypeSpec ptr_ts = ctx.fn->return_type.spec;
          ptr_ts.is_lvalue_ref = false;
          ptr_ts.is_rvalue_ref = false;
          ptr_ts.ptr_level++;
          val = append_expr(n, addr, ptr_ts);
        }
        s.expr = val;
      }
      emit_dtor_calls(ctx, 0, n);
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      return;
    }
    case NK_IF: {
      lower_if_stmt(ctx, n);
      return;
    }
    case NK_WHILE: {
      lower_while_stmt(ctx, n);
      return;
    }
    case NK_FOR: {
      lower_for_stmt(ctx, n);
      return;
    }
    case NK_RANGE_FOR: {
      lower_range_for_stmt(ctx, n);
      return;
    }
    case NK_DO_WHILE: {
      lower_do_while_stmt(ctx, n);
      return;
    }
    case NK_SWITCH: {
      lower_switch_stmt(ctx, n);
      return;
    }
    case NK_CASE: {
      lower_case_stmt(ctx, n);
      return;
    }
    case NK_CASE_RANGE: {
      lower_case_range_stmt(ctx, n);
      return;
    }
    case NK_DEFAULT: {
      lower_default_stmt(ctx, n);
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
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
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
      if (it != ctx.label_blocks.end()) s.target.resolved_block = it->second;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      return;
    }
    case NK_EMPTY:
    case NK_STATIC_ASSERT: {
      append_stmt(ctx, Stmt{StmtPayload{ExprStmt{}}, make_span(n)});
      return;
    }
    case NK_ENUM_DEF: {
      collect_enum_def(n);
      return;
    }
    case NK_INVALID_EXPR:
    case NK_INVALID_STMT:
      return;
    default: {
      ExprStmt s{};
      s.expr = lower_expr(&ctx, n);
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      return;
    }
  }
}

ExprId Lowerer::lower_stmt_expr_block(FunctionCtx& ctx,
                                      const Node* block,
                                      const TypeSpec& result_ts) {
  if (!block || block->kind != NK_BLOCK) {
    TypeSpec ts = result_ts;
    if (ts.base == TB_VOID) ts.base = TB_INT;
    return append_expr(nullptr, IntLiteral{0, false}, ts);
  }

  const bool new_scope = (block->ival != 1);
  const auto saved_locals = ctx.locals;
  const auto saved_static_globals = ctx.static_globals;

  ExprId result{};
  bool have_result = false;
  for (int i = 0; i < block->n_children; ++i) {
    const Node* child = block->children[i];
    const bool is_last = (i + 1 == block->n_children);
    if (is_last && child && child->kind == NK_EXPR_STMT && child->left &&
        result_ts.base != TB_VOID) {
      result = lower_expr(&ctx, child->left);
      have_result = true;
      continue;
    }
    lower_stmt_node(ctx, child);
  }

  if (new_scope) {
    ctx.locals = saved_locals;
    ctx.static_globals = saved_static_globals;
  }

  if (have_result) return result;
  TypeSpec ts = result_ts;
  if (ts.base == TB_VOID) ts.base = TB_INT;
  return append_expr(nullptr, IntLiteral{0, false}, ts);
}

void Lowerer::lower_struct_method(const std::string& mangled_name,
                                  const std::string& struct_tag,
                                  const Node* method_node,
                                  const TypeBindings* tpl_bindings,
                                  const NttpBindings* nttp_bindings) {
  if (method_node->is_deleted) return;
  Function fn{};
  fn.id = next_fn_id();
  fn.name = mangled_name;
  fn.link_name_id = module_->link_names.intern(fn.name);
  fn.ns_qual = make_ns_qual(
      method_node, module_ ? module_->link_name_texts.get() : nullptr);
  {
    TypeSpec ret_ts = prepare_callable_return_type(
        method_node->type, tpl_bindings, nttp_bindings, method_node,
        std::string("method-return:") + mangled_name, true);
    fn.return_type = qtype_from(ret_ts);
  }
  fn.linkage = {true, false, false, false, Visibility::Default};
  fn.attrs.variadic = method_node->variadic;
  fn.span = make_span(method_node);

  FunctionCtx ctx{};
  ctx.fn = &fn;
  if (tpl_bindings) ctx.tpl_bindings = *tpl_bindings;
  if (nttp_bindings) ctx.nttp_bindings = *nttp_bindings;
  ctx.method_struct_tag = struct_tag;

  {
    Param this_param{};
    this_param.name = "this";
    this_param.name_text_id = make_text_id(
        this_param.name, module_ ? module_->link_name_texts.get() : nullptr);
    TypeSpec this_ts{};
    this_ts.base = TB_STRUCT;
    auto sit = module_->struct_defs.find(struct_tag);
    this_ts.tag = sit != module_->struct_defs.end() ? sit->second.tag.c_str()
                                                    : struct_tag.c_str();
    this_ts.ptr_level = 1;
    this_param.type = qtype_from(this_ts, ValueCategory::LValue);
    this_param.span = make_span(method_node);
    ctx.params[this_param.name] = static_cast<uint32_t>(fn.params.size());
    fn.params.push_back(std::move(this_param));
  }

  append_callable_params(
      fn, ctx, method_node, tpl_bindings, nttp_bindings, "method-param:", true,
      false);

  if (maybe_register_bodyless_callable(
          &fn, method_node->body != nullptr || method_node->is_defaulted)) {
    return;
  }

  const BlockId entry = begin_callable_body_lowering(fn, ctx);

  if (method_node->is_defaulted) {
    emit_defaulted_method_body(ctx, fn, struct_tag, method_node);
    if (method_node->is_destructor) {
      DeclRef this_ref{};
      this_ref.name = "this";
      this_ref.name_text_id = make_text_id(
          this_ref.name, module_ ? module_->link_name_texts.get() : nullptr);
      auto pit = ctx.params.find("this");
      if (pit != ctx.params.end()) this_ref.param_index = pit->second;
      TypeSpec this_ts{};
      this_ts.base = TB_STRUCT;
      auto sit2 = module_->struct_defs.find(struct_tag);
      this_ts.tag = sit2 != module_->struct_defs.end()
                        ? sit2->second.tag.c_str()
                        : struct_tag.c_str();
      this_ts.ptr_level = 1;
      ExprId this_ptr = append_expr(
          method_node, this_ref, this_ts, ValueCategory::LValue);
      emit_member_dtor_calls(ctx, struct_tag, this_ptr, method_node);
    }
    if (method_node->is_destructor) {
      ReturnStmt rs{};
      append_stmt(ctx, Stmt{StmtPayload{rs}, make_span(method_node)});
    }
    finish_lowered_callable(&fn, entry);
    return;
  }

  if (method_node->is_constructor && method_node->n_ctor_inits > 0) {
    auto sit = module_->struct_defs.find(struct_tag);
    auto is_delegating_ctor_target = [&](const char* mem_name) -> bool {
      if (!mem_name || !mem_name[0]) return false;
      if (struct_tag == mem_name) return true;

      auto normalize_ctor_name = [&](const char* raw) -> std::string {
        if (!raw || !raw[0]) return {};
        std::string s(raw);
        size_t scope = s.rfind("::");
        if (scope != std::string::npos) s = s.substr(scope + 2);
        size_t tpl = s.find('<');
        if (tpl != std::string::npos) s.resize(tpl);
        return s;
      };

      const std::string mem_base = normalize_ctor_name(mem_name);
      auto matches_unqualified = [&](const char* candidate) -> bool {
        if (!candidate || !candidate[0]) return false;
        if (std::strcmp(candidate, mem_name) == 0) return true;
        const std::string candidate_base = normalize_ctor_name(candidate);
        return !mem_base.empty() && candidate_base == mem_base;
      };

      if (method_node) {
        if (matches_unqualified(method_node->name)) return true;
        if (matches_unqualified(method_node->unqualified_name)) return true;
        if (matches_unqualified(method_node->template_origin_name)) return true;
      }
      if (sit != module_->struct_defs.end()) {
        if (matches_unqualified(sit->second.tag.c_str())) return true;
      }
      auto sdit = struct_def_nodes_.find(struct_tag);
      if (sdit != struct_def_nodes_.end() && sdit->second) {
        if (matches_unqualified(sdit->second->name)) return true;
        if (matches_unqualified(sdit->second->unqualified_name)) return true;
        if (matches_unqualified(sdit->second->template_origin_name)) return true;
      }
      return false;
    };

    for (int i = 0; i < method_node->n_ctor_inits; ++i) {
      const char* mem_name = method_node->ctor_init_names[i];
      int nargs = method_node->ctor_init_nargs[i];
      Node** args = method_node->ctor_init_args[i];

      if (is_delegating_ctor_target(mem_name)) {
        auto cit = struct_constructors_.find(struct_tag);
        if (cit == struct_constructors_.end() || cit->second.empty()) {
          throw std::runtime_error(
              "error: no constructors found for delegating constructor call to '" +
              struct_tag + "'");
        }
        const CtorOverload* best = nullptr;
        std::vector<const CtorOverload*> arity_matches;
        arity_matches.reserve(cit->second.size());
        for (const auto& ov : cit->second) {
          if (ov.method_node->n_params != nargs) continue;
          if (ov.mangled_name == mangled_name) continue;
          arity_matches.push_back(&ov);
        }
        if (arity_matches.size() == 1) {
          best = arity_matches[0];
        } else {
          int best_score = -1;
          for (const auto& ov : cit->second) {
            if (ov.method_node->n_params != nargs) continue;
            if (ov.mangled_name == mangled_name) continue;
            int score = 0;
            bool viable = true;
            for (int pi = 0; pi < nargs && viable; ++pi) {
              TypeSpec param_ts = ov.method_node->params[pi]->type;
              resolve_typedef_to_struct(param_ts);
              TypeSpec arg_ts = infer_generic_ctrl_type(&ctx, args[pi]);
              resolve_typedef_to_struct(arg_ts);
              if (arg_ts.base != TB_STRUCT && args[pi] &&
                  args[pi]->kind == NK_CALL && args[pi]->left) {
                TypeSpec constructed_ts =
                    infer_generic_ctrl_type(&ctx, args[pi]->left);
                resolve_typedef_to_struct(constructed_ts);
                if (constructed_ts.base == TB_STRUCT &&
                    constructed_ts.ptr_level == 0) {
                  arg_ts = constructed_ts;
                }
              }
              bool arg_is_lvalue = is_ast_lvalue(args[pi], &ctx);
              TypeSpec param_base = param_ts;
              param_base.is_lvalue_ref = false;
              param_base.is_rvalue_ref = false;
              if (param_base.base == arg_ts.base &&
                  param_base.ptr_level == arg_ts.ptr_level) {
                score += 2;
                if (param_ts.is_rvalue_ref && !arg_is_lvalue) score += 4;
                else if (param_ts.is_rvalue_ref && arg_is_lvalue) viable = false;
                else if (param_ts.is_lvalue_ref && arg_is_lvalue) score += 3;
                else if (param_ts.is_lvalue_ref && !arg_is_lvalue) score += 1;
              } else if (param_base.ptr_level == 0 && arg_ts.ptr_level == 0 &&
                         param_base.base != TB_STRUCT &&
                         arg_ts.base != TB_STRUCT) {
                score += 1;
              } else {
                viable = false;
              }
            }
            if (viable && score > best_score) {
              best_score = score;
              best = &ov;
            }
          }
        }
        if (!best) {
          throw std::runtime_error(
              "error: no matching constructor for delegating constructor call to '" +
              struct_tag + "'");
        }
        if (best->method_node->is_deleted) {
          throw std::runtime_error(
              "error: call to deleted constructor '" + struct_tag + "'");
        }
        DeclRef this_ref{};
        this_ref.name = "this";
        this_ref.name_text_id = make_text_id(
            this_ref.name, module_ ? module_->link_name_texts.get() : nullptr);
        auto pit = ctx.params.find("this");
        if (pit != ctx.params.end()) this_ref.param_index = pit->second;
        TypeSpec this_ts{};
        this_ts.base = TB_STRUCT;
        this_ts.tag = struct_tag.c_str();
        this_ts.ptr_level = 1;
        ExprId this_id = append_expr(
            method_node, this_ref, this_ts, ValueCategory::LValue);

        CallExpr c{};
        DeclRef callee_ref = make_link_name_decl_ref(*module_, best->mangled_name);
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        c.callee = append_expr(method_node, callee_ref, callee_ts);
        c.args.push_back(this_id);
        for (int ai = 0; ai < nargs; ++ai) {
          TypeSpec param_ts = best->method_node->params[ai]->type;
          resolve_typedef_to_struct(param_ts);
          if (param_ts.is_rvalue_ref || param_ts.is_lvalue_ref) {
            ExprId arg_val = lower_expr(&ctx, args[ai]);
            TypeSpec storage_ts = reference_storage_ts(param_ts);
            UnaryExpr addr_e{};
            addr_e.op = UnaryOp::AddrOf;
            addr_e.operand = arg_val;
            c.args.push_back(append_expr(args[ai], addr_e, storage_ts));
          } else {
            c.args.push_back(lower_expr(&ctx, args[ai]));
          }
        }
        ExprId call_id = append_expr(method_node, c, fn_ts);
        ExprStmt es{};
        es.expr = call_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
        continue;
      }

      DeclRef this_ref{};
      this_ref.name = "this";
      this_ref.name_text_id = make_text_id(
          this_ref.name, module_ ? module_->link_name_texts.get() : nullptr);
      auto pit = ctx.params.find("this");
      if (pit != ctx.params.end()) this_ref.param_index = pit->second;
      TypeSpec this_ts{};
      this_ts.base = TB_STRUCT;
      this_ts.tag = sit != module_->struct_defs.end() ? sit->second.tag.c_str()
                                                      : struct_tag.c_str();
      this_ts.ptr_level = 1;
      auto owner_sdit = struct_def_nodes_.find(struct_tag);
      if (owner_sdit != struct_def_nodes_.end()) {
        this_ts.record_def = const_cast<Node*>(owner_sdit->second);
      }
      ExprId this_id = append_expr(
          method_node, this_ref, this_ts, ValueCategory::LValue);
      TypeSpec field_ts{};
      const Node* field_decl = nullptr;
      if (sit != module_->struct_defs.end()) {
        for (const auto& fld : sit->second.fields) {
          if (fld.name == mem_name) {
            field_ts = fld.elem_type;
            if (fld.array_first_dim >= 0) {
              field_ts.array_rank = 1;
              field_ts.array_size = fld.array_first_dim;
            }
            field_ts = substitute_signature_template_type(field_ts, &ctx.tpl_bindings);
            resolve_signature_template_type_if_needed(
                field_ts, &ctx.tpl_bindings, &ctx.nttp_bindings, &ctx.method_struct_tag,
                method_node,
                std::string("ctor-init-member:") + mem_name);
            resolve_typedef_to_struct(field_ts);
            break;
          }
        }
      }
      auto sdit = struct_def_nodes_.find(struct_tag);
      if (sdit != struct_def_nodes_.end() && sdit->second) {
        for (int fi = 0; fi < sdit->second->n_fields; ++fi) {
          const Node* candidate = sdit->second->fields[fi];
          if (candidate && candidate->name && mem_name &&
              std::string(candidate->name) == mem_name) {
            field_decl = candidate;
            break;
          }
        }
      }
      if (field_decl && resolved_types_) {
        if (auto ct = resolved_types_->lookup(field_decl)) {
          field_ts = sema::typespec_from_canonical(*ct);
        }
      }
      MemberExpr me{};
      me.base = this_id;
      me.field = mem_name;
      me.field_text_id = make_text_id(
          me.field, module_ ? module_->link_name_texts.get() : nullptr);
      const std::optional<std::string> owner_tag = resolve_member_lookup_owner_tag(
          this_ts, true, &ctx.tpl_bindings, &ctx.nttp_bindings, &ctx.method_struct_tag,
          method_node, std::string("ctor-init-member-owner:") + mem_name);
      me.resolved_owner_tag = owner_tag.value_or(struct_tag);
      me.member_symbol_id = find_struct_member_symbol_id(me.resolved_owner_tag, mem_name);
      me.is_arrow = true;
      ExprId lhs_id = append_expr(method_node, me, field_ts, ValueCategory::LValue);

      bool did_ctor_call = false;
      if (field_ts.base == TB_STRUCT && field_ts.tag && field_ts.ptr_level == 0) {
        auto cit = struct_constructors_.find(field_ts.tag);
        if (cit != struct_constructors_.end() && !cit->second.empty()) {
          const CtorOverload* best = nullptr;
          if (cit->second.size() == 1 &&
              cit->second[0].method_node->n_params == nargs) {
            best = &cit->second[0];
          } else {
            int best_score = -1;
            for (const auto& ov : cit->second) {
              if (ov.method_node->n_params != nargs) continue;
              int score = 0;
              bool viable = true;
              for (int pi = 0; pi < nargs && viable; ++pi) {
                TypeSpec param_ts = ov.method_node->params[pi]->type;
                resolve_typedef_to_struct(param_ts);
                TypeSpec arg_ts = infer_generic_ctrl_type(&ctx, args[pi]);
                resolve_typedef_to_struct(arg_ts);
                if (arg_ts.base != TB_STRUCT && args[pi] &&
                    args[pi]->kind == NK_CALL && args[pi]->left) {
                  TypeSpec constructed_ts =
                      infer_generic_ctrl_type(&ctx, args[pi]->left);
                  resolve_typedef_to_struct(constructed_ts);
                  if (constructed_ts.base == TB_STRUCT &&
                      constructed_ts.ptr_level == 0) {
                    arg_ts = constructed_ts;
                  }
                }
                bool arg_is_lvalue = is_ast_lvalue(args[pi], &ctx);
                TypeSpec param_base = param_ts;
                param_base.is_lvalue_ref = false;
                param_base.is_rvalue_ref = false;
                if (param_base.base == arg_ts.base &&
                    param_base.ptr_level == arg_ts.ptr_level) {
                  score += 2;
                  if (param_ts.is_rvalue_ref && !arg_is_lvalue) score += 4;
                  else if (param_ts.is_rvalue_ref && arg_is_lvalue) viable = false;
                  else if (param_ts.is_lvalue_ref && arg_is_lvalue) score += 3;
                  else if (param_ts.is_lvalue_ref && !arg_is_lvalue) score += 1;
                } else if (param_base.ptr_level == 0 && arg_ts.ptr_level == 0 &&
                           param_base.base != TB_STRUCT &&
                           arg_ts.base != TB_STRUCT) {
                  score += 1;
                } else {
                  viable = false;
                }
              }
              if (viable && score > best_score) {
                best_score = score;
                best = &ov;
              }
            }
          }
          if (best) {
            if (best->method_node->is_deleted) {
              std::string diag = "error: call to deleted constructor '";
              diag += field_ts.tag;
              diag += "'";
              throw std::runtime_error(diag);
            }
            CallExpr c{};
            DeclRef callee_ref = make_link_name_decl_ref(*module_, best->mangled_name);
            TypeSpec fn_ts{};
            fn_ts.base = TB_VOID;
            TypeSpec callee_ts = fn_ts;
            callee_ts.ptr_level++;
            c.callee = append_expr(method_node, callee_ref, callee_ts);
            UnaryExpr addr{};
            addr.op = UnaryOp::AddrOf;
            addr.operand = lhs_id;
            TypeSpec ptr_ts = field_ts;
            ptr_ts.ptr_level++;
            c.args.push_back(append_expr(method_node, addr, ptr_ts));
            for (int ai = 0; ai < nargs; ++ai) {
              TypeSpec param_ts = best->method_node->params[ai]->type;
              resolve_typedef_to_struct(param_ts);
              if (param_ts.is_rvalue_ref || param_ts.is_lvalue_ref) {
                ExprId arg_val = lower_expr(&ctx, args[ai]);
                TypeSpec storage_ts = reference_storage_ts(param_ts);
                UnaryExpr addr_e{};
                addr_e.op = UnaryOp::AddrOf;
                addr_e.operand = arg_val;
                c.args.push_back(append_expr(args[ai], addr_e, storage_ts));
              } else {
                c.args.push_back(lower_expr(&ctx, args[ai]));
              }
            }
            ExprId call_id = append_expr(method_node, c, fn_ts);
            ExprStmt es{};
            es.expr = call_id;
            append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
            did_ctor_call = true;
          }
        }
      }

      if (!did_ctor_call) {
        if (nargs == 0) {
          ExprId rhs_id = append_expr(method_node, IntLiteral{0, false}, field_ts);
          AssignExpr ae{};
          ae.op = AssignOp::Set;
          ae.lhs = lhs_id;
          ae.rhs = rhs_id;
          ExprId ae_id = append_expr(method_node, ae, field_ts);
          ExprStmt es{};
          es.expr = ae_id;
          append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
          continue;
        }
        if (nargs != 1) {
          std::string diag = "error: initializer for scalar member '";
          diag += mem_name;
          diag += "' must have exactly one argument";
          throw std::runtime_error(diag);
        }
        ExprId rhs_id = lower_expr(&ctx, args[0]);
        AssignExpr ae{};
        ae.op = AssignOp::Set;
        ae.lhs = lhs_id;
        ae.rhs = rhs_id;
        ExprId ae_id = append_expr(method_node, ae, field_ts);
        ExprStmt es{};
        es.expr = ae_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
      }
    }
  }

  lower_stmt_node(ctx, method_node->body);

  if (method_node->is_destructor) {
    DeclRef this_ref{};
    this_ref.name = "this";
    this_ref.name_text_id = make_text_id(
        this_ref.name, module_ ? module_->link_name_texts.get() : nullptr);
    auto pit = ctx.params.find("this");
    if (pit != ctx.params.end()) this_ref.param_index = pit->second;
    TypeSpec this_ts{};
    this_ts.base = TB_STRUCT;
    auto sit2 = module_->struct_defs.find(struct_tag);
    this_ts.tag = sit2 != module_->struct_defs.end()
                      ? sit2->second.tag.c_str()
                      : struct_tag.c_str();
    this_ts.ptr_level = 1;
    ExprId this_ptr = append_expr(
        method_node, this_ref, this_ts, ValueCategory::LValue);
    emit_member_dtor_calls(ctx, struct_tag, this_ptr, method_node);
  }

  finish_lowered_callable(&fn, entry);
}

}  // namespace c4c::hir
