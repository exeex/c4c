# HIR Legacy Compatibility Retirement

Status: Open
Created: 2026-05-12

Depends On:
- `ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`
- `ideas/open/196_hir_pending_consteval_structured_identity.md`

## Goal

Retire or hard-fence the remaining HIR-owned legacy compatibility paths so HIR
module state, lowerer state, compile-time replay, and HIR-to-LIR handoff do not
treat rendered strings as a second semantic authority.

## Why This Idea Exists

HIR has already moved many registries and lookup paths toward declaration ids,
structured owner keys, `TextId` metadata, `LinkNameId`, and route-local ids.
Some rendered maps remain as display/order storage, diagnostics, generated-name
state, no-metadata compatibility, or legacy lowerer mirrors.

Idea 196 specifically covers pending and recursive consteval structured
identity. After that route and the frontend-to-BIR closure gate are complete,
this idea should clean the broader HIR compatibility residue and leave only
deliberate display/diagnostic/final-output spelling.

## In Scope

- Inventory HIR retained compatibility paths already called out by source
  comments and closure ledgers, including:
  - remaining rendered consteval lookup surfaces after idea 196
  - `PendingConstevalExpr::fn_name` or successor display/no-metadata state
  - rendered HIR module registries such as `struct_defs`, `fn_index`,
    `global_index`, and compile-time registry mirrors when still reachable
  - lowerer function-context maps for locals, params, static globals, labels,
    local const bindings, and function-pointer signatures that still have
    rendered compatibility mirrors
  - `rendered_compat_*` local, param, and static-global sets
  - constructor/member owner fallback paths that recover from rendered
    `callee_name`, `legacy_tag`, or module rendered keys
  - HIR-to-LIR owner/type/layout compatibility tags that remain secondary
    fallbacks for incomplete metadata
- Delete HIR rendered lookup paths that no production no-metadata caller still
  needs after ideas 195 and 196.
- For retained HIR no-metadata compatibility, require explicit fail-closed
  behavior for complete structured misses.
- Add focused HIR tests for stale rendered registry, owner, local/param, or
  consteval replay names.
- When a newly discovered HIR compatibility path cannot be removed in this
  idea, add a nearby source comment containing `legacy` or `deprecated`, plus
  owner, limitation, and removal condition.

## Out Of Scope

- Parser or Sema source-carrier cleanup.
- BIR/backend compatibility retirement.
- Removing HIR diagnostic names, dump text, insertion-order display storage, or
  final link/output spelling.
- Rewriting the entire HIR lowerer, compile-time engine, or template system.
- Weakening tests or marking supported HIR routes unsupported.

## Acceptance Criteria

- HIR metadata-rich lookup paths do not recover through rendered module
  registries, lowerer maps, owner tags, or consteval names after complete
  structured misses.
- Remaining HIR rendered maps are limited to display/order, diagnostics,
  generated route-local state, or explicit no-metadata compatibility.
- Newly discovered retained HIR bridges have source comments containing
  `legacy` or `deprecated` and a concrete removal condition.
- Focused HIR tests prove stale rendered names cannot override structured HIR
  identity in at least one registry/owner route and one replay/lowerer route.
- The final ledger states whether HIR-to-LIR and frontend-to-BIR handoff may
  proceed without adding new rendered-name recovery fallbacks.

## Reviewer Reject Signals

- A slice claims HIR compatibility retirement while a metadata-rich path still
  falls through to rendered `fn_name`, `callee_name`, `legacy_tag`, module
  registry keys, or lowerer name maps.
- A retained HIR bridge is renamed but not fenced as no-metadata,
  display/order, diagnostic, or generated route-local state.
- A newly discovered retained HIR bridge lacks a `legacy` or `deprecated`
  comment.
- Tests only update dump text or rendered spelling expectations.
- The route expands into BIR/backend restart work before HIR compatibility
  retirement evidence is complete.
