#include "inline_expand.hpp"

#include <cassert>

namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir {

// ── Phase 2: InlineCloneContext ──────────────────────────────────────────────

LocalId InlineCloneContext::remap_local(LocalId old_id) {
  auto [it, inserted] = local_map.try_emplace(old_id.value, LocalId::invalid());
  if (inserted) {
    it->second = module->alloc_local_id();
  }
  return it->second;
}

BlockId InlineCloneContext::remap_block(BlockId old_id) {
  auto [it, inserted] = block_map.try_emplace(old_id.value, BlockId::invalid());
  if (inserted) {
    it->second = module->alloc_block_id();
  }
  return it->second;
}

ExprId InlineCloneContext::remap_expr(ExprId old_id) {
  auto [it, inserted] = expr_map.try_emplace(old_id.value, ExprId::invalid());
  if (inserted) {
    it->second = module->alloc_expr_id();
  }
  return it->second;
}

LocalId InlineCloneContext::lookup_local(LocalId old_id) const {
  auto it = local_map.find(old_id.value);
  return it != local_map.end() ? it->second : LocalId::invalid();
}

BlockId InlineCloneContext::lookup_block(BlockId old_id) const {
  auto it = block_map.find(old_id.value);
  return it != block_map.end() ? it->second : BlockId::invalid();
}

ExprId InlineCloneContext::lookup_expr(ExprId old_id) const {
  auto it = expr_map.find(old_id.value);
  return it != expr_map.end() ? it->second : ExprId::invalid();
}

// ── clone_expr ───────────────────────────────────────────────────────────────

/// Helper: remap an ExprId that is a sub-expression reference.
/// Deep-clones the referenced expression if not yet cloned.
static ExprId remap_sub_expr(InlineCloneContext& ctx, ExprId old_id) {
  // Check if already cloned.
  ExprId existing = ctx.lookup_expr(old_id);
  if (existing.valid()) return existing;

  // Find and deep-clone the source expression.
  const Expr* src = ctx.module->find_expr(old_id);
  if (!src) return old_id;  // external expr, keep as-is
  return clone_expr(ctx, *src);
}

/// Helper: remap an optional ExprId.
static std::optional<ExprId> remap_opt_expr(InlineCloneContext& ctx,
                                            const std::optional<ExprId>& opt) {
  if (!opt) return std::nullopt;
  return remap_sub_expr(ctx, *opt);
}

/// Helper: remap a BlockId (must already be preallocated).
static BlockId remap_block_ref(InlineCloneContext& ctx, BlockId old_id) {
  if (!old_id.valid()) return old_id;
  BlockId mapped = ctx.lookup_block(old_id);
  assert(mapped.valid() && "block must be preallocated before cloning");
  return mapped;
}

/// Helper: remap an optional BlockId.
static std::optional<BlockId> remap_opt_block(InlineCloneContext& ctx,
                                              const std::optional<BlockId>& opt) {
  if (!opt) return std::nullopt;
  return remap_block_ref(ctx, *opt);
}

ExprId clone_expr(InlineCloneContext& ctx, const Expr& src) {
  ExprId new_id = ctx.remap_expr(src.id);

  Expr cloned;
  cloned.id = new_id;
  cloned.type = src.type;
  cloned.span = src.span;

  // Deep-clone payload, remapping sub-expression references.
  std::visit(
      [&](const auto& p) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, IntLiteral> ||
                      std::is_same_v<T, FloatLiteral> ||
                      std::is_same_v<T, StringLiteral> ||
                      std::is_same_v<T, CharLiteral> ||
                      std::is_same_v<T, LabelAddrExpr> ||
                      std::is_same_v<T, SizeofTypeExpr>) {
          // Leaf nodes: copy as-is.
          cloned.payload = p;
        } else if constexpr (std::is_same_v<T, DeclRef>) {
          DeclRef r = p;
          // Remap local reference if it was cloned.
          if (r.local) {
            LocalId mapped = ctx.lookup_local(*r.local);
            if (mapped.valid()) r.local = mapped;
          }
          // param_index and global refs are not remapped during inline cloning;
          // param binding is handled separately in Phase 3.
          cloned.payload = r;
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
          UnaryExpr u = p;
          u.operand = remap_sub_expr(ctx, u.operand);
          cloned.payload = u;
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
          BinaryExpr b = p;
          b.lhs = remap_sub_expr(ctx, b.lhs);
          b.rhs = remap_sub_expr(ctx, b.rhs);
          cloned.payload = b;
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
          AssignExpr a = p;
          a.lhs = remap_sub_expr(ctx, a.lhs);
          a.rhs = remap_sub_expr(ctx, a.rhs);
          cloned.payload = a;
        } else if constexpr (std::is_same_v<T, CastExpr>) {
          CastExpr c = p;
          c.expr = remap_sub_expr(ctx, c.expr);
          cloned.payload = c;
        } else if constexpr (std::is_same_v<T, CallExpr>) {
          CallExpr c = p;
          c.callee = remap_sub_expr(ctx, c.callee);
          for (auto& arg : c.args) {
            arg = remap_sub_expr(ctx, arg);
          }
          cloned.payload = c;
        } else if constexpr (std::is_same_v<T, VaArgExpr>) {
          VaArgExpr v = p;
          v.ap = remap_sub_expr(ctx, v.ap);
          cloned.payload = v;
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
          IndexExpr i = p;
          i.base = remap_sub_expr(ctx, i.base);
          i.index = remap_sub_expr(ctx, i.index);
          cloned.payload = i;
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
          MemberExpr m = p;
          m.base = remap_sub_expr(ctx, m.base);
          cloned.payload = m;
        } else if constexpr (std::is_same_v<T, TernaryExpr>) {
          TernaryExpr t = p;
          t.cond = remap_sub_expr(ctx, t.cond);
          t.then_expr = remap_sub_expr(ctx, t.then_expr);
          t.else_expr = remap_sub_expr(ctx, t.else_expr);
          cloned.payload = t;
        } else if constexpr (std::is_same_v<T, SizeofExpr>) {
          SizeofExpr s = p;
          s.expr = remap_sub_expr(ctx, s.expr);
          cloned.payload = s;
        }
      },
      src.payload);

  ctx.module->expr_pool.push_back(std::move(cloned));
  return new_id;
}

