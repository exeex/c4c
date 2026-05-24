# AArch64 Scalar ALU Fallback Operand Phase Extraction Runbook

Status: Active
Source Idea: ideas/open/aarch64-codegen-03-alu-fallback-operand-phase-extraction.md

## Purpose

Extract the scalar ALU fallback operand-selection helper group into a clearer
phase-local implementation boundary without changing lowering behavior or
widening public header exposure.

## Goal

Give fallback operand selection a focused local home while preserving prepared
home lookup, same-block producer handling, generated operands, diagnostics, and
scalar ALU output.

## Core Rule

This is a behavior-preserving phase extraction only. Do not combine it with
helper absorption, header contraction, naming repair, control-publication
redesign, full scalar ALU decomposition, expectation rewrites, or semantic
lowering changes.

## Read First

- `ideas/open/aarch64-codegen-03-alu-fallback-operand-phase-extraction.md`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`

## Current Targets

- Primary target: fallback operand-selection helpers around
  `make_scalar_fallback_operand`.
- Related behavior to preserve:
  - immediate fallback operands
  - emitted scalar register reuse
  - same-block unpublished load-local source operands
  - prepared scalar load sources
  - named scalar operand fallback and diagnostics
- `alu.hpp` may be touched only if the existing public surface must stay
  aligned; do not expose private fallback concepts.
- A new or existing phase-local implementation unit is allowed only if it
  reduces fallback operand responsibility mixing without increasing header
  exposure.

## Non-Goals

- Do not decompose all of `alu.cpp`.
- Do not move target-specific instruction or register logic into generic
  layers.
- Do not alter control-publication materialization unless the current step
  first proves fallback operand selection cannot be separated without doing so.
- Do not rewrite test expectations, downgrade supported tests, or add
  testcase-shaped shortcuts.

## Working Model

Fallback operand selection is the target-local scalar phase that chooses an
operand from immediate values, emitted scalar registers, unpublished same-block
producer/load facts, prepared homes, and final named scalar fallback. The
extraction should make that decision boundary easier to locate without changing
which operand source wins.

## Execution Rules

- Keep each implementation packet small and behavior-preserving.
- Prefer internal linkage and `.cpp`-local helpers over new public declarations.
- Preserve existing call-site ordering unless a step explicitly documents why
  a pure move requires a mechanical adjustment.
- If the route requires widening `alu.hpp` with private fallback helpers, stop
  and request plan review.
- If control-publication behavior must be touched, stop and narrow the proof
  before continuing.
- For code-changing steps, run fresh build proof plus the supervisor-delegated
  focused tests before accepting the packet.

## Ordered Steps

### Step 1: Inspect Fallback Operand Ownership

Goal: identify the exact helper group that belongs to fallback operand
selection and the minimum call-site surface needed to move it.

Primary target: `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Inspect `make_scalar_fallback_operand` and every helper it depends on for
  immediate, emitted-register, same-block producer, prepared-load, and named
  scalar fallback behavior.
- Separate helpers that are purely fallback operand selection from helpers that
  are also control-publication or generic scalar ALU infrastructure.
- Record any boundary concern in `todo.md`; do not edit the source idea unless
  durable intent is wrong.
- Choose whether the first implementation packet can remain inside `alu.cpp`
  or needs a phase-local implementation unit.

Completion check:

- The executor can name the selected fallback helper group and the helpers that
  must stay outside the extraction.
- No implementation files need to change for this step unless the supervisor
  explicitly delegates inspection plus a small mechanical move.

### Step 2: Extract The Phase-Local Fallback Boundary

Goal: move only the selected fallback operand-selection implementation details
behind a local boundary.

Primary target: `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Extract the selected helper group without changing helper ordering semantics.
- Keep internal helpers private to implementation files; avoid new `alu.hpp`
  declarations unless an existing public declaration must remain aligned.
- Preserve diagnostics, prepared-home lookups, emitted-register reuse, and
  same-block producer decisions exactly.
- Stop if the move requires control-publication materialization changes.

Completion check:

- The fallback operand-selection boundary is clearer and phase-local.
- `alu.hpp` is unchanged or narrower; it is not widened with private fallback
  concepts.
- A fresh AArch64 backend build proof passes.

### Step 3: Rewire Call Sites Without Semantic Drift

Goal: make scalar ALU lowering use the extracted fallback boundary without
changing generated output.

Primary target: call sites in `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Update the scalar binary fallback call sites to use the extracted boundary.
- Keep the precedence among immediate, emitted register, same-block producer,
  prepared load source, and named scalar fallback unchanged.
- Avoid broad renames unless they prove the durable fallback concept more
  clearly and remain local to the extraction.

Completion check:

- Call sites are no broader than before.
- Diff review shows no expectation downgrades, unsupported-test conversions,
  named-case shortcuts, or lowering behavior changes.
- Fresh build proof still passes.

### Step 4: Prove Focused Behavior Preservation

Goal: verify the extraction with focused scalar ALU coverage.

Primary target: repo-native AArch64 backend and scalar test commands selected
by the supervisor.

Actions:

- Run the delegated AArch64 backend build command.
- Run focused scalar tests covering scalar ALU lowering, fallback operand
  selection, same-block producer operands, and prepared-home lookup behavior
  relevant to fallback operands.
- If any proof requires touching control-publication behavior, stop and request
  plan review before continuing.

Completion check:

- Build proof and focused scalar tests pass.
- `todo.md` records the exact proof commands and results.
- The source idea acceptance criteria are satisfied without broad scalar ALU
  decomposition.
