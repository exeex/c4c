# BIR Memory Provenance Global Handle Cleanup Runbook

Status: Active
Source Idea: ideas/open/187_bir_memory_provenance_global_handle_cleanup.md

## Purpose

Clean up BIR memory/provenance global-handle tables so global identity is
`LinkNameId`-backed where generated metadata is available, while preserving
route-local handles and explicit compatibility strings.

## Goal

Prevent memory/provenance lowering from becoming a second string-keyed semantic
symbol table for globals after structured link-name metadata exists.

## Core Rule

Generated global provenance paths must not accept raw global spelling as
semantic authority when `LinkNameId` metadata is present, stale, or missing
where required. Local slots, local SSA names, temporaries, and explicit raw
compatibility imports may remain string-keyed only when they are route-local or
compatibility-only.

## Read First

- `ideas/open/187_bir_memory_provenance_global_handle_cleanup.md`
- `ideas/closed/183_lir_bir_backend_freeze_authority_audit.md`
- `ideas/closed/185_lir_to_bir_global_typedecl_compatibility_fence.md`
- `ideas/closed/186_bir_direct_symbol_identity_validation_closure.md`
- `src/backend/bir/lir_to_bir/memory`
- `src/backend/bir/lir_to_bir`
- `src/codegen/lir`

## Current Scope

- Audit memory/provenance maps under `src/backend/bir/lir_to_bir/memory`.
- Classify each string-keyed handle as route-local SSA/slot state, global
  symbol identity, display/diagnostics, or raw/no-id compatibility.
- Convert one or more selected global-provenance paths to structured
  `LinkNameId` authority, or add fail-closed validation when id metadata is
  missing or stale.
- Keep local slot, local SSA, temporary, and raw-import compatibility strings
  where they are truly route-local.
- Add focused backend tests for global pointer/address provenance with matching
  and mismatched symbol metadata.

## Non-Goals

- Do not rewrite all memory lowering or pointer analysis.
- Do not remove route-local local variable, slot, or temporary names.
- Do not change final BIR dump spelling.
- Do not redesign backend alias analysis broadly.
- Do not claim the final LIR/BIR freeze gate; that remains owned by idea 188.

## Working Model

- `LinkNameId` owns global symbol identity where generated metadata carries it.
- Raw global spelling can still name explicit compatibility inputs and
  route-local handles, but those branches must be distinguishable from
  generated metadata-rich global provenance.
- Memory/provenance maps may legitimately use strings for locals and
  temporaries; only global symbol identity is the target of this cleanup.
- The selected path should be narrow enough to prove the boundary without
  pulling in unrelated memory-lowering redesign.

## Execution Rules

- Start with classification before changing code; do not treat every
  string-keyed map as a bug.
- Prefer semantic `LinkNameId` validation or keying for generated global
  provenance over helper renames or dump-only changes.
- Preserve explicit route-local and raw/no-id compatibility paths through
  named branches or clear predicates.
- Reject fixes that accept mismatched global ids because final spelling still
  matches.
- For code-changing steps, run build proof plus focused backend
  memory/provenance coverage selected by the supervisor.

## Ordered Steps

### Step 1: Inventory memory/provenance global-handle surfaces

Goal: Identify which memory/provenance maps and handles are route-local and
which can represent global symbol identity.

Primary targets:
- `src/backend/bir/lir_to_bir/memory`
- `src/backend/bir/lir_to_bir`
- `src/codegen/lir`

Actions:
- Inspect global pointer, global address, local slot, pointer alias, dynamic
  array, and aggregate/scalar access tracking surfaces.
- Locate any path where a global name or handle is derived from final spelling
  after `LinkNameId` metadata is available.
- Classify string-keyed maps as route-local SSA/slot, global symbol identity,
  display/diagnostics, or raw/no-id compatibility.
- Record the selected Step 2 hardening target and retained compatibility routes
  in `todo.md`.

Completion check:
- `todo.md` names the relevant memory/provenance maps, their classification,
  the selected global-provenance target, and compatibility boundaries.

### Step 2: Fence selected global provenance identity

Goal: Make the selected generated global-provenance path use `LinkNameId`
authority or fail closed when structured metadata is missing or stale.

Primary targets:
- The memory/provenance lowering path selected in Step 1.

Actions:
- Add or reuse a route distinction between generated global provenance and
  route-local or raw/no-id compatibility handles.
- Convert the selected global-provenance key or validation check to use
  structured link-name metadata where available.
- Reject stale or mismatched global ids even when raw final spelling matches a
  known global.
- Preserve local slot, local SSA, temporary, and explicit compatibility paths.

Completion check:
- The selected generated global-provenance path no longer treats raw global
  spelling as ordinary semantic authority when structured metadata applies.

### Step 3: Add focused global provenance tests

Goal: Prove structured success, stale or missing global-id rejection, and
retained route-local or compatibility behavior.

Primary targets:
- Existing backend memory/provenance, BIR validation, or LIR-to-BIR tests.

Actions:
- Add or extend tests for global pointer/address provenance with matching
  structured metadata.
- Add stale, mismatched, or missing global-id rejection coverage for the
  selected generated path.
- Preserve or add a local route-handle or raw/no-id compatibility case where
  string-keying remains intentional.
- Avoid tests that only inspect printed names without exercising lowering or
  validation behavior.

Completion check:
- Focused tests fail without the strengthened global-provenance boundary and
  pass with it.

### Step 4: Validate and prepare handoff

Goal: Produce acceptance-grade proof for idea 187 without broadening into the
final freeze gate.

Actions:
- Run build proof and the supervisor-selected focused backend
  memory/provenance coverage.
- Escalate to the broader backend subset if the implementation crosses shared
  lowering or validation behavior beyond the selected path.
- Update `todo.md` with proof results, residual risks, and the recommended
  lifecycle close/switch decision.

Completion check:
- Idea 187 acceptance criteria are satisfied, and the supervisor has enough
  proof to review and commit the slice.
