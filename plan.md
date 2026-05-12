# BIR Direct Symbol Identity Validation Closure Runbook

Status: Active
Source Idea: ideas/open/186_bir_direct_symbol_identity_validation_closure.md

## Purpose

Close the BIR direct-symbol validation boundary so generated metadata-rich
direct calls, global references, and pointer initializer symbol references use
`LinkNameId` as semantic identity whenever that metadata is available.

## Goal

Fence raw callee/global strings to display, diagnostics, runtime placeholder,
and explicit compatibility paths while making metadata-rich direct-symbol
references validate through structured ids.

## Core Rule

Generated user or extern direct-symbol references must not accept matching final
strings when their `LinkNameId` metadata is missing, invalid, or stale.
Placeholder/runtime calls and raw/no-id imports remain explicit compatibility
paths.

## Read First

- `ideas/open/186_bir_direct_symbol_identity_validation_closure.md`
- `ideas/closed/162_linknameid_backend_symbol_authority.md`
- `ideas/closed/183_lir_bir_backend_freeze_authority_audit.md`
- `src/backend/bir`
- `src/codegen/lir`

## Current Scope

- Audit BIR direct-symbol surfaces: `CallInst`, `LoadGlobalInst`,
  `StoreGlobalInst`, global declarations, function declarations, and pointer
  initializer symbol handling.
- Classify raw string fields as semantic identity, display/diagnostics,
  placeholder compatibility, or raw/no-id compatibility.
- Strengthen validation for at least direct calls and one global or
  pointer-initializer symbol reference.
- Preserve intentionally invalid-id runtime/intrinsic placeholder handling.
- Add focused BIR validation or lowering tests for success, mismatch, missing
  metadata, and compatibility behavior.

## Non-Goals

- Do not change object-file symbol spelling or assembler string tables.
- Do not replace local SSA, slot, block label, or route-local strings.
- Do not rewrite `LinkNameTable`.
- Do not retire all no-metadata raw BIR/LIR imports in this slice.
- Do not claim the final LIR/BIR freeze gate; that remains owned by idea 188.

## Working Model

- `LinkNameId` owns direct user/extern symbol identity where metadata exists.
- Raw string spellings may still be rendered for dumps, diagnostics, runtime
  placeholder calls, and explicit compatibility input.
- Existing validators may already check many id/name pairings; this runbook is
  a closure pass to find and harden remaining generated metadata-rich gaps.
- Pointer initializers are in scope when they carry structured link-name
  metadata and can accidentally fall back to final spelling.

## Execution Rules

- Keep implementation changes scoped to direct symbol identity validation and
  its direct tests.
- Treat map or field renames without authority changes as insufficient.
- Reject fixes that trust matching final strings when structured ids disagree.
- Keep placeholder/runtime calls explicitly separate from ordinary user/extern
  direct-symbol identity.
- For code-changing steps, run build proof plus focused BIR validation/lowering
  coverage selected by the supervisor.

## Ordered Steps

### Step 1: Inventory direct-symbol validation surfaces

Goal: Identify where BIR direct-symbol references carry both raw spelling and
`LinkNameId` metadata, and where validators consume those facts.

Primary targets:
- `src/backend/bir`
- `src/backend/bir/lir_to_bir`
- `src/codegen/lir`

Actions:
- Inspect `CallInst`, `LoadGlobalInst`, `StoreGlobalInst`, global
  declarations, function declarations, and pointer initializer symbol handling.
- Locate validation checks for mismatched or missing `LinkNameId` metadata.
- Classify string-only paths as generated metadata-rich, raw/no-id
  compatibility, display/diagnostics, or runtime/intrinsic placeholder.
- Record the selected hardening target and any retained compatibility routes in
  `todo.md`.

Completion check:
- `todo.md` names the relevant direct-symbol surfaces, current validation
  behavior, selected generated metadata-rich target, and explicit compatibility
  routes.

### Step 2: Strengthen metadata-rich direct-symbol validation

Goal: Make selected generated direct-symbol references fail closed on missing,
invalid, or stale `LinkNameId` metadata.

Primary targets:
- BIR validation and LIR-to-BIR direct-symbol lowering paths selected in Step 1.

Actions:
- Add or reuse a route distinction for generated metadata-rich direct-symbol
  references versus compatibility/placeholder references.
- Require `LinkNameId` validation for direct calls and at least one global or
  pointer-initializer symbol reference.
- Reject mismatched id/name pairs even when final strings match another
  reachable table entry.
- Preserve placeholder/runtime and raw/no-id compatibility behavior through
  explicit branches.

Completion check:
- The selected generated direct-symbol path validates through `LinkNameId` and
  no longer treats final spelling as fallback semantic authority.

### Step 3: Add focused direct-symbol tests

Goal: Prove structured success, mismatch rejection, missing metadata rejection,
and retained placeholder or compatibility behavior.

Primary targets:
- Existing backend BIR validation, LIR-to-BIR, and focused backend route tests.

Actions:
- Add or extend tests for direct-call `LinkNameId` success and mismatch or
  missing-id rejection.
- Add at least one global or pointer-initializer symbol reference test.
- Preserve or add a placeholder/runtime or raw/no-id compatibility case where
  invalid ids remain intentional.
- Avoid tests that only inspect printed names without exercising validation or
  lowering behavior.

Completion check:
- Focused tests fail without the strengthened direct-symbol boundary and pass
  with it.

### Step 4: Validate and prepare handoff

Goal: Produce acceptance-grade proof for idea 186 without broadening into the
final freeze gate.

Actions:
- Run build proof and the supervisor-selected focused BIR validation/lowering
  coverage.
- Escalate to a broader backend subset if the implementation crosses shared
  lowering or validation behavior beyond the selected direct-symbol path.
- Update `todo.md` with proof results, residual risks, and the recommended
  lifecycle close/switch decision.

Completion check:
- Idea 186 acceptance criteria are satisfied, and the supervisor has enough
  proof to review and commit the slice.
