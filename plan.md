# HIR Legacy Compatibility Retirement Runbook

Status: Active
Source Idea: ideas/open/200_hir_legacy_compatibility_retirement.md

## Purpose

Retire or hard-fence HIR-owned legacy compatibility paths so HIR module state,
lowerer state, compile-time replay, and HIR-to-LIR handoff do not treat rendered
strings as a second semantic authority.

## Goal

Leave HIR metadata-rich paths governed by structured identity, with retained
rendered-name bridges limited to display/order, diagnostics, generated
route-local state, or explicit no-metadata compatibility.

## Core Rule

Do not let a complete structured miss recover through rendered `fn_name`,
`callee_name`, `legacy_tag`, module registry keys, lowerer name maps, or
consteval replay names.

## Read First

- Source intent: `ideas/open/200_hir_legacy_compatibility_retirement.md`
- Related completed prerequisites named by the source idea:
  - `ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`
  - `ideas/open/196_hir_pending_consteval_structured_identity.md`
- HIR comments and closure ledgers that mention `legacy`, `deprecated`,
  `rendered_compat`, no-metadata compatibility, or rendered lookup fallback.

## Current Scope

- Remaining rendered consteval lookup surfaces after idea 196.
- `PendingConstevalExpr::fn_name` or successor display/no-metadata state.
- HIR module registries including `struct_defs`, `fn_index`, `global_index`,
  and compile-time registry mirrors when still reachable.
- Lowerer function-context maps for locals, params, static globals, labels,
  local const bindings, and function-pointer signatures that still have rendered
  compatibility mirrors.
- `rendered_compat_*` local, param, and static-global sets.
- Constructor/member owner fallback paths that recover from rendered
  `callee_name`, `legacy_tag`, or module rendered keys.
- HIR-to-LIR owner/type/layout compatibility tags that remain secondary
  fallbacks for incomplete metadata.

## Non-Goals

- Parser or Sema source-carrier cleanup.
- BIR/backend compatibility retirement.
- Removing HIR diagnostic names, dump text, insertion-order display storage, or
  final link/output spelling.
- Rewriting the entire HIR lowerer, compile-time engine, or template system.
- Weakening tests, marking supported HIR routes unsupported, or claiming
  progress through expectation-only changes.

## Working Model

- Metadata-rich HIR paths should use declaration ids, structured owner keys,
  `TextId` metadata, `LinkNameId`, and route-local ids.
- Rendered strings may remain when they are only display/order storage,
  diagnostics, generated route-local spelling, final output spelling, or an
  explicitly documented no-metadata compatibility bridge.
- Any retained no-metadata bridge must fail closed when complete structured
  metadata is present and misses.
- Newly discovered retained bridges need a nearby source comment containing
  `legacy` or `deprecated`, the owner, the limitation, and a concrete removal
  condition.

## Execution Rules

- Prefer semantic HIR identity repair over testcase-shaped matching.
- Keep each code step behavior-preserving except for intentionally removing or
  fencing legacy rendered fallback.
- Do not expand into BIR/backend restart work before HIR evidence is complete.
- Add focused tests that prove stale rendered names cannot override structured
  HIR identity.
- For each code-changing step, run a fresh build or compile proof plus the
  narrow HIR tests selected by the supervisor.
- Escalate to broader validation when a step touches shared HIR lowering,
  compile-time replay, or HIR-to-LIR handoff behavior.

## Ordered Steps

### Step 1: Inventory Retained HIR Compatibility Paths

Goal: identify the remaining HIR rendered-name compatibility surfaces and sort
them by removal, hard-fence, or allowed display-only retention.

Primary targets:
- HIR module registries and compile-time registry mirrors.
- Pending and recursive consteval replay metadata.
- Lowerer maps for locals, params, static globals, labels, local const bindings,
  and function-pointer signatures.
- Constructor/member owner fallback paths.
- HIR-to-LIR owner/type/layout compatibility tags.

Concrete actions:
- Search HIR code for rendered lookup fallbacks and comments containing
  `legacy`, `deprecated`, `rendered_compat`, `fn_name`, `callee_name`, and
  `legacy_tag`.
- Classify each found path as metadata-rich, no-metadata compatibility,
  display/order, diagnostic, generated route-local, or final output spelling.
