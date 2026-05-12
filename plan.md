# LIR/BIR Backend Freeze Authority Audit Runbook

Status: Active
Source Idea: ideas/open/183_lir_bir_backend_freeze_authority_audit.md

## Purpose

Audit the LIR/BIR/backend-prealloc identity boundary before backend restart and
produce a freeze ledger for remaining identity-authority surfaces.

## Goal

Classify the remaining backend-facing identity surfaces so follow-up backend
work does not accidentally treat compatibility or display strings as semantic
authority on generated metadata-rich paths.

## Core Rule

This is an audit and lifecycle-classification runbook. Do not implement backend
fixes here; convert required repairs into concrete follow-up ideas or confirm
that existing open ideas already cover them.

## Read First

- `ideas/open/183_lir_bir_backend_freeze_authority_audit.md`
- `ideas/open/184_direct_call_signature_metadata_structured_boundary.md`
- `ideas/open/185_lir_to_bir_global_typedecl_compatibility_fence.md`
- `ideas/open/186_bir_direct_symbol_identity_validation_closure.md`
- `ideas/open/187_bir_memory_provenance_global_handle_cleanup.md`
- `ideas/open/188_lir_bir_freeze_closure_gate.md`
- `src/codegen/lir`
- `src/backend/bir`
- `src/backend/prealloc`

## Current Scope

- Inventory LIR/BIR/backend-prealloc identity surfaces relevant to backend
  restart.
- Classify retained strings as display/output, diagnostics, route-local
  handles, no-metadata compatibility, ABI/final spelling, or semantic
  authority that needs replacement.
- Explicitly cover direct-call signatures, direct callee identity,
  global/type declaration compatibility tables, aggregate layout facts, memory
  provenance global handles, and prealloc route-local names.
- Confirm whether open ideas 184-188 cover all required freeze blockers.

## Non-Goals

- Do not restart x86, AArch64, or target backend implementation.
- Do not rewrite LIR/BIR lowering, prealloc, assemblers, linkers, or object
  emission inside this audit.
- Do not remove printer, diagnostic, output, or route-local strings just
  because they are strings.
- Do not downgrade tests, weaken supported-path contracts, or treat expectation
  rewrites as freeze progress.

## Working Model

- `LinkNameId`, `LirTypeRef`, `StructNameId`, and structured layout facts are
  candidate semantic authorities for metadata-rich generated paths.
- Final rendered names and signature strings may remain for display, emitted
  spelling, diagnostics, compatibility fixtures, or route-local bookkeeping.
- Raw or no-metadata LIR/BIR imports may keep compatibility behavior, but that
  compatibility must be distinguishable from normal generated-path semantics.
- Audit findings should first live in `todo.md`. Durable follow-up ideas belong
  under `ideas/open/` through a plan-owner lifecycle handoff.

## Execution Rules

- Keep evidence tied to concrete files, data structures, and lowering routes.
- Separate display/output strings from semantic identity instead of marking all
  strings as defects.
- When a generated metadata-rich path can fall back to rendered text, record it
  as a blocker unless an existing open idea already owns it.
- If a missing blocker idea is discovered, stop implementation-style work and
  route it through lifecycle state rather than expanding this audit into a fix.
- Validation for this audit is source-level proof: commands may include fast
  compile or targeted tests only if the audit changes lifecycle/docs artifacts.

## Ordered Steps

### Step 1: Inventory LIR/BIR identity surfaces

Goal: Build the first freeze-ledger inventory from source inspection.

Primary targets:
- `src/codegen/lir`
- `src/backend/bir`
- `src/backend/prealloc`

Actions:
- Inspect identity-bearing structures, maps, validators, and lowering helpers.
- List surfaces for signatures, symbols, globals, type/layout facts, memory
  provenance, and prealloc route-local names.
- For each surface, record the file/function, stored spelling or id fields,
  and whether it is used by generated metadata-rich paths, raw/no-id
  compatibility, display/output, diagnostics, or route-local bookkeeping.
- Update `todo.md` with the inventory summary and any uncertain surfaces.

Completion check:
- `todo.md` contains an inventory that another agent can use without repeating
  broad source discovery.

### Step 2: Classify authority and compatibility boundaries

Goal: Decide which retained strings are acceptable and which are freeze
blockers.

Primary targets:
- Direct-call signature lowering.
- Direct call/global/pointer-initializer symbol identity.
- `GlobalTypes`, `TypeDeclMap`, and structured layout tables.
- Memory/provenance global-handle maps.
- Prealloc route-local names.

Actions:
- Classify each inspected surface as display/output, diagnostics, route-local,
  no-metadata compatibility, ABI/final spelling, or semantic authority.
- Identify generated metadata-rich paths that still parse or trust rendered
  signature, symbol, global, type, or layout spelling.
- Distinguish valid compatibility fallbacks from unowned semantic authority.
- Update `todo.md` with a freeze-ledger classification table or equivalent
  structured summary.

Completion check:
- Every source-idea identity domain is classified, and each high-risk
  generated-path fallback has an owner or blocker note.

### Step 3: Map blockers to follow-up ideas

Goal: Ensure every required freeze blocker is captured as durable follow-up
  work.

Primary targets:
- `ideas/open/184_direct_call_signature_metadata_structured_boundary.md`
- `ideas/open/185_lir_to_bir_global_typedecl_compatibility_fence.md`
- `ideas/open/186_bir_direct_symbol_identity_validation_closure.md`
- `ideas/open/187_bir_memory_provenance_global_handle_cleanup.md`
- `ideas/open/188_lir_bir_freeze_closure_gate.md`

Actions:
- Match each classified blocker to an existing open idea when the scope lines
  up.
- Record any uncovered blocker in `todo.md` with concrete reject signals and a
  suggested new idea title.
- Do not create or edit source ideas directly from routine audit notes; route
  missing durable follow-up ideas through the supervisor and plan owner.

Completion check:
- `todo.md` states either that ideas 184-188 cover the freeze blockers, or
  names the exact additional idea needed before backend restart.

### Step 4: Prepare audit closeout

Goal: Hand back a lifecycle-ready audit result without claiming backend freeze
  closure prematurely.

Actions:
- Summarize the freeze ledger in `todo.md`.
- State whether idea 183 can close, whether plan rewrite is needed, or whether
  a separate follow-up idea must be created first.
- Recommend the next lifecycle action for the supervisor.

Completion check:
- The supervisor can decide whether to close this audit idea, activate one of
  ideas 184-188, or route a missing blocker through plan-owner creation.