// ── clone_stmt ───────────────────────────────────────────────────────────────

Stmt clone_stmt(InlineCloneContext& ctx, const Stmt& src) {
  Stmt cloned;
  cloned.span = src.span;

  std::visit(
      [&](const auto& p) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, LocalDecl>) {
          LocalDecl d = p;
          d.id = ctx.remap_local(d.id);
          d.init = remap_opt_expr(ctx, d.init);
          d.vla_size = remap_opt_expr(ctx, d.vla_size);
          cloned.payload = d;
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
          ExprStmt s = p;
          s.expr = remap_opt_expr(ctx, s.expr);
          cloned.payload = s;
        } else if constexpr (std::is_same_v<T, InlineAsmStmt>) {
          InlineAsmStmt a = p;
          if (a.output) a.output = remap_sub_expr(ctx, *a.output);
          for (auto& inp : a.inputs) inp = remap_sub_expr(ctx, inp);
          cloned.payload = a;
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
          ReturnStmt r = p;
          r.expr = remap_opt_expr(ctx, r.expr);
          cloned.payload = r;
        } else if constexpr (std::is_same_v<T, IfStmt>) {
          IfStmt s = p;
          s.cond = remap_sub_expr(ctx, s.cond);
          s.then_block = remap_block_ref(ctx, s.then_block);
          s.else_block = remap_opt_block(ctx, s.else_block);
          s.after_block = remap_block_ref(ctx, s.after_block);
          cloned.payload = s;
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
          WhileStmt s = p;
          s.cond = remap_sub_expr(ctx, s.cond);
          s.body_block = remap_block_ref(ctx, s.body_block);
          s.continue_target = remap_opt_block(ctx, s.continue_target);
          s.break_target = remap_opt_block(ctx, s.break_target);
          cloned.payload = s;
        } else if constexpr (std::is_same_v<T, ForStmt>) {
          ForStmt s = p;
          s.init = remap_opt_expr(ctx, s.init);
          s.cond = remap_opt_expr(ctx, s.cond);
          s.update = remap_opt_expr(ctx, s.update);
          s.body_block = remap_block_ref(ctx, s.body_block);
          s.continue_target = remap_opt_block(ctx, s.continue_target);
          s.break_target = remap_opt_block(ctx, s.break_target);
          cloned.payload = s;
        } else if constexpr (std::is_same_v<T, DoWhileStmt>) {
          DoWhileStmt s = p;
          s.body_block = remap_block_ref(ctx, s.body_block);
          s.cond = remap_sub_expr(ctx, s.cond);
          s.continue_target = remap_opt_block(ctx, s.continue_target);
          s.break_target = remap_opt_block(ctx, s.break_target);
          cloned.payload = s;
        } else if constexpr (std::is_same_v<T, SwitchStmt>) {
          SwitchStmt s = p;
          s.cond = remap_sub_expr(ctx, s.cond);
          s.body_block = remap_block_ref(ctx, s.body_block);
          s.default_block = remap_opt_block(ctx, s.default_block);
          s.break_block = remap_opt_block(ctx, s.break_block);
          for (auto& [val, bid] : s.case_blocks) {
            bid = remap_block_ref(ctx, bid);
          }
          for (auto& [lo, hi, bid] : s.case_range_blocks) {
            bid = remap_block_ref(ctx, bid);
          }
          cloned.payload = s;
        } else if constexpr (std::is_same_v<T, GotoStmt>) {
          GotoStmt g = p;
          g.target.resolved_block = remap_block_ref(ctx, g.target.resolved_block);
          cloned.payload = g;
        } else if constexpr (std::is_same_v<T, IndirBrStmt>) {
          IndirBrStmt i = p;
          i.target = remap_sub_expr(ctx, i.target);
          cloned.payload = i;
        } else if constexpr (std::is_same_v<T, BreakStmt>) {
          BreakStmt b = p;
          b.target = remap_opt_block(ctx, b.target);
          cloned.payload = b;
        } else if constexpr (std::is_same_v<T, ContinueStmt>) {
          ContinueStmt c = p;
          c.target = remap_opt_block(ctx, c.target);
          cloned.payload = c;
        } else {
          // LabelStmt, CaseStmt, CaseRangeStmt, DefaultStmt: no IDs to remap.
          cloned.payload = p;
        }
      },
      src.payload);

  return cloned;
}

