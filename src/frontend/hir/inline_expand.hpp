#pragma once

#include "ir.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace c4c::hir {

// ── Phase 2: ID Remapping Infrastructure ─────────────────────────────────────

/// Context for cloning callee HIR nodes into a caller with fresh IDs.
struct InlineCloneContext {
  Module* module = nullptr;
  std::unordered_map<uint32_t, LocalId> local_map;
  std::unordered_map<uint32_t, BlockId> block_map;
  std::unordered_map<uint32_t, ExprId> expr_map;
  std::string debug_prefix;

  // Phase 3: parameter index → synthetic local for argument capture.
  std::unordered_map<uint32_t, LocalId> param_to_local;

  /// Allocate a fresh LocalId mapped from the callee's local.
  LocalId remap_local(LocalId old_id);

  /// Allocate a fresh BlockId mapped from the callee's block.
  BlockId remap_block(BlockId old_id);

  /// Allocate a fresh ExprId mapped from the callee's expr.
  ExprId remap_expr(ExprId old_id);

  /// Look up a previously remapped LocalId, or return invalid if not mapped.
  [[nodiscard]] LocalId lookup_local(LocalId old_id) const;

  /// Look up a previously remapped BlockId, or return invalid if not mapped.
  [[nodiscard]] BlockId lookup_block(BlockId old_id) const;

  /// Look up a previously remapped ExprId, or return invalid if not mapped.
  [[nodiscard]] ExprId lookup_expr(ExprId old_id) const;
};

/// Clone an expression (deep: recursively clones sub-expressions).
/// Allocates fresh ExprIds and remaps internal references.
/// Returns the new ExprId of the cloned root expression.
ExprId clone_expr(InlineCloneContext& ctx, const Expr& src);

/// Clone a statement, remapping all internal IDs (locals, blocks, exprs).
Stmt clone_stmt(InlineCloneContext& ctx, const Stmt& src);

/// Clone a block (all statements), remapping all IDs.
/// Returns a new Block with a fresh BlockId.
Block clone_block(InlineCloneContext& ctx, const Block& src);

/// Pre-allocate block ID mappings for all blocks in the callee.
/// Must be called before clone_block so forward block references resolve.
void preallocate_block_ids(InlineCloneContext& ctx, const Function& callee);

// ── Phase 3: Argument Capture and Parameter Binding ──────────────────────────

/// Synthesize one LocalDecl per callee parameter in the caller.
/// Each synthetic local is initialized with the corresponding argument expression.
/// Records the mapping in ctx.param_to_local so that subsequent clone_expr calls
/// rewrite DeclRef.param_index references to the captured locals.
/// Returns the list of Stmt (LocalDecl) that should be inserted at the call site.
std::vector<Stmt> bind_callee_params(
    InlineCloneContext& ctx,
    const Function& callee,
    const CallExpr& call);

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
  ArrayParam,         // callee has array-typed parameter (codegen limitation)
  VaArgInBody,        // callee body contains va_arg (ABI-sensitive)
};

/// Return a human-readable description of the reject reason.
const char* inline_reject_reason_str(InlineRejectReason reason);

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

}  // namespace c4c::hir
