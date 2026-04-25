#include "stmt.hpp"
#include "consteval.hpp"

namespace c4c::hir {

void Lowerer::lower_if_stmt(FunctionCtx& ctx, const Node* n) {
  if (n->is_constexpr) {
    const Node* cond_node = n->cond ? n->cond : n->left;
    LowererConstEvalStructuredMaps structured_maps;
    ConstEvalEnv cenv =
        make_lowerer_consteval_env(structured_maps, &ctx.local_const_bindings);
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
}

void Lowerer::lower_while_stmt(FunctionCtx& ctx, const Node* n) {
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
}

void Lowerer::lower_for_stmt(FunctionCtx& ctx, const Node* n) {
  ForStmt s{};
  if (n->init) {
    const bool init_is_decl = (n->init->kind == NK_DECL || n->init->kind == NK_BLOCK);
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
}

void Lowerer::lower_do_while_stmt(FunctionCtx& ctx, const Node* n) {
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
}

}  // namespace c4c::hir
