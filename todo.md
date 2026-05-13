Status: Active
Source Idea Path: ideas/open/208_aarch64_branch_compare_target_mir_records.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Document Guardrails And Run Backend Proof

# Current Packet

## Just Finished

Completed Step 4 from `plan.md`: modeled compare-backed branch candidates as
record-only metadata without selecting concrete AArch64 opcodes.

Concrete work completed:
- Added `BranchCompareCandidateKind` and `BranchCompareCandidateRecord` so
  materialized-bool conditions, fused compare-and-branch candidates, and
  non-fusable compare facts are distinguishable in the target record surface.
- Extended prepared conditional branch conversion to populate candidate records
  with condition `PreparedValueId` / `ValueNameId`, predicate/type facts,
  compare operands, target pairs, and the `can_fuse_with_branch` decision.
- Added focused `backend_aarch64_compare_branch_candidate_records` coverage for
  materialized-bool, fused-compare, non-fusable-compare, immediate operand, and
  unsupported-predicate cases.
- Extended `backend_aarch64_prepared_branch_records` assertions to prove the
  prepared conversion keeps the candidate metadata attached to condition
  records.

## Suggested Next

Execute Step 5 from `plan.md`: document the branch/compare record-only
guardrails and add or extend contract proof that these surfaces do not own
assembly text, concrete opcode selection, encoding, object emission, or other
lowering behavior.

## Watchouts

- Step 4 remains record-only. No `cmp`, `cset`, `b.cond`, `cbz`, `cbnz`,
  `tbz`, `tbnz`, assembler text, object emission, or opcode selection was
  introduced.
- Named BIR compare operands now require matching prepared value homes to
  preserve `PreparedValueId` / `ValueNameId`; immediate compare operands remain
  valid without a prepared value id.
- `BranchCompareCandidateKind::NonFusableCompare` intentionally preserves valid
  compare facts when prepared analysis says they should not be fused with the
  branch.
- Unsupported/non-compare predicates still fail closed with
  `PreparedBranchRecordError::UnsupportedComparePredicate`.

## Proof

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log
```

Result: passed. Backend subset reported 122 tests passed, 0 failed; new
`backend_aarch64_compare_branch_candidate_records` is included and green.
Proof log: `test_after.log`.