// ── clone_block ──────────────────────────────────────────────────────────────

Block clone_block(InlineCloneContext& ctx, const Block& src) {
  Block cloned;
  cloned.id = remap_block_ref(ctx, src.id);
  cloned.has_explicit_terminator = src.has_explicit_terminator;
  cloned.stmts.reserve(src.stmts.size());
  for (const auto& stmt : src.stmts) {
    cloned.stmts.push_back(clone_stmt(ctx, stmt));
  }
  return cloned;
}

// ── preallocate_block_ids ────────────────────────────────────────────────────

void preallocate_block_ids(InlineCloneContext& ctx, const Function& callee) {
  for (const auto& bb : callee.blocks) {
    ctx.remap_block(bb.id);
  }
}

// ── resolve_direct_callee ────────────────────────────────────────────────────

const Function* resolve_direct_callee(const Module& module, const CallExpr& call) {
  // The callee expr must be a DeclRef naming a function in fn_index.
  const Expr* callee_expr = module.find_expr(call.callee);
  if (!callee_expr) return nullptr;

  const auto* ref = std::get_if<DeclRef>(&callee_expr->payload);
  if (!ref) return nullptr;

  auto it = module.fn_index.find(ref->name);
  if (it == module.fn_index.end()) return nullptr;

  return module.find_function(it->second);
}

