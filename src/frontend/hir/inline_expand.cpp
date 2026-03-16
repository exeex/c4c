#include "inline_expand.hpp"

namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir {

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
