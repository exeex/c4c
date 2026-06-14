# Plan: Phase F3 Prepared Control-Flow Call-Preservation Dominance Candidate

Status: Active
Source Idea: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md

## Purpose

Turn one remaining bounded candidate from idea 260 into an executable packet
without claiming broad `PreparedBirModule` field retirement.

## Goal

Implement and prove the `control_flow` call-preservation dominance candidate
only: prior preserved-value lookup may use route/BIR dominance over call-plan
block indices plus same-block instruction ordering only when null context,
invalid ids, empty prior lists, later same-block entries, non-dominating or
unreachable blocks, duplicate prior entries, and missing preserved rows return
current null or unavailable results.

## Core Rule

Do not delete, privatize, wrap, or broadly retire any `PreparedBirModule`
field. This plan is one `control_flow` reader/helper candidate, not a
structural exit for the aggregate.

## Read First

- `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/call_plans.hpp`
- `src/backend/prealloc/control_flow.hpp`
- Prior preserved-value lookup tests in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- BIR/route dominance helpers that can answer whether one block reaches or
  dominates another without trusting prepared call-plan block indices alone.

## Current Scope

- Selected candidate: `control_flow` call-preservation dominance packet.
- Primary authority to introduce: route/BIR dominance and same-block
  instruction ordering may support prior preserved-value source lookup.
- Retained compatibility surface: prepared call plans, prepared
  preserved-value rows, existing lookup tables, and current null or
  unavailable behavior remain authoritative when route/BIR dominance is
  absent, invalid, ambiguous, or non-agreeing.

## Non-Goals

- Do not implement another candidate from idea 260 in this runbook.
- Do not move printer/debug strings, route-debug strings, target output,
  fallback spelling, or target policy into BIR.
- Do not change helper/oracle status names, diagnostics, unsupported
  expectations, or baselines to claim progress.
- Do not change branch-target identity, block-index label bridge,
  value-name lookup, value-home lookup, semantic resolver API, module,
  store-source-publication, or backend lowering behavior.
- Do not claim broad structural exit, aggregate retirement, privatization,
  wrapping, or deletion progress.

## Working Model

Prepared call-plan rows remain the owner of public call-preservation behavior.
A BIR or route control-flow fact can participate only when it agrees that the
preserved source call reaches the queried call: same block requires a strictly
earlier instruction index, and cross-block candidates require a valid
dominating or reaching control-flow relationship from source block to query
block. Missing control-flow context, invalid value ids, empty prior lists,
later same-block rows, non-dominating rows, unreachable rows, duplicate
candidate positions, or incomplete preserved rows must fail closed instead of
being accepted because their prepared block indices happen to sort earlier.

## Execution Rules

- Keep edits local to the selected prior preserved-value lookup path and
  focused tests.
- Prefer a small agreement helper if both lookup construction and selection
  need the same dominance boundary.
- Preserve prepared call-plan lookup behavior as compatibility behavior; do
  not treat sorted prepared indices alone as structural agreement.
- Prove nearby same-feature rows, not only one positive testcase.
- Treat expectation rewrites, helper renames, classification-only movement, or
  broad control-flow rewrites as non-progress.

## Steps

### Step 1: Inventory Call-Preservation Dominance Contract

Goal: identify the exact current prepared lookup behavior and consumers for
the selected candidate.

Actions:

- Inspect prior preserved-value indexing and lookup code in
  `prepared_lookups.cpp` and `call_plans.cpp`.
- Inspect control-flow reachability or dominance helpers in
  `control_flow.hpp` and nearby route/BIR query surfaces.
- Record the current handling of null control-flow context, invalid value ids,
  empty prior lists, later same-block entries, non-dominating blocks,
  unreachable blocks, duplicate prior entries, and missing or incomplete
  preserved rows in `todo.md`.
- Identify the narrow test bucket and fixture helpers that already cover
  prior preserved-value lookup.

Completion check:

- `todo.md` names the selected lookup row, current behavior to preserve,
  candidate implementation files, and a focused proof command for the next
  packet.

### Step 2: Design the Dominance Agreement Boundary

Goal: define when route/BIR control-flow may be used without changing the
prepared call-preservation contract.

Actions:

- Choose whether the agreement boundary belongs in an existing lookup helper,
  a small shared prealloc helper, or local adapters in the call-preservation
  lookup path.
- Specify accepted rows: matching value id, complete preserved source,
  strictly earlier same-block instruction, or a valid route/BIR relationship
  proving the source block reaches or dominates the query block.
- Specify rejected rows: null control-flow context, invalid value id, empty
  prior list, later same-block entry, non-dominating source block,
  unreachable source block, duplicate same-position entries, missing preserved
  pointer, incomplete preservation metadata, and ambiguous agreement.
- Record the design in `todo.md`; do not edit the source idea unless the
  selected candidate itself is ambiguous.

Completion check:

- `todo.md` contains an implementation packet with explicit owned files,
  retained compatibility behavior, and focused fail-closed proof
  requirements.

### Step 3: Implement Narrow Dominance Bridge

Goal: allow the selected prior preserved-value lookup path to use route/BIR
dominance only under the Step 2 agreement boundary.

Actions:

- Implement the narrow dominance agreement helper or adapter from Step 2.
- Wire it only into the selected prior preserved-value lookup path used by
  call-preservation source selection.
- Keep prepared call plans, prepared preserved-value records, existing lookup
  vectors, public aggregate compatibility, and current null or unavailable
  fallback behavior intact.
- Avoid touching unrelated `control_flow` branch-target or block-label rows,
  `names`, `module`, `store_source_publications`, printer/debug, route-debug,
  or backend lowering candidates.

Completion check:

- Build proof passes.
- Focused prepared lookup helper tests pass.
- The diff does not contain output baseline rewrites, unsupported downgrades,
  or unrelated candidate movement.

### Step 4: Add Fail-Closed Proof Rows

Goal: prove the selected candidate rejects stale, ambiguous, or non-dominating
control-flow facts while preserving compatibility behavior.

Actions:

- Add or preserve positive rows showing valid same-block prior calls and valid
  cross-block reaching or dominating calls can select a prior preserved source.
- Add fail-closed rows for null context, invalid ids, empty prior lists, later
  same-block entries, non-dominating or unreachable blocks, duplicate prior
  entries, missing preserved rows, and incomplete preservation metadata.
- Preserve route-debug, target-output, printer/debug, branch-target,
  block-index bridge, names, module, store-source, and backend expectations
  unless the selected lookup path already owns that surface and the change is
  explicitly proved.

Completion check:

- Focused proof covers positive and nearby fail-closed rows.
- `todo.md` records any unsupported fixture surfaces that should remain out of
  scope rather than being forced into this runbook.

### Step 5: Broader Validation and Closure Readiness

Goal: decide whether the selected candidate is complete and ready for
plan-owner closure review.

Actions:

- Run the focused proof again after any final cleanup.
- Run a broader backend/prepared or call-boundary subset if the dominance
  helper is shared by more than one consumer family.
- Record proof commands, pass/fail result, and residual out-of-scope rows in
  `todo.md`.
- If the selected candidate is complete under idea 260 acceptance criteria,
  request plan-owner closure review; if more candidates remain, close or
  replace this runbook rather than expanding it in place.

Completion check:

- `todo.md` shows the selected candidate complete or blocked with exact
  evidence.
- No other idea 260 candidate has been absorbed into this active plan.
