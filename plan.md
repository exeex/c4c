# HIR Pending Consteval Structured Identity Runbook

Status: Active
Source Idea: ideas/open/196_hir_pending_consteval_structured_identity.md

## Purpose

Carry structured consteval identity through pending and recursive consteval
evaluation so metadata-rich replay paths do not recover callees from rendered
function names.

Goal: select a pending or recursive consteval replay route, make structured
callee identity the semantic authority there, and prove complete structured
misses do not fall back to stale rendered registry entries.

## Core Rule

Do not remove diagnostic or display names. Rendered consteval names may remain
only as diagnostics, display text, or explicit no-metadata compatibility.
Metadata-rich pending or recursive replay must use structured callee identity.

## Read First

- `ideas/open/196_hir_pending_consteval_structured_identity.md`
- `ideas/closed/192_hir_compile_time_rendered_registry_api_retirement_audit.md`
- HIR compile-time and consteval implementation files that define
  `PendingConstevalExpr`, pending replay, recursive consteval evaluation, and
  rendered consteval registry lookup.
- Existing HIR tests for consteval call lowering, pending consteval replay,
  recursive consteval evaluation, and stale rendered fallback rejection.

## Scope

- Audit `PendingConstevalExpr` creation, storage, and replay.
- Audit recursive consteval evaluation entry points that can choose a
  consteval definition by rendered function name.
- Convert or fence one selected pending or recursive route so structured
  consteval identity drives lookup when metadata is complete.
- Keep rendered-name lookup only as explicit no-metadata compatibility with an
  owner and removal condition.
- Add focused HIR proof for structured success and stale rendered fallback
  rejection on the selected route.

## Non-Goals

- Do not rewrite the complete compile-time engine.
- Do not redesign template instantiation or specialization lookup.
- Do not remove diagnostic or display function names.
- Do not start backend restart work.
- Do not treat address-of consteval diagnostics as semantic lookup authority.
- Do not weaken tests, downgrade supported behavior, or mark supported paths
  unsupported to claim closure.

## Working Model

Idea 192 retired ordinary rendered registry authority for direct consteval call
lowering. This plan covers the remaining pending and recursive consteval
surface: if replay has complete structured callee metadata, lookup must use
that metadata, fail closed on complete structured misses, and avoid falling
back to rendered `fn_name` registry lookup.

## Execution Rules

- Keep source-idea edits out of routine execution; use `todo.md` for packet
  progress.
- Prefer a narrow selected route over broad compile-time engine rewrites.
- If the selected route lacks a structured carrier, record the exact carrier
  gap before implementation.
- A code-changing packet needs fresh build or focused HIR test proof. Closure
  needs focused stale-fallback evidence and milestone-appropriate validation
  or regression-guard evidence.

## Step 1 - Inventory Pending And Recursive Consteval Lookup

Goal: identify pending and recursive consteval paths where rendered names can
still act as semantic lookup authority.

Actions:
- Inspect `PendingConstevalExpr` creation, stored fields, and replay paths.
- Inspect recursive consteval evaluation entry points that call rendered-name
  lookup such as `find_consteval_def(string)` or equivalent registry maps.
- Classify each rendered name use as structured-authority mirror,
  display/diagnostic, no-metadata compatibility, or metadata-rich authority
  bug.
- Select one pending or recursive route for Step 2.

Completion check:
- `todo.md` contains a ledger of consteval lookup surfaces, classifications,
  and the selected Step 2 route.

## Step 2 - Route One Metadata-Rich Replay Through Structured Identity

Goal: make the selected route stop treating rendered function names as ordinary
semantic lookup authority when structured consteval metadata is complete.

Actions:
- Carry or consume structured consteval callee identity for the selected
  pending or recursive route.
- Require complete structured metadata to resolve through structured identity.
- Keep rendered lookup only at an explicit no-metadata compatibility boundary.
- Avoid behavior changes outside the selected route.

Completion check:
- The selected metadata-rich route uses structured consteval identity or has a
  clearly fenced compatibility boundary.
- Focused build or HIR test proof is recorded in `todo.md`.

## Step 3 - Prove Stale Rendered Fallback Rejection

Goal: demonstrate the covered route cannot recover through stale rendered
function spelling after a complete structured consteval miss.

Actions:
- Add or update focused HIR tests for structured success on the selected route.
- Add or update focused HIR tests for stale rendered fallback rejection on the
  selected route.
- Confirm the proof does not only exercise the direct call-lowering route
  already closed by idea 192.

Completion check:
- Focused tests prove structured success and fail-closed stale rendered lookup
  behavior for the covered pending or recursive route.

## Step 4 - Record Remaining Consteval Compatibility Surface

Goal: leave a closure ledger for rendered consteval lookup strings that remain
after the selected conversion or fence.

Actions:
- Record remaining rendered consteval name uses.
- For each no-metadata compatibility path, record owner, reason, and removal
  condition.
- Identify whether any remaining pending, recursive, or address-of consteval
  surface requires a separate follow-up idea.

Completion check:
- `todo.md` contains a remaining-surface ledger and any follow-up
  recommendation.

## Step 5 - Validate And Prepare Closure Evidence

Goal: prove the source idea acceptance criteria are satisfied.

Actions:
- Run the focused proof for the selected structured consteval replay route.
- Run broader validation appropriate for this HIR consteval milestone, or
  prepare matching `test_before.log` and `test_after.log` for
  regression-guard comparison.
- Record final acceptance evidence and whether idea 196 is ready for
  plan-owner close review.

Completion check:
- `todo.md` records final validation, regression status, and whether idea 196
  is ready for plan-owner close review.
