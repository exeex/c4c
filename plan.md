# Global Aggregate Layout Structured Boundary Runbook

Status: Active
Source Idea: ideas/open/178_global_aggregate_layout_structured_boundary.md

## Purpose

Move one bounded global aggregate layout route away from rendered aggregate
type spelling and toward structured owner and layout identity when metadata is
available.

## Goal

Generated global aggregate loads, stores, pointer roots, or initializer-backed
layout queries should use structured layout identity through the selected
HIR/LIR/BIR/backend route, while rendered `%struct...` spelling remains output
or explicit no-metadata compatibility payload.

## Core Rule

Do not accept equal rendered global aggregate spelling as sufficient identity
when structured owner or layout metadata is present.

## Read First

- `ideas/open/178_global_aggregate_layout_structured_boundary.md`
- Completed local aggregate layout work from
  `ideas/closed/173_aggregate_layout_identity_structured_boundary.md`
- Structured LIR type-ref equality from
  `ideas/closed/176_lir_type_ref_structured_equality.md`
- Global aggregate lowering, initializer, pointer-root, and backend layout
  lookup paths that carry or consume aggregate metadata

## Current Scope

- Select one bounded global aggregate layout family, such as global struct
  array read/write, nested global aggregate pointer roots, or
  initializer-derived aggregate layout queries.
- Carry structured owner/layout identity from HIR/LIR into the selected
  BIR/backend consumer.
- Prefer structured layout facts over rendered struct/type spelling for
  metadata-rich generated inputs.
- Preserve final emitted spelling, diagnostics, and legacy no-metadata
  compatibility surfaces.
- Prove the selected route with focused global aggregate tests and fresh build
  or compile proof.

## Non-Goals

- Do not rewrite every global aggregate lowering route.
- Do not change emitted LLVM, assembly, or diagnostic spelling for cosmetic
  reasons.
- Do not remove legacy no-metadata compatibility for hand-authored or raw
  LIR/BIR inputs.
- Do not broaden into aggregate ABI classification, byval copy lowering, or
  local aggregate GEP work unless it directly blocks the selected global route.
- Do not weaken global aggregate, initializer, layout, or backend expectations.

## Working Model

- HIR/LIR can carry structured aggregate owner, type-ref, field, and layout
  facts independently from rendered type text.
- Global aggregate lowering may still keep rendered `type_decls`,
  `%struct...` names, initializer spelling, or display strings for output and
  compatibility.
- Metadata-rich generated global aggregate paths must fail closed or report a
  clear mismatch when structured layout identity is missing, stale, opaque, or
  inconsistent.
- Legacy no-metadata fixtures may keep explicit fallback behavior, but that
  fallback must not hide generated metadata mismatches.

## Execution Rules

- Keep each implementation packet scoped to the selected global aggregate
  route and immediately required helpers.
- Separate structured semantic identity from display spelling in names and data
  flow.
- Prefer semantic lowering and explicit mismatch handling over testcase-shaped
  matching.
- Add tests that exercise real global aggregate layout behavior, not only final
  printed type text.
- For code-changing packets, run fresh build or compile proof and the
  supervisor-delegated focused CTest subset.
- Escalate to review before broad global-lowering rewrites or expectation
  downgrades.

## Steps

### Step 1: Select And Map One Global Layout Route

Goal: Identify the smallest global aggregate layout route where rendered type
spelling still influences semantic layout lookup or validation.

Primary target: one generated global aggregate load, store, pointer-root, or
initializer-backed layout consumer.

Actions:

- Inspect global aggregate paths that consume rendered `type_decls`,
  `struct_defs`, `%struct...` names, initializer type text, or backend
  structured layout tables.
- Pick the smallest route where structured owner, type-ref, or layout facts
  already exist upstream and rendered spelling still influences layout
  authority.
- Identify the exact legacy text bridge that must become display-only or
  explicit no-metadata fallback.
- Record the selected route, upstream structured facts, legacy fallback, nearby
  same-feature cases, and targeted proof command family in `todo.md`.

Completion check:

- `todo.md` names concrete files/functions for the selected global route.
- The executor can state which rendered spelling dependency will stop being
  semantic authority.
- The selected proof family covers both the structured global path and a
  stale/mismatched identity case.

### Step 2: Thread Structured Layout Identity To The Global Consumer

