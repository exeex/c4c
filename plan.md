# Aggregate Layout Identity Structured Boundary Runbook

Status: Active
Source Idea: ideas/open/173_aggregate_layout_identity_structured_boundary.md

## Purpose

Move one aggregate layout boundary away from rendered struct/type spelling and
toward structured record and layout identity.

## Goal

Generated aggregate layout decisions should use HIR-derived structured layout
identity through the selected LIR/BIR/backend consumer, with legacy text-keyed
compatibility kept explicit.

## Core Rule

Do not accept equal rendered aggregate spelling as sufficient identity when
structured record/layout metadata is present.

## Read First

- `ideas/open/173_aggregate_layout_identity_structured_boundary.md`
- HIR record owner, record definition, field metadata, and computed layout
  surfaces.
- Generated LIR/BIR aggregate layout handoff surfaces such as `base_tags`,
  `struct_defs`, `type_decls`, structured layout maps, aggregate field-offset
  consumers, globals, copies, and stack layout routes.

## Current Targets

- Select one bounded HIR -> LIR -> BIR/backend aggregate layout boundary.
- Prefer a route where field offsets, aggregate copies, globals, stack layout,
  or backend structured layout maps still depend on rendered struct spelling.
- Preserve final LLVM/display spelling as output, not semantic authority.

## Non-Goals

- Do not rewrite the full HIR, LIR, or BIR type system.
- Do not remove every legacy rendered layout bridge in this runbook.
- Do not change final LLVM type spelling or diagnostic display names merely
  because they are strings.
- Do not add a new `%struct...` parser as the main fix.
- Do not weaken layout, offset, size, align, copy, global, or stack tests.

## Working Model

- HIR owns the stronger aggregate identity inputs: record owner identity,
  record definitions, field metadata, owner indexes, and computed layout facts.
- LIR/BIR/backend bridges may still carry rendered compatibility data such as
  `struct_defs`, `base_tags`, `type_decls`, or final `%struct...` text.
- Metadata-rich generated paths must fail closed or report a clear mismatch
  when structured identity is missing or inconsistent.
- Legacy or hand-authored no-metadata inputs may keep explicit compatibility
  fallbacks, but those fallbacks must not hide generated metadata mismatches.

## Execution Rules

- Keep changes scoped to one aggregate layout boundary at a time.
- Separate semantic structured identity from display strings in code shape and
  naming.
- Prefer semantic lowering/generalization over testcase-shaped matching.
- Add tests that would catch equal rendered spelling with missing, stale, or
  mismatched structured layout identity.
- For code-changing steps, produce fresh build proof and targeted layout or
  backend route coverage before accepting the packet.

## Steps

### Step 1: Select And Map One Layout Boundary

Goal: identify the first concrete aggregate layout consumer to migrate.

Primary target: one generated aggregate layout handoff or consumer in the HIR
to LIR/BIR/backend route.

Actions:
- Inspect aggregate layout paths that consume `base_tags`, `struct_defs`,
  `type_decls`, backend structured layout maps, recursive aggregate field text,
  or final `%struct...` names.
- Pick the smallest boundary where structured HIR record/layout facts already
  exist upstream and rendered spelling still influences layout lookup, mirror
  checks, offsets, copies, globals, or stack layout.
- Record the selected boundary and proof route in `todo.md` before
  implementation begins.

Completion check:
- `todo.md` names the selected consumer, the upstream structured facts, the
  legacy text bridge being isolated, and the targeted proof command family.

### Step 2: Thread Structured Layout Identity To The Consumer

Goal: make the selected consumer receive enough structured identity to avoid
using rendered aggregate spelling as semantic authority.

Primary target: the chosen generated LIR/BIR/backend data handoff.

Actions:
- Thread the relevant record owner, record definition, layout id, field
  metadata, or computed layout facts through the selected generated path.
- Keep rendered names available only as display/output or explicit
  compatibility data.
- Name compatibility fallback code so generated metadata-rich inputs are
  visibly distinct from legacy/no-metadata inputs.

Completion check:
- The selected consumer can access structured layout identity for generated
  aggregate inputs without reparsing or comparing rendered struct spelling as
  the primary key.

### Step 3: Fail Closed On Metadata-Rich Mismatches

Goal: prevent stale or colliding rendered spelling from hiding structured
layout identity problems.

Primary target: mismatch handling at the selected layout consumer or verifier.

Actions:
- Reject or clearly report missing, stale, or mismatched structured layout
  metadata when the generated path should have it.
- Keep any legacy/no-metadata fallback guarded and documented by code shape,
  not by broad comments.
- Avoid synthesizing structured ids from rendered names at the comparison or
  lookup site.

Completion check:
- Equal rendered aggregate spelling is not sufficient to pass the selected
  metadata-rich layout check when structured identity is missing or mismatched.

### Step 4: Prove Layout Behavior And Collision Coverage

Goal: lock the selected boundary with focused tests that exercise real layout
behavior.

Primary target: targeted HIR record layout and backend aggregate routes for
the changed consumer.

Actions:
- Add or adjust focused tests for aggregate layout, member offsets, aggregate
  copies, global aggregate handling, stack layout, or backend structured layout
  maps as appropriate for the selected boundary.
- Include at least one case that would fail if equal rendered spelling still
  decided identity for metadata-rich inputs.
- Preserve strong existing expectations for offset, size, align, copy, global,
  and stack behavior.

Completion check:
- Targeted tests fail on the old spelling-keyed behavior and pass with the
  structured layout identity path.

### Step 5: Validate And Summarize The Boundary

Goal: make the slice acceptance-ready without broadening the active idea.

Primary target: build proof plus focused CTest or route coverage.

Actions:
- Run a fresh build or compile check.
- Run targeted layout/backend CTest or route coverage selected in Step 1.
- If the changed boundary affects multiple layout consumers, escalate to a
  broader repo-native check chosen by the supervisor.
- Update `todo.md` with proof commands, results, residual compatibility
  fallback notes, and any follow-up ideas that are separate initiatives.

Completion check:
- Proof is fresh, focused, and recorded in `todo.md`; remaining work is either
  clearly inside the active idea or captured as a separate follow-up candidate.

## Completion Criteria

- One generated aggregate layout boundary uses structured identity from HIR
  layout facts into the relevant LIR/BIR/backend consumer.
- The changed metadata-rich consumer no longer treats equal rendered aggregate
  spelling as sufficient identity.
- Legacy/no-metadata compatibility remains explicit and isolated.
- Focused tests assert meaningful layout, offset, copy, global, or stack
  behavior without weaker expectations.
- Fresh build or compile proof plus targeted CTest or route coverage is
  recorded in `todo.md`.

## Reviewer Reject Signals

- A new rendered struct-name parser branch is claimed as structured layout
  identity.
- Metadata-rich layout misses silently fall back to text-keyed `type_decls`,
  `%struct...` names, or recursive field text.
- Tests are downgraded, marked unsupported, or changed to accept weaker
  offset, size, align, copy, global, or stack behavior without explicit user
  approval.
- Helper renames or expectation rewrites are claimed as progress while the same
  spelling-keyed authority still decides layout.
- The slice expands into broad HIR/LIR/BIR rewrites without a focused layout
  boundary and focused proof route.