- Record the packet findings in `todo.md`, not in the source idea.

Completion check:
- `todo.md` names the next concrete removal or fence target and lists any
  discovered compatibility bridge that must not be silently retained.

### Step 2: Fence Metadata-Rich Module And Owner Lookups

Goal: prevent metadata-rich HIR paths from recovering through rendered module
registries or owner tags after complete structured misses.

Primary targets:
- `struct_defs`, `fn_index`, `global_index`, and compile-time registry mirrors.
- Constructor/member owner fallbacks using rendered `callee_name`,
  `legacy_tag`, or module rendered keys.

Concrete actions:
- Delete rendered fallback paths that no no-metadata caller still needs.
- Where no-metadata compatibility remains necessary, gate it behind explicit
  incomplete-metadata checks.
- Add or update nearby `legacy` or `deprecated` comments for retained bridges
  with owner, limitation, and removal condition.

Completion check:
- A metadata-rich stale rendered registry or owner name cannot override
  structured HIR identity, with focused test coverage or a documented blocker
  in `todo.md`.

### Step 3: Fence Consteval Replay And Pending Identity Names

Goal: make consteval replay and pending expression identity use structured
metadata without rendered names acting as semantic fallback.

Primary targets:
- Remaining rendered consteval lookup surfaces after idea 196.
- `PendingConstevalExpr::fn_name` or successor display/no-metadata state.
- Recursive or replay registry mirrors that still accept stale rendered names.

Concrete actions:
- Remove rendered consteval lookup fallback where structured identity is
  complete.
- Limit any retained pending function spelling to display or explicit
  no-metadata compatibility.
- Preserve diagnostics and dump readability without using spelling as a second
  lookup authority.

Completion check:
- A focused replay or pending consteval case proves stale rendered names cannot
  redirect structured identity, with build and narrow test proof recorded in
  `todo.md`.

### Step 4: Fence Lowerer Local, Param, Static, Label, And Signature Mirrors

Goal: stop lowerer function-context compatibility maps from recovering
metadata-rich locals, params, static globals, labels, local const bindings, or
function-pointer signatures by rendered spelling.

Primary targets:
- Lowerer function-context maps.
- `rendered_compat_*` local, param, and static-global sets.
- Local const binding and label compatibility maps.
- Function-pointer signature mirrors that still depend on rendered names.

Concrete actions:
- Replace metadata-rich rendered recovery with structured lookup or fail-closed
  behavior.
- Keep route-local generated names only when they are not used as semantic
  fallback.
- Document any retained no-metadata bridge with `legacy` or `deprecated`,
  owner, limitation, and removal condition.

Completion check:
- A focused lowerer route proves stale rendered local/param/static or replay
  names cannot override structured identity, with proof recorded in `todo.md`.

### Step 5: Audit HIR-To-LIR Handoff Compatibility Tags

Goal: ensure HIR-to-LIR owner/type/layout compatibility tags are not secondary
fallbacks for complete structured metadata.

Primary targets:
- HIR-to-LIR owner tags.
- Type/layout compatibility tags.
- Handoff paths that consume rendered compatibility metadata.

Concrete actions:
- Inspect handoff callers that still use rendered tags.
- Remove or hard-fence rendered compatibility when complete structured metadata
  is available.
- Leave final link/output spelling intact when it is output spelling only.

Completion check:
- `todo.md` records whether HIR-to-LIR handoff may proceed without adding new
  rendered-name recovery fallbacks, or names the exact blocker.

### Step 6: Final Ledger And Broader Validation

Goal: produce acceptance evidence for the source idea without expanding the
scope beyond HIR compatibility retirement.

Concrete actions:
- Confirm remaining rendered maps are display/order, diagnostics, generated
  route-local state, final output spelling, or explicit no-metadata
  compatibility.
- Confirm retained HIR bridges have `legacy` or `deprecated` comments with
  removal conditions.
- Run the supervisor-selected broader validation checkpoint after the final
  code slice.
- Record the final ledger in `todo.md`.

Completion check:
- Acceptance criteria in the source idea are satisfied, focused stale-name
  tests cover at least one registry/owner route and one replay/lowerer route,
  and the final ledger states whether HIR-to-LIR and frontend-to-BIR handoff
  may proceed without new rendered-name recovery fallbacks.
