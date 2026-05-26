# AArch64 Prior Preservation Baseline Drift

## Goal

Investigate and repair the full-suite baseline drift that first appears at
`11b33c8d0586` (`Retire prior-preservation selected fallback`).

## Why This Exists

`log/baseline_08a7f725a3f5820506a517f58ae9b9012bc20b7e.log` is clean with
`100% tests passed, 0 tests failed out of 3410`. The next relevant drift point
is `log/baseline_11b33c8d0586b44d163de6b74bff9c33957aab0b.log`, which reports
`37 tests failed out of 3410`, all in `aarch64_backend c_testsuite`.

The triggering commit only changes `calls_moves.cpp`, one dispatch test, and
`todo.md`. The failure family therefore points at the removal of a
prior-preservation selected fallback in AArch64 call-boundary move lowering.

## In Scope

- Compare the clean baseline `08a7f725a3f5` with the drifting baseline
  `11b33c8d0586`.
- Inspect `src/backend/mir/aarch64/codegen/calls_moves.cpp`
  prior-preservation call argument source selection.
- Reproduce or narrow at least one failing `c_testsuite_aarch64_backend_*`
  case.
- Restore semantic call argument preservation without reviving broad
  testcase-shaped fallback behavior.
- Keep tests focused on the missing prior-preservation source-selection rule.

## Out Of Scope

- Broad AArch64 call-plan architecture cleanup.
- Reverting unrelated later commits.
- Downgrading c_testsuite expectations.
- Treating the 37 failures as unrelated independent cases.

## Acceptance Criteria

- The first bad baseline family is explained with a concrete missing lowering
  rule.
- The AArch64 backend no longer regresses the representative failing tests.
- Focused proof covers the repaired prior-preservation/source-selection path.
- No unsupported markings, expectation weakening, or named-test shortcuts are
  used.

## Reviewer Reject Signals

- A patch simply restores the old broad fallback and reintroduces rederived
  prior homes for incomplete explicit selections.
- A patch special-cases the listed c_testsuite names.
- A patch weakens baseline or c_testsuite contracts.
- A patch claims the drift is fixed without proving at least one failing case
  and the focused call-boundary path.

## Deactivation Note

The active runbook completed the prior-preservation/source-selection family and
is deactivated. Commits `12d68c4d6`, `898884db5`, and `f3d8611c7` repaired the
representative plus the remaining same-family cases covered by the final
delegated subset: `backend_aarch64_instruction_dispatch`, `00173`, `00179`,
`00186`, and `00187` passed.

The final subset still reported `00181`, `00216`, and `00204` red, but those
were classified as separate families rather than unresolved
prior-preservation/source-selection drift. They are split into separate open
ideas:

- `ideas/open/13_aarch64_cts_00181_recursion_global_array_runtime.md`
- `ideas/open/14_c_aggregate_initializer_compound_literal_layout.md`
- `ideas/open/15_aarch64_aggregate_hfa_stdarg_abi.md`

This idea was not moved to `ideas/closed/` because close-grade regression guard
could not be accepted from the available canonical logs: `test_after.log` was
absent, and the delegated ownership explicitly excluded test log mutation.

## Closure Note

Closed after validation-only proof. Step 3 close-readiness validation recorded
matching focused canonical regression evidence for the repaired
prior-preservation/source-selection representative set:
`backend_aarch64_instruction_dispatch`,
`c_testsuite_aarch64_backend_src_00173_c`,
`c_testsuite_aarch64_backend_src_00179_c`,
`c_testsuite_aarch64_backend_src_00186_c`, and
`c_testsuite_aarch64_backend_src_00187_c`.

The close gate was rerun with matching canonical logs:

```sh
cmake --build --preset default > test_after.log 2>&1
ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00173_c|c_testsuite_aarch64_backend_src_00179_c|c_testsuite_aarch64_backend_src_00186_c|c_testsuite_aarch64_backend_src_00187_c)$' >> test_after.log 2>&1
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS, before 5/5 and after 5/5 with no new failures.

The supervisor-recorded broader backend closure validation also passed before
close handoff: `cmake --build --preset default` followed by
`ctest --test-dir build -j --output-on-failure -R '^backend_'` reported 162/162
backend tests passed. No implementation files, tests, expectations, or review
artifacts were changed by the validation-only close route.
