# HIR Constructor/Member Owner Structured Lookup Closure Runbook

Status: Active
Source Idea: ideas/open/193_hir_constructor_member_owner_structured_lookup_closure.md

## Purpose

Close one remaining HIR object/member lookup path where metadata-rich
constructor or aggregate/member handling can still recover record owner
identity from rendered callee or tag spelling.

Goal: select one metadata-rich constructor/member route, make structured owner
identity the semantic authority there, and prove complete structured owner
misses do not silently fall back to rendered owner tags.

## Core Rule

Do not remove display tags or diagnostic spelling. This plan is about owner
lookup authority: metadata-rich constructor/member paths must use structured
owner identity, while rendered tag fallback may remain only as explicit
no-owner/no-metadata compatibility.

## Read First

- `ideas/open/193_hir_constructor_member_owner_structured_lookup_closure.md`
- `src/frontend/hir/impl/expr/object.cpp`
- Nearby HIR object/member helper APIs that resolve struct constructors,
  aggregate owners, or member owners.
- Existing HIR tests covering direct struct constructor calls, aggregate
  construction, and member access/lowering.
- Closed dependency notes:
  `ideas/closed/177_template_record_owner_structured_identity.md` and
  `ideas/closed/182_type_identity_migration_closure_gate.md`.

## Scope

- Audit direct struct constructor lowering and aggregate/member owner lookup.
- Classify rendered owner spelling uses as structured-authority mirrors,
  display/diagnostic text, no-owner compatibility, or metadata-rich authority
  bugs.
- Convert or fence one selected metadata-rich route so structured owner
  identity drives lookup.
- Add focused HIR proof for structured success and stale rendered fallback
  rejection on the selected route.
- Record any remaining no-owner compatibility path and its removal condition.

## Non-Goals

- Do not rewrite all object/member lowering.
- Do not change overload resolution policy.
- Do not remove final display tags from `HirStructDef`.
- Do not reopen parser tag parsing unless a concrete missing owner carrier is
  found.
- Do not weaken tests, downgrade supported behavior, or mark supported paths
  unsupported to claim closure.

## Working Model

Rendered owner tags are compatibility mirrors. Metadata-rich callers should
resolve constructor/member owners through structured record owner metadata. The
safe outcomes are structured owner lookup success, explicit no-owner
compatibility, or fail-closed behavior for complete structured misses.

## Execution Rules

- Keep source-idea edits out of routine execution; use `todo.md` for packet
  progress.
- Prefer a narrow selected route over broad object-lowering rewrites.
- If the selected route reveals a missing owner carrier, record the exact
  carrier gap before implementation.
- A code-changing packet needs fresh build or focused HIR test proof. Closure
  needs focused stale-fallback evidence and milestone-appropriate validation or
  regression-guard evidence.

## Step 1 - Inventory Constructor And Member Owner Authority

Goal: identify HIR object/member paths where rendered owner spelling can still
act as semantic authority.

Actions:
- Inspect `src/frontend/hir/impl/expr/object.cpp` and nearby helper APIs for
  uses of `callee_name`, `legacy_tag`, `module_->struct_defs`, rendered struct
  tags, and member owner strings.
- Classify each use as structured-authority mirror, display/diagnostic,
  no-owner/no-metadata compatibility, route-local handle, or metadata-rich
  authority bug.
- Select one constructor/member route for Step 2.

Completion check:
- `todo.md` contains a ledger of owner lookup surfaces, classifications, and
  the selected Step 2 route.

## Step 2 - Route One Metadata-Rich Owner Lookup Through Structured Identity

Goal: make the selected route stop treating rendered owner tags as ordinary
semantic lookup authority when structured owner metadata is complete.

Actions:
- Carry or consume structured record owner identity for the selected
  constructor/member route.
- Require complete structured owner metadata to resolve through structured
  identity.
- Keep rendered fallback only at an explicit no-owner/no-metadata compatibility
  boundary.
- Avoid behavior changes outside the selected route.

Completion check:
- The selected metadata-rich route uses structured owner identity or has a
  clearly fenced compatibility boundary.
- Focused build or HIR test proof is recorded in `todo.md`.

## Step 3 - Prove Stale Rendered Owner Fallback Rejection

Goal: demonstrate the covered route cannot recover through stale rendered
owner spelling after a complete structured owner miss.

Actions:
- Add or update focused HIR tests for structured success and stale rendered
  fallback rejection on the selected route.
- Preserve diagnostic/display spelling behavior.
- Confirm the proof does not rely only on simple unqualified constructor names.

Completion check:
- Focused tests prove structured success and fail-closed stale rendered owner
  lookup behavior for the covered route.

## Step 4 - Record Remaining Compatibility Surface

Goal: leave a closure ledger for owner lookup strings that remain after the
selected conversion or fence.

Actions:
- Record remaining rendered owner uses in object/member lowering.
- For each remaining no-owner/no-metadata compatibility path, record owner,
  reason, and removal condition.
- Identify whether any remaining surface requires a separate follow-up idea.

Completion check:
- `todo.md` contains a remaining-surface ledger and any follow-up
  recommendation.

## Step 5 - Validate And Prepare Closure Evidence

Goal: prove the source idea acceptance criteria are satisfied.

Actions:
- Run the focused proof for the selected structured owner route.
- Run broader validation appropriate for this HIR object/member milestone, or
  prepare matching `test_before.log` and `test_after.log` for regression-guard
  comparison.
- Record final acceptance evidence and whether idea 193 is ready for
  plan-owner close review.

Completion check:
- `todo.md` records final validation, regression status, and whether idea 193
  is ready for plan-owner close review.
