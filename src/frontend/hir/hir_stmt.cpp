#include "hir_lowerer_internal.hpp"
#include "consteval.hpp"

namespace c4c::hir {

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
      if (n->is_constexpr) {
        const Node* cond_node = n->cond ? n->cond : n->left;
        ConstEvalEnv cenv{&enum_consts_, &const_int_bindings_, &ctx.local_const_bindings};
        auto cr = evaluate_constant_expr(cond_node, cenv);
        if (cr.ok()) {
          if (cr.as_int()) {
            lower_stmt_node(ctx, n->then_);
          } else if (n->else_) {
            lower_stmt_node(ctx, n->else_);
          }
          return;
        }
      }

      IfStmt s{};
      {
        const Node* cond_n = n->cond ? n->cond : n->left;
        s.cond = maybe_bool_convert(&ctx, lower_expr(&ctx, cond_n), cond_n);
      }
      const BlockId then_b = create_block(ctx);
      s.then_block = then_b;
      if (n->else_) s.else_block = create_block(ctx);
      const BlockId after_b = create_block(ctx);
      s.after_block = after_b;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

      ctx.current_block = then_b;
      lower_stmt_node(ctx, n->then_);
      if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        GotoStmt j{};
        j.target.resolved_block = after_b;
        append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }

      if (n->else_) {
        ctx.current_block = *s.else_block;
        lower_stmt_node(ctx, n->else_);
        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = after_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }
      }

      ctx.current_block = after_b;
      return;
    }
    case NK_WHILE: {
      const BlockId cond_b = create_block(ctx);
      const BlockId body_b = create_block(ctx);
      const BlockId after_b = create_block(ctx);

      if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        GotoStmt j{};
        j.target.resolved_block = cond_b;
        append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }

      ctx.current_block = cond_b;
      WhileStmt s{};
      {
        const Node* cond_n = n->cond ? n->cond : n->left;
        s.cond = maybe_bool_convert(&ctx, lower_expr(&ctx, cond_n), cond_n);
      }
      s.body_block = body_b;
      s.continue_target = cond_b;
      s.break_target = after_b;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

      ctx.break_stack.push_back(after_b);
      ctx.continue_stack.push_back(cond_b);
      ctx.current_block = body_b;
      lower_stmt_node(ctx, n->body);
      if (ctx.current_block.value != after_b.value &&
          !ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        ContinueStmt c{};
        c.target = cond_b;
        append_stmt(ctx, Stmt{StmtPayload{c}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }
      ctx.break_stack.pop_back();
      ctx.continue_stack.pop_back();

      ctx.current_block = after_b;
      return;
    }
    case NK_FOR: {
      ForStmt s{};
      if (n->init) {
        const bool init_is_decl = (n->init->kind == NK_DECL ||
                                   n->init->kind == NK_BLOCK);
        if (init_is_decl) {
          lower_stmt_node(ctx, n->init);
        } else {
          s.init = lower_expr(&ctx, n->init);
        }
      }
      if (n->cond) s.cond = maybe_bool_convert(&ctx, lower_expr(&ctx, n->cond), n->cond);
      if (n->update) s.update = lower_expr(&ctx, n->update);
      const BlockId body_b = create_block(ctx);
      const BlockId after_b = create_block(ctx);
      s.body_block = body_b;
      s.continue_target = body_b;
      s.break_target = after_b;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

      ctx.break_stack.push_back(after_b);
      ctx.continue_stack.push_back(body_b);
      ctx.current_block = body_b;
      lower_stmt_node(ctx, n->body);
      if (ctx.current_block.value != after_b.value &&
          !ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        ContinueStmt c{};
        c.target = body_b;
        append_stmt(ctx, Stmt{StmtPayload{c}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }
      ctx.break_stack.pop_back();
      ctx.continue_stack.pop_back();

      ctx.current_block = after_b;
      return;
    }
    case NK_RANGE_FOR: {
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
        auto fit = module_->fn_index.find(begin_mangled);
        if (fit != module_->fn_index.end() &&
            fit->second.value < module_->functions.size()) {
          iter_ts = module_->functions[fit->second.value].return_type.spec;
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
        ctx.local_types[lid.value] = iter_ts;
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
        TypeSpec inc_ret_ts = iter_ts;
        {
          auto fit2 = module_->fn_index.find(mit->second);
          if (fit2 != module_->fn_index.end() &&
              fit2->second.value < module_->functions.size()) {
            inc_ret_ts = module_->functions[fit2->second.value].return_type.spec;
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
          {
            auto fit2 = module_->fn_index.find(mit->second);
            if (fit2 != module_->fn_index.end() &&
                fit2->second.value < module_->functions.size()) {
              deref_ret_ts = module_->functions[fit2->second.value].return_type.spec;
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
          ctx.local_types[ld.id.value] = var_ts;
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
      return;
    }
    case NK_DO_WHILE: {
      const BlockId body_b = create_block(ctx);
      const BlockId cond_b = create_block(ctx);
      const BlockId after_b = create_block(ctx);

      if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        GotoStmt j{};
        j.target.resolved_block = body_b;
        append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }

      ctx.break_stack.push_back(after_b);
      ctx.continue_stack.push_back(cond_b);
      ctx.current_block = body_b;
      lower_stmt_node(ctx, n->body);
      if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        GotoStmt j{};
        j.target.resolved_block = cond_b;
        append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }

      ctx.current_block = cond_b;
      DoWhileStmt s{};
      s.body_block = body_b;
      {
        const Node* cond_n = n->cond ? n->cond : n->left;
        s.cond = maybe_bool_convert(&ctx, lower_expr(&ctx, cond_n), cond_n);
      }
      s.continue_target = cond_b;
      s.break_target = after_b;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, cond_b).has_explicit_terminator = true;

      ctx.break_stack.pop_back();
      ctx.continue_stack.pop_back();
      ctx.current_block = after_b;
      return;
    }
    case NK_SWITCH: {
      SwitchStmt s{};
      s.cond = lower_expr(&ctx, n->cond ? n->cond : n->left);
      const BlockId body_b = create_block(ctx);
      s.body_block = body_b;
      const BlockId after_b = create_block(ctx);
      s.break_block = after_b;
      const BlockId parent_b = ctx.current_block;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

      ctx.switch_stack.push_back({parent_b, {}, {}, {}});
      ctx.break_stack.push_back(after_b);
      ctx.current_block = body_b;
      lower_stmt_node(ctx, n->body);
      ctx.break_stack.pop_back();

      if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        GotoStmt j{};
        j.target.resolved_block = after_b;
        append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }

      if (!ctx.switch_stack.empty()) {
        auto& sw_ctx = ctx.switch_stack.back();
        for (auto& blk : ctx.fn->blocks) {
          if (blk.id.value != parent_b.value) continue;
          for (auto& stmt : blk.stmts) {
            if (auto* sw = std::get_if<SwitchStmt>(&stmt.payload)) {
              sw->case_blocks = std::move(sw_ctx.cases);
              sw->case_range_blocks = std::move(sw_ctx.case_ranges);
              if (sw_ctx.default_block) sw->default_block = sw_ctx.default_block;
              break;
            }
          }
          break;
        }
        ctx.switch_stack.pop_back();
      }

      ctx.current_block = after_b;
      return;
    }
    case NK_CASE: {
      long long case_val = 0;
      if (n->left) {
        if (n->left->kind == NK_INT_LIT) {
          case_val = n->left->ival;
        } else {
          ConstEvalEnv env{&enum_consts_, nullptr, &ctx.local_const_bindings};
          if (auto r = evaluate_constant_expr(n->left, env); r.ok()) case_val = r.as_int();
        }
      }
      if (!ctx.switch_stack.empty()) {
        const BlockId case_b = create_block(ctx);
        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = case_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }
        ctx.switch_stack.back().cases.push_back({case_val, case_b});
        ctx.current_block = case_b;
      }
      CaseStmt s{};
      s.value = case_val;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      lower_stmt_node(ctx, n->body);
      return;
    }
    case NK_CASE_RANGE: {
      long long lo = 0;
      long long hi = 0;
      {
        ConstEvalEnv env{&enum_consts_, nullptr, &ctx.local_const_bindings};
        if (n->left) {
          if (n->left->kind == NK_INT_LIT) lo = n->left->ival;
          else if (auto r = evaluate_constant_expr(n->left, env); r.ok()) lo = r.as_int();
        }
        if (n->right) {
          if (n->right->kind == NK_INT_LIT) hi = n->right->ival;
          else if (auto r = evaluate_constant_expr(n->right, env); r.ok()) hi = r.as_int();
        }
      }
      if (!ctx.switch_stack.empty()) {
        const BlockId case_b = create_block(ctx);
        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = case_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }
        ctx.switch_stack.back().case_ranges.push_back({lo, hi, case_b});
        ctx.current_block = case_b;
      }
      CaseRangeStmt s{};
      s.range.lo = lo;
      s.range.hi = hi;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      lower_stmt_node(ctx, n->body);
      return;
    }
    case NK_DEFAULT: {
      if (!ctx.switch_stack.empty()) {
        const BlockId def_b = create_block(ctx);
        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = def_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }
        ctx.switch_stack.back().default_block = def_b;
        ctx.current_block = def_b;
      }
      append_stmt(ctx, Stmt{StmtPayload{DefaultStmt{}}, make_span(n)});
      lower_stmt_node(ctx, n->body);
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

}  // namespace c4c::hir
