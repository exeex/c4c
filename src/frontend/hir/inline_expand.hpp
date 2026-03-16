#pragma once

#include "ir.hpp"

#include <string>
#include <vector>

namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir {

// ── Phase 1: Call-site discovery and eligibility ─────────────────────────────

/// Reason an inline candidate was rejected.
enum class InlineRejectReason : uint8_t {
  None = 0,
  IndirectCall,       // callee is not a direct function reference
  CalleeMissing,      // callee not found in module fn_index
  NoBody,             // callee declaration has no blocks (extern)
  Variadic,           // callee is variadic
  SelfRecursive,      // call to the same function
  CallerIsNoinline,   // caller is marked noinline (not relevant for expansion)
  CalleeIsNoinline,   // callee is marked noinline
  NotAlwaysInline,    // callee is not marked always_inline (current policy)
};

/// Result of call-site eligibility analysis.
struct InlineCandidate {
  const Function* caller = nullptr;
  const Function* callee = nullptr;
  ExprId call_expr_id{};
  BlockId caller_block{};
  size_t stmt_index = 0;  // index within block's stmts vector
  InlineRejectReason reject = InlineRejectReason::None;

  [[nodiscard]] bool eligible() const { return reject == InlineRejectReason::None; }
};

/// Resolve the direct callee for a CallExpr, or return nullptr if indirect.
const Function* resolve_direct_callee(const Module& module, const CallExpr& call);

/// Check whether a specific call site is eligible for inline expansion.
/// If not eligible, candidate.reject describes the reason.
InlineCandidate check_inline_eligibility(
    const Module& module,
    const Function& caller,
    const CallExpr& call,
    ExprId call_expr_id,
    BlockId block_id,
    size_t stmt_index);

/// Scan all functions in the module and collect inline candidates.
/// Returns both eligible and ineligible candidates (for diagnostics).
std::vector<InlineCandidate> discover_inline_candidates(const Module& module);

/// Run the full inline expansion pass on the module (Phase 2+, stub for now).
void run_inline_expansion(Module& module);

}  // namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir
