# RV64 Packed Bitfield Global Layout And Access Lowering Runbook

Status: Active
Source Idea: ideas/open/651_rv64_packed_bitfield_global_layout_access.md

## Purpose

Repair RV64 global-object materialization for packed bitfield aggregates whose
semantic storage size is not a whole-word lane shape.

## Goal

Make the RV64 backend preserve the 9-byte packed file-scope struct layout used
by `src/pr79737-2.c`, and lower the corresponding global bitfield accesses
through that packed representation instead of the current 12-byte word-lane
model.

## Core Rule

Global packed bitfield layout and access lowering must come from explicit
layout and access semantics. Do not infer success from symbol names, testcase
identity, runtime outcomes alone, expectation changes, unsupported markers, or
pass/fail accounting.

## Read First

- `ideas/open/651_rv64_packed_bitfield_global_layout_access.md`
- `ideas/closed/642_rv64_global_residual_runtime_mismatch_research.md`
- `docs/runtime_mismatch_ownership/04_global_residual_runtime_mismatch.md`

## Current Scope

- Representative surface: `tests/c/external/gcc_torture/src/pr79737-2.c`.
- First target: file-scope packed bitfield globals `i` and `j` as semantic
  9-byte objects.
- Access target: RV64 global bitfield loads and stores that preserve the packed
  byte-oriented representation.

## Non-Goals

- Do not make generic runtime-support changes.
- Do not reopen direct global-symbol local-memory admission from idea 631.
- Do not perform broad ABI, call-lowering, branch/control-flow, relocation, or
  stack-layout rewrites.
- Do not special-case `src/pr79737-2.c`, the global names `i` or `j`, or exact
  source spelling.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Working Model

Idea 642 classified `src/pr79737-2.c` as a semantic global-object mismatch, not
a runtime-support gap. The control path lays the packed file-scope aggregate
out as 9 bytes and uses byte-oriented accesses; C4C currently materializes the
same aggregate as a 12-byte word-lane object, so generated values diverge.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Start with refreshed evidence for layout, emitted globals, BIR/MIR access
  shape, RV64 object output, and runtime behavior for `src/pr79737-2.c`.
- Identify the first owned boundary before implementation: global object
  layout size, bitfield access lowering, or another precise producer/consumer
  gap.
- Add focused positive coverage for packed global layout/access semantics and
  focused fail-closed coverage for unsupported or incomplete authority shapes.
- Preserve rejection of incomplete packed layout facts rather than silently
  choosing word-lane storage.
- If refreshed evidence proves a distinct owner outside this idea, record it
  in `todo.md` and request lifecycle split or park instead of broadening this
  runbook.

## Steps

### Step 1: Refresh Packed Global Evidence

Goal: Confirm the current first owner for `src/pr79737-2.c`.

Actions:

- Capture C4C and control evidence for the packed globals' size, alignment,
  section emission, and relevant global access operations.
- Probe prepared BIR, semantic BIR, MIR or RV64 object emission paths as needed
  to identify where the 9-byte semantic layout becomes a 12-byte word-lane
  object.
- Record whether the first boundary is global layout, load/store lowering, or a
  different owner.

Completion check:

- `todo.md` states the current first owner, the exact evidence paths, and the
  next implementation or split boundary.

### Step 2: Locate The Layout And Access Boundary

Goal: Find the narrow producer or consumer surface that should own packed
global layout and access lowering.

Actions:

- Trace how packed bitfield global size and lane information are represented
  before RV64 emission.
- Trace how global bitfield loads and stores choose byte-lane versus word-lane
  accesses.
- Identify focused positive and negative test shapes for one complete packed
  global layout/access rule.
- Name fail-closed behavior for missing packed layout facts, incomplete access
  identity, unsupported bitfield spans, or mismatched storage size.

Completion check:

- `todo.md` records the owned implementation surface, test target shape, and
  rejection behavior to preserve.

### Step 3: Implement The Narrow Packed Global Owner

Goal: Repair one proven packed global layout/access owner without broad backend
rewrites.

Actions:

- Preserve 9-byte file-scope packed aggregate layout when explicit layout
  authority proves the packed object size.
- Lower the selected packed global bitfield loads and stores through byte-lane
  access or an equivalent semantic representation.
- Add focused positive coverage for the selected packed global shape.
- Add or preserve negative coverage for unsupported or incomplete layout/access
  authority.

Completion check:

- Fresh build plus focused proof passes, or lifecycle state records the exact
  split owner and parks this route.

### Step 4: Prove Representative Integration

Goal: Show `src/pr79737-2.c` advances through the packed global mismatch.

Actions:

- Rerun focused coverage plus the RV64 GCC torture backend route for
  `src/pr79737-2.c`.
- Capture object or dump evidence showing both packed globals use the 9-byte
  representation and the relevant accesses preserve packed semantics.
- Record any distinct downstream owner if the representative advances but does
  not fully pass.

Completion check:

- `todo.md` records representative proof and any remaining downstream owner.

### Step 5: Run Broader Validation And Close Or Park

Goal: Decide whether the source idea is complete after focused and
representative proof.

Actions:

- Run the supervisor-selected broader validation for the affected RV64 backend
  scope after focused proof is green.
- If acceptance criteria are satisfied, request plan-owner close with
  regression-guard proof.
- If a distinct downstream owner remains, record it in `todo.md` and request a
  lifecycle split or park decision.

Completion check:

- Lifecycle state either closes the source idea with passing guard proof or
  records a precise blocked or follow-up owner without broadening this idea.
