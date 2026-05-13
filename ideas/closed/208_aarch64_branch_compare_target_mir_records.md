# AArch64 Branch Compare Target MIR Records

Status: Closed
Created: 2026-05-13
Closed: 2026-05-13

Depends On:
- `ideas/closed/207_aarch64_target_register_and_instruction_record_core.md`

## Goal

Add the first structured AArch64 target-MIR records for branch and compare
families, sourced from BIR and prepared control-flow facts, without emitting
assembly or selecting final machine instructions.

This slice should prove the target record core can represent control-flow
facts from `PreparedBirModule` and keep ownership aligned with the layout
ledger before scalar instruction selection begins.

## Why This Idea Exists

The layout ledger marks branch/compare facts as present in BIR and prepared
control flow, but AArch64 has no target-local branch/compare records. Branch
and compare are a good first family after the register/instruction core
because they exercise block labels, predicates, values, and target labels while
remaining smaller than calls or memory.

## Reference Inputs

- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/mir/aarch64/codegen/comparison.md`
- `ref/claudes-c-compiler/src/backend/arm/codegen/comparison.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/encoder/compare_branch.rs`

## In Scope

- Define AArch64 target-MIR records for:
  - unconditional branch targets
  - conditional branch targets
  - compare predicates and compare operand pairs
  - materialized boolean branch conditions
  - fused compare-and-branch candidates
- Source branch facts from BIR terminators and prepared control-flow records:
  `PreparedBranchCondition`, `PreparedBranchTargetLabels`, block ids, and
  prepared value ids.
- Preserve `BlockLabelId`, `ValueNameId`, prepared value identity, BIR
  predicate/type facts, and display labels only as display.
- Place branch/compare record ownership in the file chosen by idea 207's
  target instruction/operand core and 205's layout ledger.
- Add tests or compile proof that representative prepared branch/compare facts
  produce structured AArch64 records with correct ids and no rendered-name
  recovery.

## Out Of Scope

- Emitting `cmp`, `cset`, `b.cond`, `cbz`, `cbnz`, `tbz`, `tbnz`, or any
  assembly text.
- Lowering scalar ALU operations beyond carrying compare operands.
- Handling calls, returns, memory, object emission, assembler encoding, or
  linker behavior.
- Adding branch-shape shortcuts for named tests.
- Recovering labels or predicates from printed BIR or old markdown examples.

## Acceptance Criteria

- AArch64 target-MIR branch/compare records exist and consume prepared
  control-flow facts.
- Structured ids remain authoritative for functions, blocks, branch targets,
  values, and compare operands.
- The records distinguish materialized bool branches from fused compare-branch
  candidates without selecting concrete instructions.
- Focused tests or compile proof cover representative unconditional,
  conditional, and compare-backed branches.
- No assembly emission or instruction selection is introduced.

## Closure Notes

Completed by the active runbook covering branch target and compare predicate
vocabulary, prepared branch target conversion, materialized-boolean and
fused-compare branch candidate records, focused backend tests, and local
record-only documentation.

Close-time backend regression guard passed with 123 passed, 0 failed, using
the existing accepted `test_before.log` and a fresh matching `test_after.log`.

## Reviewer Reject Signals

- The slice emits AArch64 branch assembly or encodes instructions.
- Branch targets are recovered from rendered labels when `BlockLabelId` is
  available.
- A missing prepared branch fact is worked around through printed BIR,
  testcase shape, or old markdown.
- The implementation only handles one named branch fixture.
- The branch/compare records are hidden in `module/` without respecting the
  accepted layout owner.
