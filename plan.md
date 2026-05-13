# AArch64 Branch Compare Target MIR Records Runbook

Status: Active
Source Idea: ideas/open/208_aarch64_branch_compare_target_mir_records.md

## Purpose

Transcribe the branch/compare target-MIR record idea into an execution route
that proves AArch64 can carry structured branch and compare facts from BIR and
prepared control-flow records without selecting concrete instructions.

Goal: add record-only AArch64 branch and compare target-MIR surfaces sourced
from prepared facts, with focused proof that structured ids remain
authoritative.

Core Rule: keep this slice record-only. Do not emit assembly, encode
instructions, or recover authority from rendered BIR, labels, or names.

## Read First

- `ideas/open/208_aarch64_branch_compare_target_mir_records.md`
- `ideas/closed/207_aarch64_target_register_and_instruction_record_core.md`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `src/backend/mir/aarch64/codegen/comparison.md`
- `ref/claudes-c-compiler/src/backend/arm/codegen/comparison.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/encoder/compare_branch.rs`

## Current Targets

- Extend the AArch64 `codegen/` target-record owner selected by idea 207.
- Consume prepared control-flow facts such as `PreparedBranchCondition`,
  `PreparedBranchTargetLabels`, block ids, prepared value ids, and BIR
  predicate/type facts.
- Preserve structured ids for functions, blocks, branch targets, compare
  operands, and prepared values.

## Non-Goals

- Do not emit `cmp`, `cset`, `b.cond`, `cbz`, `cbnz`, `tbz`, `tbnz`, or any
  assembly text.
- Do not add assembler encoding, object emission, linker behavior, call
  lowering, return lowering, memory lowering, or scalar ALU lowering beyond
  carrying compare operands.
- Do not add branch-shape shortcuts for named tests.
- Do not recover labels, predicates, values, or types from printed BIR or old
  markdown examples.
- Do not place new target records in `module/`; keep `module/` as prepared/BIR
  snapshot ownership.

## Working Model

- Target branch/compare records describe facts and candidates, not final
  machine instructions.
- Display labels may exist for diagnostics, but `BlockLabelId`, `ValueNameId`,
  prepared value identity, predicate facts, and typed operand records are the
  authoritative payload.
- Materialized boolean branches and fused compare-and-branch candidates must be
  distinguishable without selecting concrete opcodes.

## Execution Rules

- Prefer narrow, compile-backed packets.
- Add or extend focused backend tests for each new record family.
- Keep unsupported cases explicit; missing prepared facts should fail closed or
  be documented as deferred instead of patched around with rendered text.
- For code-changing steps, run:
  `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`
- Escalate to a broader supervisor-side check if changes leave the AArch64
  target-record owner or touch shared prepared/BIR contracts.

## Steps

### Step 1: Confirm Record Core And Branch Inputs

Goal: verify the accepted record-core owner and the available prepared
control-flow/BIR facts before adding new record families.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- prepared control-flow definitions used by `PreparedBirModule`

Actions:
- Inspect the record-only branch/compare gap in the current AArch64 target
  records.
- Identify exact prepared structures and BIR fields for unconditional branch
  targets, conditional branch targets, branch conditions, predicates, and value
  operands.
- Record any missing upstream fact as a deferred blocker in `todo.md` rather
  than inventing target-local recovery.

Completion check:
- `todo.md` names the concrete owner files and prepared inputs for the next
  implementation step, with no source idea or implementation edits required.

### Step 2: Add Branch Target And Predicate Vocabulary

Goal: define the typed record vocabulary needed for branch targets and compare
predicates.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- focused backend tests

Actions:
- Add record types for unconditional branch targets and conditional branch
  targets.
- Add compare predicate and compare operand-pair records that preserve BIR
  predicate/type facts and prepared value ids.
- Keep display strings diagnostic-only.
- Add narrow tests for constructing representative records directly or through
  existing helper boundaries.

Completion check:
- Backend proof passes and tests show branch target and compare predicate
  records preserve structured ids without rendered-name recovery.

### Step 3: Consume Prepared Branch Target Facts

Goal: build branch target records from prepared control-flow facts and BIR
terminators.

Primary targets:
- AArch64 `codegen/` record construction helpers
- focused backend tests covering unconditional and conditional branches

Actions:
- Convert `PreparedBranchTargetLabels` and block-id facts into target-local
  branch target records.
- Convert `PreparedBranchCondition` and associated BIR facts into conditional
  branch records.
- Preserve source function/block identity and target `BlockLabelId` values.
- Fail closed when required prepared target facts are absent.

Completion check:
- Representative unconditional and conditional branch cases produce structured
  target records with correct ids and no assembly emission.

### Step 4: Model Compare-Backed Branch Candidates

Goal: represent materialized boolean branches and fused compare-and-branch
  candidates without choosing concrete AArch64 opcodes.

Primary targets:
- AArch64 `codegen/` target branch/compare records
- focused backend tests covering compare-backed branch shapes

Actions:
- Add record fields or helper records that distinguish materialized boolean
  branch conditions from fused compare-and-branch candidates.
- Preserve compare operands, result/value ids, predicate facts, and type facts.
- Keep concrete instruction choice, encoding, and branch mnemonics deferred.

Completion check:
- Tests prove the two branch-condition families are represented distinctly
  while preserving structured operand and predicate authority.

### Step 5: Document Guardrails And Run Backend Proof

Goal: make the record-only contract discoverable and prove the backend subset
remains green.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.md` if local documentation needs an
  update
- focused backend contract tests
- `test_after.log`

Actions:
- Document any new branch/compare record-only surfaces and deferred lowering
  boundaries.
- Add or extend contract tests that reject assembly/encoding ownership creep
  where practical.
- Run the backend proof command and write `test_after.log`.

Completion check:
- Backend subset passes.
- The source idea acceptance criteria are satisfied with no assembler,
  object, memory, call, return, or scalar ALU selection changes.