Goal: Make the selected global consumer receive enough structured identity to
avoid using rendered aggregate spelling as semantic authority.

Primary target: the selected generated HIR/LIR/BIR/backend global aggregate
handoff.

Actions:

- Thread or reuse the relevant record owner, `LirTypeRef`, layout id, field
  metadata, initializer layout facts, or computed layout facts through the
  selected path.
- Keep rendered names and type text available only for output, diagnostics, or
  explicit compatibility data.
- Make generated metadata-rich inputs visibly distinct from legacy
  no-metadata inputs in code shape and fallback naming.
- Avoid synthesizing structured identity from rendered `%struct...` names at
  the consumer.

Completion check:

- The selected global consumer can make layout decisions from structured facts
  for generated aggregate inputs.
- Rendered aggregate spelling is no longer the primary key for the selected
  metadata-rich route.
- The change is limited to the selected route and immediately required helper
  surfaces.

### Step 3: Fail Closed On Stale Or Mismatched Global Metadata

Goal: Prevent stale, missing, opaque, or same-spelled aggregate metadata from
hiding structured layout identity problems.

Primary target: mismatch handling at the selected global aggregate consumer,
lookup, or verifier.

Actions:

- Reject, assert, or report a clear unsupported/mismatch status when generated
  metadata-rich global aggregate inputs lack the required structured layout
  facts.
- Keep legacy no-metadata fallback guarded and visibly named.
- Ensure equal rendered aggregate spelling cannot override a structured owner,
  type-ref, field, or layout mismatch.
- Preserve existing output spelling and diagnostics unless local conventions
  require a clearer mismatch message.

Completion check:

- A metadata-rich global aggregate mismatch is not accepted merely because the
  rendered type spelling matches.
- Legacy compatibility remains explicit and does not mask generated metadata
  errors.

### Step 4: Add Focused Global Aggregate Proof

Goal: Lock the selected route with tests that prove structured global layout
identity matters.

Primary target: focused backend/LIR/BIR tests for the selected global aggregate
family.

Actions:

- Add or update tests for the selected global load, store, pointer-root,
  nested aggregate, or initializer-derived layout behavior.
- Include at least one stale, missing, or same-spelled mismatch case that would
  pass if rendered spelling still decided layout identity.
- Preserve strong existing expectations for layout, offset, size, align,
  initializer, load/store, and backend behavior.
- Avoid proof that only checks printer text or final spelling.

Completion check:

- Tests fail on the old spelling-authority behavior or directly exercise the
  new explicit mismatch boundary.
- Tests cover meaningful global aggregate layout behavior without unsupported
  downgrades or weaker contracts.

### Step 5: Validate And Summarize The Boundary

Goal: Produce acceptance-quality proof for the bounded global aggregate route.

Primary target: fresh build or compile proof plus targeted backend/LIR/BIR
route coverage.

Actions:

- Run the supervisor-delegated build or compile command.
- Run the supervisor-delegated focused CTest subset for the selected global
  aggregate route.
- If the blast radius crosses multiple global or aggregate layout buckets,
  request broader validation before closure.
- Update `todo.md` with proof commands, results, residual compatibility
  fallback notes, and any separate follow-up ideas.

Completion check:

- Fresh proof is recorded in `todo.md`.
- Focused route coverage is recorded in `todo.md`.
- Remaining work is either inside this active idea or captured as a separate
  follow-up candidate.

## Completion Criteria

- One selected global aggregate route has a structured layout identity path.
- Metadata-rich global aggregate misses or mismatches do not fall back to
  rendered type text.
- Legacy no-metadata compatibility remains explicit and isolated.
- Focused backend/LIR/BIR tests prove the structured path and a stale or
  mismatched structured identity case.
- Fresh build or compile proof plus targeted backend route coverage is
  recorded in `todo.md`.

## Reviewer Reject Signals

- The route adds another rendered `%struct` parser branch as the main fix.
- Equal rendered aggregate spelling still accepts stale structured metadata.
- Metadata-rich global aggregate misses silently fall back to `type_decls`,
  `%struct...` names, initializer text, or recursive field text.
- Tests only update snippets, printer output, or final spelling without proving
  layout identity behavior.
- Tests are downgraded, marked unsupported, or weakened without explicit user
  approval.
- The slice expands into unrelated aggregate ABI, byval copy, local-slot, or
  broad backend rewrites.
