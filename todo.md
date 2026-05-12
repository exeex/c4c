Status: Active
Source Idea Path: ideas/open/196_hir_pending_consteval_structured_identity.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Record Remaining Consteval Compatibility Surface

# Current Packet

## Just Finished

Step 4 - Record Remaining Consteval Compatibility Surface completed as a
todo-only closure ledger after the pending replay structured-identity
conversion.

Remaining rendered consteval name surface:

- `PendingConstevalExpr::fn_name`
  (`src/frontend/hir/hir_ir.hpp`) remains as display text and as the rendered
  no-metadata compatibility key for pending carriers whose
  `callee_identity.complete()` is false. Owner: HIR compile-time pending
  replay. Reason: old or synthetic pending consteval carriers may not have
  parser TextId/namespace/qualifier metadata, while diagnostics still need a
  readable function spelling. Removal condition: every `PendingConstevalExpr`
  producer must populate complete `PendingConstevalCalleeIdentity`, and tests
  must prove no pending replay path needs `fn_name` for semantic lookup.
- `find_pending_consteval_def`
  (`src/frontend/hir/impl/compile_time/engine.cpp`) now uses
  `pce.callee_identity` when complete and falls back to
  `ct_state.find_consteval_def(pce.fn_name)` only when that metadata is
  incomplete. Owner: HIR compile-time engine. Reason: preserve no-metadata
  pending replay compatibility while complete structured misses fail closed.
  Removal condition: retire no-metadata pending carriers or make them invalid
  at construction time.
- `ConstevalCallInfo::fn_name`, `fn_name_text_id`, pending consteval
  diagnostics, and the HIR inspector printers remain rendered/display state.
  Owner: HIR diagnostics/inspection. Reason: users and tests need stable
  readable output for evaluated or unreduced consteval calls. Removal
  condition: none required for idea 196; if replaced later, the replacement
  should be a display-name carrier derived after structured resolution, not a
  semantic lookup key.
- `CompileTimeState::find_consteval_def(std::string)`,
  `has_consteval_def(std::string)`, and `consteval_fn_defs()` remain rendered
  registry compatibility APIs. Owner: `CompileTimeState` registry boundary.
  Reason: legacy/no-metadata callers and the sema recursive interpreter still
  accept a rendered consteval map as compatibility input. Removal condition:
  migrate remaining callers to declaration identity, consteval structured
  keys, or TextId lookup, then fence or delete the string-only overloads.
- Recursive consteval evaluation in `src/frontend/sema/consteval.cpp` remains
  structured/text-first: complete structured or TextId misses fail closed, and
  the rendered `consteval_fns` map is consulted only when the callee has no
  authoritative metadata. Owner: sema consteval interpreter. Reason: recursive
  AST evaluation still supports no-metadata call nodes. Removal condition:
  require parser metadata for recursive consteval calls and drop the rendered
  fallback from `lookup_consteval_function`.
- Direct HIR recursive entry
  `try_evaluate_consteval_call_expr`
  (`src/frontend/hir/impl/compile_time/engine.cpp`) uses
  `ct_state.find_consteval_def(expr->left, expr->left->name)` and is already
  declaration/structured-authority lookup with rendered spelling only as the
  no-metadata fallback provided by `CompileTimeState`.
- Address-of consteval diagnostics in
  `Lowerer::lower_addr_expr`
  (`src/frontend/hir/impl/expr/scalar_control.cpp`) still call
  `ct_state_->has_consteval_def(n->left->name)`. Owner: HIR expression
  diagnostics. Reason: the active plan explicitly excludes treating address-of
  consteval diagnostics as semantic lookup authority. Removal condition:
  replace the diagnostic guard with declaration/structured lookup once
  address-of consteval diagnostics are in scope.

Follow-up recommendation:

- No remaining pending or recursive metadata-rich consteval lookup surface
  needs a separate follow-up idea for idea 196 closure: pending replay is
  structured for complete metadata, and recursive evaluation is already
  structured/text-first with no-metadata fallback only.
- Address-of consteval remains a diagnostic-only rendered-name surface. Create
  a separate follow-up idea only if the project wants to retire that diagnostic
  compatibility path or prove address-of consteval diagnostics fail closed on
  structured misses.

## Suggested Next

Step 5 should run the supervisor-selected focused proof and any broader
regression evidence needed for closure review, then record whether idea 196 is
ready for plan-owner close review.

## Watchouts

Do not treat the remaining rendered display fields as semantic lookup
authority. The only pending replay compatibility lookup left is the explicit
no-metadata fallback in `find_pending_consteval_def`; complete pending metadata
must continue to fail closed on structured misses.

## Proof

Documentation/todo-only packet. No validation was run because no code changed;
no build was required by the delegated proof. No new proof log was written.
