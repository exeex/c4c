#include "stmt.hpp"
#include "consteval.hpp"

namespace c4c::hir {

void Lowerer::attach_switch_cases(FunctionCtx& ctx, BlockId parent_b) {
  if (ctx.switch_stack.empty()) return;

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
}

void Lowerer::branch_to_block_if_needed(FunctionCtx& ctx,
                                        BlockId target,
                                        const Node* n) {
  if (ensure_block(ctx, ctx.current_block).has_explicit_terminator) return;
  GotoStmt j{};
  j.target.resolved_block = target;
  append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
  ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
}

void Lowerer::lower_switch_stmt(FunctionCtx& ctx, const Node* n) {
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

  branch_to_block_if_needed(ctx, after_b, n);
  attach_switch_cases(ctx, parent_b);
  ctx.switch_stack.pop_back();

  ctx.current_block = after_b;
}

void Lowerer::lower_case_stmt(FunctionCtx& ctx, const Node* n) {
  long long case_val = 0;
  if (n->left) {
    if (n->left->kind == NK_INT_LIT) {
      case_val = n->left->ival;
    } else {
      LowererConstEvalStructuredMaps structured_maps;
      ConstEvalEnv env = make_lowerer_consteval_env(
          structured_maps, &ctx.local_const_bindings, false);
      if (auto r = evaluate_constant_expr(n->left, env); r.ok()) case_val = r.as_int();
    }
  }
  if (!ctx.switch_stack.empty()) {
    const BlockId case_b = create_block(ctx);
    branch_to_block_if_needed(ctx, case_b, n);
    ctx.switch_stack.back().cases.push_back({case_val, case_b});
    ctx.current_block = case_b;
  }
  CaseStmt s{};
  s.value = case_val;
  append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
  lower_stmt_node(ctx, n->body);
}

void Lowerer::lower_case_range_stmt(FunctionCtx& ctx, const Node* n) {
  long long lo = 0;
  long long hi = 0;
  {
    LowererConstEvalStructuredMaps structured_maps;
    ConstEvalEnv env = make_lowerer_consteval_env(
        structured_maps, &ctx.local_const_bindings, false);
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
    branch_to_block_if_needed(ctx, case_b, n);
    ctx.switch_stack.back().case_ranges.push_back({lo, hi, case_b});
    ctx.current_block = case_b;
  }
  CaseRangeStmt s{};
  s.range.lo = lo;
  s.range.hi = hi;
  append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
  lower_stmt_node(ctx, n->body);
}

void Lowerer::lower_default_stmt(FunctionCtx& ctx, const Node* n) {
  if (!ctx.switch_stack.empty()) {
    const BlockId def_b = create_block(ctx);
    branch_to_block_if_needed(ctx, def_b, n);
    ctx.switch_stack.back().default_block = def_b;
    ctx.current_block = def_b;
  }
  append_stmt(ctx, Stmt{StmtPayload{DefaultStmt{}}, make_span(n)});
  lower_stmt_node(ctx, n->body);
}

}  // namespace c4c::hir
