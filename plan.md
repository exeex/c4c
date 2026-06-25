# BIR/Prepared Object Consumer Contract Completion

Status: Active
Source Idea: ideas/open/359_bir_prepared_object_consumer_contract_completion.md
Supersedes Active Runbook: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md remains open and paused, not closed.

## Purpose

Complete the shared BIR/prepared object-lowering consumer contract before
continuing target-specific RV64 object-route expansion.

## Goal

Give target backends an explicit prepared traversal and transfer contract so
they emit machine code through hooks instead of rediscovering CFG, publication,
select, value-home, or frame semantics.

## Core Rule

BIR/prepared owns block order, labels, terminators, edge/publication/copy
obligations, select/join-transfer carriers, value homes, and frame ownership.
Target-specific object emission provides concrete machine encodings from the
shared contract; it must not reschedule or infer target-independent semantics.

## Read First

- `ideas/open/359_bir_prepared_object_consumer_contract_completion.md`
- `ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md`
- Existing BIR/prepared publication, move, select, value-home, frame, and
  diagnostic tests before changing code.
- RV64/AArch64 prepared object consumer surfaces only after the shared contract
  audit identifies the intended boundary.

## Current Targets

- Shared prepared object-lowering consumer contract.
- Focused tests for:
  - block-entry copy placement,
  - select/join-transfer carrier classification,
  - value-home transfer planning,
  - frame layout ownership,
  - precise diagnostics.

## Non-Goals

- Do not optimize for the opt-in full RV64 gcc torture scan failure count.
- Do not implement globals/data sections, vararg/FPR ABI, or full RV64
  instruction coverage.
- Do not continue RV64 `object_emission.cpp` patching as the primary solution.
- Do not require existing RV64 gcc torture failures to pass before accepting
  this idea.

## Context Note

The uncommitted RV64 `20000113-1.c` slice was intentionally retracted. It
showed shared BIR/prepared contract gaps rather than a purely RV64-local
missing instruction-set fix, so it must not be accepted as target-local
progress.

## Working Model

```text
BIR/prepared semantic authority
  -> shared prepared object traversal and transfer plan
  -> target backend emission hooks
  -> target encoder / object writer
```

Target hooks may encode moves, loads, stores, labels, branches, and diagnostics
from the plan. They may not decide where copies belong, whether a select is a
join-transfer carrier, which frame slots must exist, or which value homes must
be discovered by scanning unrelated target state.

## Execution Rules

- Start with audit/design. Do not implement target-specific fixes first.
- Add focused contract tests before or with shared helpers.
- Keep target-specific changes minimal and hook-shaped.
- Preserve existing baseline tests for touched shared and backend surfaces.
- Treat RV64 torture cases as motivating evidence, not acceptance blockers.

## Step 1: Audit and Design the Shared Prepared Consumer Contract

Goal: identify existing metadata and define the missing shared contract before
editing implementation.

Concrete actions:

- Audit existing prepared move/publication/select/frame metadata and tests.
- Locate current representations for:
  - `PreparedMovePhase::BlockEntry`,
  - pre-terminator / predecessor-edge copies,
  - select/join-transfer carriers,
  - value homes,
  - frame/stack ownership,
  - prepared object diagnostics.
- Write a concise design note in `todo.md` naming the shared query/helper or
  traversal shape needed for each required contract area.
- Confirm which parts are already represented and which are contract gaps.
- Do not patch RV64 `object_emission.cpp` as the primary solution.

Completion check:

- `todo.md` contains a boundary/design table for the six contract areas.
- The next step can add focused shared-contract tests without relying on a
  target-specific `20000113-1.c` patch.

## Step 2: Add Focused Shared-Contract Tests

Goal: lock the desired BIR/prepared consumer behavior with narrow tests.

Concrete actions:

- Add or extend tests for block-entry copy placement.
- Add tests for real-select versus join-transfer carrier classification.
- Add tests for value-home transfer planning across register, stack,
  rematerializable immediate, and pointer-base-plus-offset homes as supported.
- Add tests that prepared frame ownership accounts for stack-homed publication
  and temporary needs without target object emission scanning all homes.
- Add diagnostics tests for specific missing contract pieces.

Completion check:

- Focused tests fail for missing contract behavior before implementation or are
  written alongside minimal shared helpers.
- Tests do not depend on full RV64 gcc torture progress.

## Step 3: Implement Shared Traversal and Query Helpers

Goal: expose a target-independent schedule and planning API for prepared object
consumers.

Concrete actions:

- Implement or complete traversal order:

```text
label
  -> block-entry copies
  -> instructions
  -> pre-terminator / edge copies
  -> terminator
```

- Add shared select-carrier classification.
- Add shared value-home transfer planning/query helpers.
- Ensure frame layout ownership is represented before target emission.
- Add precise diagnostic paths for missing contract pieces.

Completion check:

- Shared tests from Step 2 pass.
- The helpers are target-independent and do not encode RV64-only policy.

## Step 4: Connect Target Consumers Through Hooks

Goal: make target backends consume the shared contract without rediscovering
semantics.

Concrete actions:

- Update target object consumers to accept traversal / transfer plans through
  hooks.
- Keep target code responsible for concrete machine encodings only.
- Preserve existing RV64/AArch64 baseline behavior.
- Avoid broad instruction-set, ABI, or data-section expansion.

Completion check:

- Target-specific code no longer needs local heuristics for the covered
  contract areas.
- Existing baseline tests for touched surfaces pass.

## Step 5: Review and Handoff Back to 356

Goal: decide whether the shared contract is complete enough to resume RV64
object-route work.

Concrete actions:

- Run the relevant focused test set and any existing backend baseline tests
  touched by the change.
- Record which 356 blockers are now unblocked and which remain out of scope.
- Recommend whether to reactivate 356 or create a smaller follow-up idea.

Completion check:

- The contract work is independently useful and tested.
- 356 remains open for continuation, with a clear handoff.

## Validation Ladder

- Lifecycle-only packets: no build required.
- Audit/design packets: no build required unless code is touched.
- Code/test packets: build affected backend/shared test targets first.
- Acceptance proof: run the focused BIR/prepared contract tests plus existing
  baseline tests for touched backend surfaces.
- The opt-in full RV64 gcc torture backend scan is not an acceptance gate for
  this idea.

## Reviewer Reject Signals

- Reject ad hoc RV64 `object_emission.cpp` patching as the primary solution.
- Reject testcase-name shortcuts or `20000113-1.c`-only progress.
- Reject target code that infers block-entry, edge-copy, select-carrier,
  value-home, or frame semantics from local heuristics.
- Reject expectation downgrades, unsupported broadening, or diagnostic
  weakening as proof of progress.
- Reject unrelated globals/data-section, vararg/FPR ABI, or full instruction
  coverage folded into this idea.
