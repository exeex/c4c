# Idea 215 Route 7 Comparison Provenance Acceptance Review

Active source idea: `ideas/open/215_route7_comparison_provenance_consumer.md`

Review base: `153dee802` (`[plan] Activate Route 7 comparison provenance plan`)

Reason for base: git lifecycle history shows `153dee802` activated the current
source idea and current `plan.md`; later commits are the Step 1 selection,
implementation, fallback proof, and string-stability proof for the same active
idea.

Commits reviewed: 4 (`153dee802..HEAD`)

Files changed in reviewed range:

- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`
- `todo.md`

## Findings

No blocking findings.

## Contract Review

Source-idea alignment: matches source idea.

Runbook transcription: plan matches idea.

Route alignment: on track.

Technical debt: acceptable.

Validation sufficiency: narrow proof sufficient for this slice.

Reviewer recommendation: continue current route; acceptance-ready for
supervisor handoff.

## Evidence

- Exactly one consumer was moved: `lower_materialized_compare_condition_branch`
  now computes generic BIR identity, valid Route 7 materialized-condition
  identity, and prepared materialized-condition identity at
  `src/backend/mir/aarch64/codegen/comparison.cpp:2216`.
- Route 7 is accepted only when it agrees with both generic BIR identity and
  prepared materialized-condition authority:
  `materialized_condition_identity_matches_bir` compares availability, binary,
  instruction index, condition value name, and operand producers at
  `src/backend/mir/aarch64/codegen/comparison.cpp:484`; and
  `materialized_condition_identity_agrees_with_prepared` requires matching
  prepared binary, opcode, operand type, operands, instruction index, and
  prepared condition value name at
  `src/backend/mir/aarch64/codegen/comparison.cpp:495`.
- Invalid or stale Route 7 materialized-condition records fail closed before
  they can override branch behavior:
  `find_valid_route7_materialized_condition_producer_identity` returns
  `std::nullopt` for missing records, stale references, mismatched binary
  identity, and mismatched operand-reference validation at
  `src/backend/mir/aarch64/codegen/comparison.cpp:381`.
- Fallback remains prepared/generic BIR-owned: the selected producer is
  initialized from generic BIR identity and only replaced after Route 7,
  prepared, and BIR agreement all pass at
  `src/backend/mir/aarch64/codegen/comparison.cpp:2222`.
- Branch policy, suffix selection, target selection, fused legality, final
  assembler behavior, Route 8, and generic route facades were not migrated by
  this diff. The suffix and target emission still use existing branch facts and
  existing helper paths after producer selection at
  `src/backend/mir/aarch64/codegen/comparison.cpp:2243`.
- Positive proof now checks Route 7, BIR identity, and prepared producer
  agreement before dispatching the selected materialized compare branch at
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1460`.
- The new stale-prepared-lookup test covers valid Route 7/BIR evidence with
  prepared mismatch and proves fallback emits a compare branch rather than a
  materialized-condition `cbnz` path at
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1688`.
- Existing branch-control tests cover duplicate/conflict, absent Route 7 facts,
  condition-name mismatch, invalid Route 7 reference, and operand provenance
  mismatch at
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1575`,
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1607`,
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1631`,
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1662`,
  and `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1764`.
- Expected-string authority was not weakened: the reviewed diff does not touch
  `tests/backend/mir/backend_aarch64_machine_printer_test.cpp` or
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, and the
  branch-control exact emitted string checks around the positive path remain in
  place at
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1536`.

## Validation

Reviewed `test_after.log` shows the delegated proof passed:

```sh
cmake --build build --target backend_aarch64_branch_control_lowering_test backend_prepared_lookup_helper_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_aarch64_branch_control_lowering|backend_prepared_lookup_helper|backend_aarch64_machine_printer)$' --output-on-failure | tee test_after.log
```

Result: 3/3 tests passed:

- `backend_aarch64_machine_printer`
- `backend_aarch64_branch_control_lowering`
- `backend_prepared_lookup_helper`

`git diff --check 153dee802..HEAD` reported no whitespace errors.

## Residual Risk

This is a targeted acceptance review, not a full backend regression pass. The
risk is acceptable because the implementation is localized to one consumer plus
its immediate Route 7/prepared/BIR agreement tests, and no expected-string
baselines or broader branch-policy surfaces changed.

Lifecycle note: current `todo.md` still records Step 4 and suggests Step 5 next;
the supervisor should use this transient review report when performing final
handoff or lifecycle closure.
