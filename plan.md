# Phase F1 riscv Route 5/Route 3 edge-publication adapter

Status: Active
Source Idea: ideas/open/245_phase_f1_riscv_route5_route3_edge_publication_adapter.md

## Purpose

Move riscv prepared edge-publication consumption behind a narrow adapter that can
compare Route 5 edge/source identity and Route 3 memory-source identity against
the existing prepared publication authority.

Goal: consume route-native facts only when they agree with current prepared
riscv answers, while preserving exact wrapper output, statuses, fallback, and
target-local policy.

## Core Rule

Route 5 and Route 3 may provide semantic identity evidence. riscv still owns
register names, stack offsets, scratch registers, signed-12-bit decisions,
load/address spelling, formatting, and wrapper emission policy.

## Read First

- `ideas/open/245_phase_f1_riscv_route5_route3_edge_publication_adapter.md`
- `src/backend/mir/riscv/codegen/emit.hpp`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- riscv prepared edge-publication consumption:
  `consume_edge_publication_move_intent()` and
  `append_edge_publication_move_instruction()`.
- Route 5 edge/source identity comparison for scalar, register, stack, and
  immediate publication sources where comparable route facts exist.
- Route 3 source-memory identity comparison for prepared publications that
  require memory-source access.
- Compatibility status surface:
  `EdgePublicationMoveIntentStatus` and prepared publication/source-memory
  status strings.

## Non-Goals

- Do not delete, privatize, or replace whole `edge_publications`,
  `edge_publication_source_producers`, `memory_accesses`,
  `PreparedFunctionLookups`, or `PreparedBirModule` surfaces.
- Do not move riscv ABI, register allocation, stack layout, scratch selection,
  offset decisions, instruction spelling, formatting, or emission policy into
  BIR.
- Do not rename `EdgePublicationMoveIntentStatus` or prepared status strings.
- Do not weaken expectations, downgrade supported cases to unsupported,
  relabel wrapper output, mask timeouts, or refresh baselines as proof.
- Do not claim draft 155 or aggregate prepared-field demotion readiness.

## Working Model

- The prepared publication answer remains the compatibility authority.
- The adapter must fail closed to the prepared behavior when route facts are
  missing, incomplete, duplicate/conflicting, unsupported, or non-agreeing.
- Route agreement may enable the same already-supported riscv output, but must
  not introduce new policy-sensitive emission.
- Positive proof must check exact instruction text for existing accepted cases.
- Negative proof must include policy-sensitive cases and status preservation,
  not only absence of emitted text.

## Execution Rules

- Keep source-idea edits unnecessary unless source intent changes.
- Prefer small code packets with one delegated proof command each.
- For each code-changing step, build the touched test targets before running
  the selected CTest subset.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  matching regression-guard before/after pair.
- Treat testcase-shaped matching, expectation weakening, and status relabeling
  as route drift.

## Steps

### Step 1: Establish the riscv Route 5/Route 3 baseline

Goal: identify current riscv prepared edge-publication behavior and the route
fact surfaces available for agreement-gating.

Actions:

- Inspect the riscv edge-publication consumer, helper/oracle tests, and any
  Route 5/Route 3 helper APIs relevant to edge/source and memory-source facts.
- Record the current positive cases: register moves, immediates, stack loads,
  stack destinations, memory sources, pointer-base address formation, large
  stack offsets, and same-register moves.
- Record the current fail-closed cases: missing shared lookups, missing
  publication, unsupported publication, unsupported source home, unsupported
  destination home, unsupported subword/unsigned/F32, dynamic stack,
  aggregate-width, non-move, pointer-base-as-address, and large-offset policy
  cases.
- Identify the narrow adapter seam and first proof subset for implementation.

Completion check:

- `todo.md` names the adapter seam, the route helper surfaces, the preserved
  status/output contracts, and the first build/test subset.

### Step 2: Add route diagnostic/oracle coverage before authority changes

Goal: make Route 5/Route 3 agreement, fallback, and conflict behavior visible
before changing riscv wrapper authority.

Actions:

- Add or extend helper-oracle diagnostics/tests for positive Route 5 edge/source
  identity and Route 3 memory-source identity rows.
- Cover missing, mismatch, duplicate/conflict, incomplete memory-source, and
  fallback cases.
- Preserve current riscv prepared wrapper output and status expectations.

Completion check:

- The delegated build plus targeted tests pass.
- The tests fail for route fact absence, mismatch, duplicate/conflict, or
  incomplete memory-source evidence without relying on changed wrapper output.

### Step 3: Introduce the riscv adapter boundary

Goal: isolate prepared edge-publication consumption behind a route-aware adapter
without changing externally visible behavior.

Actions:

- Extract the adapter boundary around prepared edge-publication lookup and
  source-home interpretation.
- Preserve `EdgePublicationMoveIntentStatus`, `EdgePublicationMoveIntent`, and
  exact instruction text.
- Label or structure fallback paths so missing or unusable route facts remain
  visibly prepared-backed.

Completion check:

- The delegated riscv edge-publication and prepared helper subset passes with
  no output/status changes.

### Step 4: Agreement-gate Route 5 edge/source identity

Goal: allow riscv to consume Route 5 edge/source identity only when it agrees
with prepared publication/source answers.

Actions:

- Compare Route 5 source and destination identity with the prepared
  edge-publication row for scalar, register, immediate, and stack/register
  publication sources where comparable facts exist.
- Fail closed to prepared fallback on missing, incomplete, duplicate/conflicting,
  or non-agreeing Route 5 facts.
- Do not let Route 5 choose riscv registers, stack slots, scratch registers,
  offsets, instruction spelling, or wrapper emission.

Completion check:

- Positive cases preserve exact riscv instruction text.
- Negative and conflict cases preserve current statuses and fallback behavior.

### Step 5: Agreement-gate Route 3 memory-source identity

Goal: allow riscv to consume Route 3 memory-source identity only when it agrees
with prepared memory-source answers.

Actions:

- Compare Route 3 memory-source identity for prepared publications that require
  source-memory access.
- Preserve prepared handling for pointer-base, large-offset, dynamic-stack,
  aggregate-width, subword, unsigned, F32, and policy-sensitive cases.
- Keep address/load spelling and scratch policy in riscv.

Completion check:

- Existing memory-source, pointer-base, large-offset, and fail-closed cases
  retain exact output/status behavior.
- Route 3 absence, mismatch, conflict, or incomplete evidence does not enable
  new emission.

### Step 6: Prove compatibility and close readiness

Goal: demonstrate that the route-aware adapter is behavior-preserving and that
remaining prepared-field demotion is still deferred to later ideas.

Actions:

- Run a matched before/after regression guard over riscv edge-publication and
  prepared lookup helper tests, or use supervisor-approved matching logs.
- Recheck that no prepared lookup or `PreparedBirModule` field group deletion,
  privatization, draft 155 readiness claim, or broad BIR policy migration was
  made.
- Summarize remaining follow-up work for ideas 246 and 247 in `todo.md` if
  needed; do not mutate those source ideas.

Completion check:

- Matching regression guard passes for the selected close scope.
- The source idea acceptance criteria are satisfied without weakening tests or
  changing compatibility status strings.
