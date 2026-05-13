Status: Active
Source Idea Path: ideas/open/208_aarch64_branch_compare_target_mir_records.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume Prepared Branch Target Facts

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: added record-only AArch64 branch target and
compare predicate vocabulary under `src/backend/mir/aarch64/codegen/records.hpp`
and `.cpp`.

Concrete work completed:
- Added `BranchConditionForm` to distinguish unconditional,
  materialized-bool, and fused-compare branch forms without naming concrete
  AArch64 instructions.
- Added `BranchTargetPairRecord` for authoritative true/false
  `BlockLabelId` pairs while preserving `FunctionNameId`.
- Added `ComparePredicateRecord`, `CompareValueRecord`, and
  `CompareOperandPairRecord` so BIR predicate opcode, compare type,
  `PreparedValueId`, `ValueNameId`, and source `bir::Value` facts can survive
  as target-record data.
- Added `BranchConditionRecord` and extended `BranchInstructionRecord` with
  optional target-pair and typed condition records while preserving the older
  generic fields for existing tests.
- Added `branch_condition_form_name(...)` and `is_compare_predicate(...)`
  helpers.
- Added focused `backend_aarch64_branch_compare_records` coverage proving
  materialized-bool and fused-compare record shapes directly.

## Suggested Next

Execute Step 3 from `plan.md`: add conversion helpers that populate the new
record-only branch/compare vocabulary from prepared control-flow facts, using
authoritative prepared ids and BIR values rather than rendered-name recovery.

## Watchouts

- Step 2 intentionally added record vocabulary only. No `cmp`, `cset`,
  `b.cond`, `cbz`, `cbnz`, `tbz`, `tbnz`, assembler text, object emission, or
  opcode selection was introduced.
- Step 3 should consume prepared branch-condition facts without inventing
  string recovery. Arbitrary `bir::Value` operands may still need lookup
  through prepared value locations or storage plans.
- Existing `BranchInstructionRecord::target`, `condition`, and `conditional`
  remain for compatibility; new conversion should prefer `target_pair` and
  `condition_record` for conditional branch authority.

## Proof

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log
```

Result: passed. Backend subset reported 120 tests passed, 0 failed; new
`backend_aarch64_branch_compare_records` is included and green. Proof log:
`test_after.log`.
