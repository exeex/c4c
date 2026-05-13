Status: Active
Source Idea Path: ideas/open/208_aarch64_branch_compare_target_mir_records.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Document Guardrails And Run Backend Proof

# Current Packet

## Just Finished

Completed Step 5 from `plan.md`: documented branch/compare record-only
guardrails and added contract proof for the deferred lowering boundary.

Concrete work completed:
- Extended `src/backend/mir/aarch64/codegen/records.md` with branch/compare
  guardrails for structured ids, predicates, operand facts, materialized bool
  conditions, fused compare candidates, non-fusable compare facts, and deferred
  opcode/assembly/encoding/object/linker boundaries.
- Added `backend_aarch64_branch_compare_contract` to prove branch/compare
  records stay `RecordOnly`, preserve structured `BlockLabelId`,
  `PreparedValueId`, `ValueNameId`, BIR predicate/type/source operand facts,
  and keep diagnostic names generic rather than concrete AArch64 mnemonics.
- Confirmed no lowering, assembly text, concrete opcode selection, encoding,
  object emission, linker behavior, or disallowed implementation files were
  touched.

## Suggested Next

Hand off to the supervisor/plan owner for lifecycle review and close decision
for `ideas/open/208_aarch64_branch_compare_target_mir_records.md`.

## Watchouts

- This completes the runbook steps, but plan exhaustion is separate from source
  idea completion; the supervisor/plan owner owns the close decision.
- The branch/compare surface is still record-only. Later opcode selection,
  assembly emission, encoding, object emission, and linker work should consume
  these records in a separate idea rather than expanding this plan.

## Proof

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log
```

Result: passed. Backend subset reported 123 tests passed, 0 failed; new
`backend_aarch64_branch_compare_contract` is included and green.
Proof log: `test_after.log`.
