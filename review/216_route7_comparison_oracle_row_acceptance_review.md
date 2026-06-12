# Idea 216 Route 7 Comparison Oracle Row Acceptance Review

Active source idea: `ideas/open/216_route7_comparison_oracle_row.md`

Review base: `64f255b12` (`[plan] Activate Route 7 comparison oracle row plan`)

Reason for base: git lifecycle history shows `64f255b12` activated the current
source idea and current `plan.md`. The four later commits are the Step 1 row
selection, Step 2 helper-oracle augmentation, Step 3 absent-route proof, and
Step 4 stability proof for the same active idea.

Commits reviewed: 4 (`64f255b12..HEAD`)

Files changed in reviewed range:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
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

- Exactly one helper-oracle row was augmented: the materialized-condition
  success row in `verify_prepared_bir_comparison_condition_producer_equivalence`
  for `%cond` around
  `prepare::find_prepared_materialized_condition_producer(...)` at
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:10137`.
- The prepared oracle assertion remains authoritative. The same condition still
  requires `prepared_condition.has_value()`, BIR identity availability, prepared
  binary/instruction/value-name agreement, and populated lhs/rhs facts at
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:10161`.
- Route 7 evidence is route-native rather than a relabeled prepared result: the
  selected row now queries
  `route7_find_materialized_condition(route7_index, block, %cond, ...)` and
  validates the row through
  `route_index_validate_materialized_condition_reference(route_index_reference_facade(route7_index), ...)`
  at `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:10149`.
- The Route 7 row is checked against prepared identity for binary,
  instruction index, and condition value name, and against BIR identity for
  lhs/rhs producer provenance at
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:10175`.
- The added absent-route helper assertion is focused on the selected
  materialized-condition row: an empty Route 7 index/facade must report
  `MissingBlock`/`MissingRecord`, while prepared and BIR materialized-condition
  facts remain available at
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:10212`.
- Existing Route 7 helper coverage still proves stale owner, wrong
  relationship, duplicate reference, missing condition divergence,
  non-comparison, missing producer, and duplicate producer behavior at
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:9525` and
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:9588`.
- Existing branch-control coverage remains available for selected-row
  duplicate/conflict, absent Route 7 provenance, condition-name mismatch,
  invalid/stale Route 7 reference, stale prepared lookup/BIR fallback, and
  lhs/rhs provenance mismatch at
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1575`,
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1607`,
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1631`,
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1662`,
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1688`,
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1764`,
  and
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1782`.
- Unfused/materialized-bool fallback remains covered by the existing helper
  row where a `PreparedBranchConditionKind::MaterializedBool` condition makes
  fused-compare helper lookup unavailable at
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:9978`.
- The reviewed diff does not touch `src/backend/bir/bir.cpp`,
  `src/backend/bir/bir.hpp`, branch-control tests, machine-printer tests,
  wrappers, expected strings, final assembler rows, Route 8 code, or generic
  route-index facade code. It also does not rename helpers, downgrade
  supported paths, refresh baselines, or weaken expected-string contracts.

## Validation

Reviewed `test_after.log` shows the delegated targeted proof passed:

```sh
cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_machine_printer)$' --output-on-failure | tee test_after.log
```

Result: 3/3 tests passed:

- `backend_aarch64_machine_printer`
- `backend_aarch64_branch_control_lowering`
- `backend_prepared_lookup_helper`

`git diff --check 64f255b12..HEAD` reported no whitespace errors.

## Residual Risk

This is a targeted acceptance review, not a full backend regression pass. The
risk is acceptable because the material change is limited to assertions in one
BIR helper test, branch-control and machine-printer stability tests are green,
and no implementation, expected-string, wrapper, branch-policy, final
assembler, Route 8, or generic route-index surfaces changed.

Lifecycle note: current `todo.md` still records Step 4 and suggests Step 5
next. The supervisor can use this transient review report for final acceptance
handoff or lifecycle closure.
