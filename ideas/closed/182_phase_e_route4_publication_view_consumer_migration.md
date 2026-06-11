# 182 Phase E Route 4 publication view consumer migration

## Goal

Switch one AArch64 current/block-entry publication reader to the Route 4 typed
publication view or validated facade style.

## Why This Exists

Phase D identified Route 4 publication availability as the lowest-risk
consumer migration rung after the selected facade contraction. This idea turns
that one rung into an implementation slice without claiming broad prepared API
deletion.

Source: `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`.

## In Scope

- One AArch64 value-publication or dispatch-publication reader that asks for
  current/block-entry publication identity.
- Route 4 publication availability records, typed references, or the existing
  fail-closed facade pattern.
- Prepared current-block and block-entry publication helpers as comparison and
  fallback answers.
- Route/prepared equivalence tests for present, absent, and mismatched
  reference cases.

## Out Of Scope

- Deleting prepared publication helpers.
- Moving value homes, storage availability, move planning, publication record
  construction, or block-order emission into BIR.
- Migrating prepared printer, debug, x86 wrapper, or oracle-test consumers.
- Treating the Route 4 facade as a generic lowering-plan aggregate.

## Acceptance Criteria

- The selected consumer reads Route 4-backed publication identity through a
  narrow typed view or validated facade.
- Prepared helpers remain public as oracle/fallback surfaces.
- Tests prove route/prepared equivalence for the selected boundary and retain
  fail-closed validation.
- The implementation does not weaken existing oracle or publication tests.

## Reviewer Reject Signals

- A generic facade or BIR-owned aggregate replaces typed Route 4 validation.
- The slice claims prepared publication API contraction from one migrated
  consumer.
- The change moves target storage, move, publication-record, or block-order
  policy into BIR.
- Tests are downgraded, renamed, or narrowed to hide a mismatch.
- The selected consumer is matched by testcase shape instead of semantic Route
  4 records.

## Closure Note

Closed after migrating the selected AArch64 dispatch-publication reader
`current_block_entry_publication_register(...)` through a narrow Route 4
publication-identity boundary while preserving prepared helper fallback and
oracle behavior.

Changed implementation and test files:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Focused proof:

```bash
cmake --build build --target backend_aarch64_instruction_dispatch_test backend_prealloc_block_entry_publications_test backend_prepared_lookup_helper_test &&
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prealloc_block_entry_publications|backend_prepared_lookup_helper)$' --output-on-failure
```

Result: 3/3 tests passed in `test_after.log`. Close-time regression guard
passed against `test_before.log` with `--allow-non-decreasing-passed`.

The hook-produced full-suite candidate `test_baseline.new.log` was rejected
and did not replace `test_baseline.log` because it regressed three c_testsuite
AArch64 backend tests:

- `c_testsuite_aarch64_backend_src_00119_c`
- `c_testsuite_aarch64_backend_src_00123_c`
- `c_testsuite_aarch64_backend_src_00195_c`

No prepared publication helper was deleted, privatized, or narrowed. This
closure does not claim broader prepared API deletion or broad consumer
migration.
