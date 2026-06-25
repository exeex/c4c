# BIR/Prepared Object Consumer Contract Completion

Status: Closed
Type: Architecture contract idea
Related:
- `ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md`

## Closure Summary

Closed after the Step 5 handoff back to idea 356.

The shared prepared object consumer contract now has focused coverage and
target consumers can use shared traversal/query/diagnostic helpers instead of
rediscovering target-independent BIR/prepared semantics locally. The completed
handoff records that idea 356 remains open for RV64 object-route work around
target-block preservation, MIR pseudo lowering, assembler-backed encoding,
labels, fixups, relocations, and executable progress.

Close proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(riscv_object_emission|prepared_object_consumer_contract|aarch64_function_traversal|prepare_phi_materialize|prepared_lookup_helper|prepared_printer)$' > test_after.log 2>&1
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: passed, 6/6 tests; regression guard passed with no new failures.

## Problem

The retracted RV64 `20000113-1.c` object-route slice showed that shared
BIR/prepared consumer gaps were being implemented ad hoc inside
`src/backend/mir/riscv/codegen/object_emission.cpp`.

That route is backwards. RV64 object emission should not become the place where
target-independent prepared/BIR semantics are rediscovered, rescheduled, or
patched case by case. Complete the shared BIR/prepared object-lowering consumer
contract before continuing target-specific object-route expansion.

The retracted RV64 slice is intentionally not accepted as progress: it exposed
shared contract gaps rather than a purely RV64-local missing instruction or
encoding issue.

## Goal

Define and test a shared prepared object-lowering consumer contract that target
backends can consume without reconstructing CFG or data-flow semantics.

## Scope

- Define and test a shared prepared object-lowering consumer contract for
  target backends.
- Treat BIR/prepared as the authority for block order, labels, terminators,
  edge/publication/copy obligations, select/join-transfer carriers, value
  homes, and frame ownership.
- Make target-specific backends provide hooks for concrete machine emission,
  not rediscovery or rescheduling of CFG/data-flow semantics.

## Required Contract Areas

### Block-Entry / Edge-Copy Execution Sites

- `PreparedMovePhase::BlockEntry` bundles must have an unambiguous insertion
  point: after the block label and before normal block body instructions,
  unless metadata explicitly says otherwise.
- Pre-terminator or predecessor-edge copies need equally explicit placement.
- Targets must not infer placement from bundle names plus block index.

### Select / Join-Transfer Carrier Classification

- Provide a shared query/helper that tells whether a select is a real select to
  emit or a prepared join-transfer carrier whose value is published by
  edge/block-entry copies.
- Targets must not scan `join_transfers` ad hoc to decide select behavior.

### Value-Home Transfer Planning

- Shared planning/query must describe how to read from and publish to value
  homes such as `Register`, `StackSlot`, `RematerializableImmediate`, and
  `PointerBasePlusOffset`.
- Targets should only emit concrete move/load/store encodings from an explicit
  home-transfer plan.

### Frame Ownership

- Prepared frame/stack layout must already account for stack-homed value homes
  required by publications and temporaries.
- Target object emission must not scan all value homes to discover additional
  frame size.

### Diagnostic Taxonomy

- Unsupported prepared object shapes must report specific missing contract
  pieces, such as:
  - missing block-entry publication,
  - unsupported move op,
  - unsupported stack width,
  - missing value home,
  - unsupported select carrier,
  - globals/data-section out of scope.
- Avoid the coarse `prepared module shape` diagnostic when a precise missing
  contract piece is known.

### Shared Traversal Shape

- Provide a target-independent prepared-function traversal schedule:

```text
label
  -> block-entry copies
  -> instructions
  -> pre-terminator / edge copies
  -> terminator
```

- RV64 and AArch64 should consume the same schedule and provide target hooks
  for machine emission.

## Non-Goals

- Do not optimize for the opt-in full RV64 gcc torture scan failure count in
  this idea.
- Do not implement globals/data sections, vararg/FPR ABI, or full RV64
  instruction coverage here.
- Do not continue patching RV64 `object_emission.cpp` as the primary solution.
- Do not make existing RV64 gcc torture failures acceptance blockers for this
  idea.

## Testing / Validation Requirements

- Preserve existing baseline tests.
- Add focused tests for the strengthened BIR/prepared contract itself:
  - block-entry copy placement,
  - select carrier classification,
  - value-home transfer planning,
  - frame layout ownership,
  - precise diagnostics where appropriate.
- RV64 gcc torture failures from the opt-in full scan should not block this
  idea, though they may be used as motivating examples.

## Acceptance Criteria

- A shared prepared object consumer contract exists and is covered by focused
  tests.
- Target object emission can consume an explicit traversal / transfer plan
  instead of rediscovering placement, select-carrier, value-home, or frame
  semantics.
- Diagnostics distinguish missing contract pieces instead of collapsing them
  into a broad prepared-module-shape failure.
- The retracted RV64 `20000113-1.c` slice remains unaccepted as a target-local
  fix unless later work proves the shared contract is complete.
- Existing baseline tests remain green for touched surfaces.

## Reviewer Reject Signals

- Reject if progress is claimed by continuing ad hoc RV64
  `object_emission.cpp` patching as the primary solution.
- Reject testcase-name shortcuts or fixes scoped only to `20000113-1.c`.
- Reject target code that infers block-entry, edge-copy, select-carrier,
  value-home, or frame semantics from local heuristics instead of a shared
  contract.
- Reject expectation downgrades or weaker unsupported diagnostics as proof of
  progress.
- Reject broad globals/data-section, vararg/FPR ABI, or full instruction-set
  expansion folded into this contract idea.
