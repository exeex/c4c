# RV64 Object Route Move Bundle Target Shapes

Status: Closed
Type: Follow-up repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Repair prepared move-bundle target and copy-authority shapes that block RV64
object emission.

## Why This Exists

The 2026-06-26 reopened 354 classification found 92 move-bundle failures:

- 86 `unsupported_move_bundle_target_shape`
- 4 `prepared select publication move bundle requires unsupported RV64 moves`
- 2 `prepared_consumer_category=missing_move_bundle`

Representatives:

- `tests/c/external/gcc_torture/src/20080519-1.c`
- `tests/c/external/gcc_torture/src/20010604-1.c`
- `tests/c/external/gcc_torture/src/930123-1.c`

## In Scope

- Classify target shapes that prepared move bundles are trying to publish.
- Add RV64 move lowering for reusable register, stack, select-publication, and
  copy-authority forms already present in the prepared contract.
- Keep move-bundle ownership explicit; missing authority should be fixed at
  the correct producer when the prepared contract is incomplete.
- If the repair requires changing BIR/prepared move-bundle production,
  create or switch to a separate BIR/prepared producer idea instead of
  guessing that authority in MIR/RV64 object emission.

## Out Of Scope

- Treating every failed copy as a generic memory operation.
- Hiding missing move-bundle authority by silently dropping copies.
- Rewriting unrelated instruction or terminator lowering.

## Acceptance

- Representative move-bundle cases no longer fail with the same move-bundle
  diagnostics.
- The refreshed bucket count for move-bundle target/copy failures decreases.
- Runtime comparison remains clean for newly object-emitted representatives.

## Reviewer Reject Signals

- Reject lowering that drops a publication/copy to make object compilation
  succeed.
- Reject named-case matching on `20080519-1.c`, `20010604-1.c`, or
  `930123-1.c`.
- Reject changes that conflate missing prepared authority with target lowering
  and leave ownership ambiguous.
- Reject MIR/RV64 fixes that fabricate move-bundle or select-publication facts
  that should have been produced by BIR/prepared lowering.
- Reject expectation rewrites or allowlist filtering.

## Lifecycle Notes

- 2026-06-26: Closed after refreshed Step 1 proof found no current owned
  move-bundle target, select-publication move, or missing-authority family in
  the selected seed bucket. `20080519-1`, `20010604-1`, `930123-1`,
  `20000808-1`, `20020510-1`, `20040309-1`, `20000503-1`, and `20000801-2`
  all reported `c4cll_status=0` and `prepared_status=0`.
- 2026-06-26: Prepared dumps exercised register, stack, stack/register
  crossing, call argument/result, return, select-materialization, and
  edge-preservation move shapes. All selected representatives compiled
  successfully, no producer missing-authority gap surfaced, and no Step 2 move
  lowering family was justified.
- 2026-06-26: Close gate passed with the backend regression guard over
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`. The
  rolled-forward `test_before.log` and regenerated `test_after.log` both
  reported 326/326 passing backend tests with no new failures.
