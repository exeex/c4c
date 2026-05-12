Status: Active
Source Idea Path: ideas/open/196_hir_pending_consteval_structured_identity.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Pending And Recursive Consteval Lookup

# Current Packet

## Just Finished

Step 1 - Inventory Pending And Recursive Consteval Lookup completed as an
inventory-only packet.

Surface ledger:

- `src/frontend/hir/hir_ir.hpp` `PendingConstevalExpr`: stores `fn_name`,
  constant arguments, rendered template bindings, rendered/TextId NTTP
  bindings, span, and deferred-unlock status. It does not currently carry the
  callee declaration, `ConstEvalStructuredNameKey`, or callee `TextId` as
  semantic identity. Classification: `fn_name` is metadata-rich authority bug
  for pending replay because producers already start from an AST callee with
  complete metadata.
- `src/frontend/hir/impl/expr/call.cpp`
  `Lowerer::try_lower_consteval_call_expr`: resolves the callee with
  `ct_state_->find_consteval_def(n->left, n->left->name)`, so direct lowering
  is already structured-authority mirror/fail-closed, but the created
  `PendingConstevalExpr` drops that identity and records only `pce.fn_name`.
  Classification: creation-site identity loss; selected implementation route.
- `src/frontend/hir/impl/compile_time/engine.cpp`
  `PendingConstevalEvalStep::run`: pre-checks
  `ct_state->has_consteval_def(pce->fn_name)` before evaluation. This is
  rendered-name semantic lookup for a metadata-rich pending object.
  Classification: metadata-rich authority bug.
- `src/frontend/hir/impl/compile_time/engine.cpp`
  `evaluate_pending_consteval`: resolves the definition with
  `ct_state.find_consteval_def(pce.fn_name)` and reports misses using
  `pce.fn_name`. The diagnostic string is acceptable display text, but the
  lookup is rendered-name semantic authority. Classification: metadata-rich
  authority bug.
- `src/frontend/hir/impl/compile_time/engine.cpp`
  successful pending replay records `ConstevalCallInfo::fn_name` and interns
  `fn_name_text_id` from that string. Because this is post-resolution call
  metadata, the current `fn_name` value is display/reporting state, but it is
  derived from the same string-only pending carrier. Classification:
  display/diagnostic after replay, dependent on fixing pending lookup.
- `src/frontend/sema/consteval.cpp` recursive `NK_CALL` evaluation:
  `lookup_consteval_function` first uses structured consteval key, then TextId,
  then rendered `consteval_fns` only when the callee has no metadata. Complete
  structured/text misses fail closed. Classification: structured-authority
  mirror with explicit no-metadata compatibility fallback.
- `src/frontend/hir/impl/compile_time/engine.cpp`
  `try_evaluate_consteval_call_expr`: uses
  `ct_state.find_consteval_def(expr->left, expr->left->name)` before invoking
  recursive evaluation maps. Classification: structured-authority mirror.
- `src/frontend/hir/impl/inspect/printer.cpp` pending and consteval-call
  printers render `fn_name`. Classification: display/diagnostic only.
- `src/frontend/hir/impl/expr/scalar_control.cpp` address-of consteval guard
  uses `ct_state_->has_consteval_def(n->left->name)` for diagnostics. This is
  out of scope for the active idea and explicitly excluded as semantic lookup
  authority. Classification: separate diagnostic/follow-up surface, not the
  Step 2 route.
- Existing `tests/frontend/frontend_hir_lookup_tests.cpp` coverage includes
  direct consteval structured lookup/fail-closed behavior, pending NTTP
  metadata retention, nested consteval recursive metadata success, and direct
  call stale rendered fallback rejection. It does not yet prove stale rendered
  fallback rejection for the pending replay route.

Selected Step 2 route:

- Convert/fence the pending replay route. Carry structured consteval callee
  identity from `Lowerer::try_lower_consteval_call_expr` into
  `PendingConstevalExpr`, then make `PendingConstevalEvalStep` and
  `evaluate_pending_consteval` resolve through structured identity when that
  metadata is complete. Preserve `fn_name` as display/no-metadata compatibility
  only. This directly matches idea 196 acceptance criteria because it covers a
  metadata-rich pending route, can fail closed on a complete structured miss
  without falling back to stale rendered registry entries, and avoids broad
  recursive interpreter rewrites where structured fencing already exists.

## Suggested Next

Step 2 should update only the pending replay route: add a structured consteval
callee carrier to `PendingConstevalExpr`, populate it at
`try_lower_consteval_call_expr`, and change pending pre-check/evaluation to use
that carrier with fail-closed behavior before allowing an explicit no-metadata
rendered compatibility fallback.

## Watchouts

The recursive interpreter route already has structured/text-first lookup and a
rendered fallback only for no-metadata calls; avoid expanding Step 2 into a
recursive evaluator rewrite unless new evidence contradicts that. The pending
route must not merely rename `fn_name`; stale rendered lookup must be fenced at
both the pre-check and `evaluate_pending_consteval` lookup.

## Proof

Inventory-only packet. No validation was run because no implementation or test
files changed; no build was required by the delegated proof.
