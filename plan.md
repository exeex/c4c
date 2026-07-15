# LIR Aggregate and Vector Value Identity Convergence Runbook

Status: Active
Source Idea: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Resumed from: 811 native vector authority carrier closure

## Purpose

Complete the remaining representative vector rows one bounded, row-local
authority route at a time without reopening accepted aggregate work.

## Core Rule

Use checked current-function IDs and native row-specific facts only. Display
text is compatibility rendering and cannot select, recover, or repair
authority. Keep every unselected row fail closed.

## Read First

- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- `ideas/closed/811_lir_native_vector_authority_carrier_publication.md`
- `src/codegen/lir/ir.hpp`, `src/codegen/lir/operands.hpp`, and
  `src/codegen/lir/verify.cpp`
- `src/codegen/lir/hir_to_lir/expr/binary.cpp` and `expr/misc.cpp`

## Non-Goals

- Do not repeat accepted Steps 1--8, including the extractvalue and
  insertvalue routes or 811 carrier publication.
- Do not widen into generic provenance, vector-layout publication, aggregate,
  CFG/PHI, pointer/object, Raw-BIR, target lowering, MIR, or emission.
- Do not select more than one vector row or use presentation text as facts.

## Accepted History

- Steps 1--4: selected `LirExtractValueOp` work is accepted; its full
  baseline closure proof is recorded in the source idea.
- Steps 5--7: selected terminal direct-complex `LirInsertValueOp` route is
  accepted in `270c6a93e` and `8fe6c3569`; source closure correctly remained
  rejected because vector rows remain.
- Step 8: no vector row was selectable from existing structured facts. The
  separate 811 prerequisite is now accepted in `76f92ad60` and `56203cbf9`.

## Steps

### Step 9 - Implement and prove the Step 8 selection

Goal: perform the required fresh one-row audit against 811's accepted carrier,
then implement and prove only the selected row.

Actions:

- before changing code, inspect only `LirInsertElementOp`,
  `LirExtractElementOp`, and `LirShuffleVectorOp` seams against the new carrier;
  record in `todo.md` the selected seam, exact carrier facts, positive and
  malformed matrix, and the two excluded rows;
- select exactly one row only when its complete result/use and row-specific
  vector/index/mask contract is natively available and fail closed;
- publish and verify only that row's authority; add nearby valid,
  missing/foreign, and selected type/index/mask malformed coverage;
- run a fresh build, selected same-feature proof, and matching regression
  guard. Escalate to a full baseline when the supervisor judges the shared
  surface requires it.

Completion check: one and only one vector row has a proven structural contract;
the two unselected rows and all non-vector scope remain unchanged.

### Step 10 - Reassess remaining source completion

Goal: make the next explicit lifecycle decision after the Step 9 packet.

Completion check: repair to one fresh row audit, atomically switch to a
separately scoped blocker, or produce source-closure evidence. Never silently
widen Step 9.