// ── check_inline_eligibility ─────────────────────────────────────────────────

InlineCandidate check_inline_eligibility(
    const Module& module,
    const Function& caller,
    const CallExpr& call,
    ExprId call_expr_id,
    BlockId block_id,
    size_t stmt_index) {

  InlineCandidate cand;
  cand.caller = &caller;
  cand.call_expr_id = call_expr_id;
  cand.caller_block = block_id;
  cand.stmt_index = stmt_index;

  // 1. Resolve direct callee.
  const Function* callee = resolve_direct_callee(module, call);
  if (!callee) {
    cand.reject = InlineRejectReason::IndirectCall;
    return cand;
  }
  cand.callee = callee;

  // 2. Callee must have a body (not just a declaration).
  if (callee->blocks.empty()) {
    cand.reject = InlineRejectReason::NoBody;
    return cand;
  }

  // 3. Callee must not be variadic.
  if (callee->attrs.variadic) {
    cand.reject = InlineRejectReason::Variadic;
    return cand;
  }

  // 4. Reject self-recursive calls.
  if (caller.id.value == callee->id.value) {
    cand.reject = InlineRejectReason::SelfRecursive;
    return cand;
  }

  // 5. Callee must not be noinline.
  if (callee->attrs.no_inline) {
    cand.reject = InlineRejectReason::CalleeIsNoinline;
    return cand;
  }

  // 6. Current policy: only expand always_inline callees.
  if (!callee->attrs.always_inline) {
    cand.reject = InlineRejectReason::NotAlwaysInline;
    return cand;
  }

  // All checks passed.
  cand.reject = InlineRejectReason::None;
  return cand;
}

// ── discover_inline_candidates ───────────────────────────────────────────────

/// Walk all expressions in a block's statements looking for CallExpr nodes.
/// For each found, run eligibility check.
static void scan_block_for_calls(
    const Module& module,
    const Function& fn,
    const Block& bb,
    std::vector<InlineCandidate>& out) {

  for (size_t si = 0; si < bb.stmts.size(); ++si) {
    const auto& stmt = bb.stmts[si];

    // Collect ExprIds that might contain calls.
    // We check top-level expression statements and return statements,
    // plus initializers in local declarations.
    auto check_expr = [&](ExprId eid) {
      const Expr* e = module.find_expr(eid);
      if (!e) return;
      if (const auto* call = std::get_if<CallExpr>(&e->payload)) {
        out.push_back(check_inline_eligibility(module, fn, *call, eid, bb.id, si));
      }
    };

    std::visit(
        [&](const auto& s) {
          using T = std::decay_t<decltype(s)>;
          if constexpr (std::is_same_v<T, ExprStmt>) {
            if (s.expr) check_expr(*s.expr);
          } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            if (s.expr) check_expr(*s.expr);
          } else if constexpr (std::is_same_v<T, LocalDecl>) {
            if (s.init) check_expr(*s.init);
          }
        },
        stmt.payload);
  }
}

std::vector<InlineCandidate> discover_inline_candidates(const Module& module) {
  std::vector<InlineCandidate> candidates;

  for (const auto& fn : module.functions) {
    for (const auto& bb : fn.blocks) {
      scan_block_for_calls(module, fn, bb, candidates);
    }
  }

  return candidates;
}

// ── run_inline_expansion (stub) ──────────────────────────────────────────────

void run_inline_expansion(Module& /*module*/) {
  // Phase 2+ will implement actual expansion.
  // For now, this is a no-op entry point for pipeline integration.
}

}  // namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir
