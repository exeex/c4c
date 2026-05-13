Status: Active
Source Idea Path: ideas/open/208_aarch64_branch_compare_target_mir_records.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Model Compare-Backed Branch Candidates

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: added record-only helpers that convert
prepared control-flow branch facts and BIR terminators into AArch64 branch
records.

Concrete work completed:
- Added `PreparedBranchRecordError` and `PreparedBranchInstructionRecordResult`
  so missing target, condition, compare, predicate, or value-home facts fail
  closed with explicit result state.
- Added `make_prepared_unconditional_branch_record(...)` for prepared
  unconditional branch blocks, preserving `FunctionNameId` and target
  `BlockLabelId` authority while validating the BIR terminator label id.
- Added `make_prepared_conditional_branch_record(...)` for prepared conditional
  branches, preserving target pairs, condition `PreparedValueId` /
  `ValueNameId`, BIR condition type, BIR predicate/type facts, and compare
  operand `bir::Value` payloads.
- Added focused `backend_aarch64_prepared_branch_records` coverage for
  unconditional branches, materialized-bool conditional branches, fused-compare
  conditional branches, and fail-closed missing/mismatched facts.

## Suggested Next

Execute Step 4 from `plan.md`: review the new prepared conversion helper
surface against compare-backed branch candidate needs and add any remaining
record-only candidate metadata/tests needed before documentation and final
contract proof.

## Watchouts

- Step 3 remains record-only. No `cmp`, `cset`, `b.cond`, `cbz`, `cbnz`,
  `tbz`, `tbnz`, assembler text, object emission, or opcode selection was
  introduced.
- Named BIR compare operands now require matching prepared value homes to
  preserve `PreparedValueId` / `ValueNameId`; immediate compare operands remain
  valid without a prepared value id.
- The helper validates structured BIR terminator label ids against prepared
  control-flow labels and does not recover branch targets from rendered label
  strings.

## Proof

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log
```

Result: passed. Backend subset reported 121 tests passed, 0 failed; new
`backend_aarch64_prepared_branch_records` is included and green. Proof log:
`test_after.log`.
