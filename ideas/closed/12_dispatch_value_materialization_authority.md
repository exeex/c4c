# Dispatch Value Materialization Authority

## Goal

Start dispatch cleanup by extracting value materialization authority from
AArch64 `dispatch*` into clearer owner surfaces.

## Why This Exists

`dispatch*` currently does more than block traversal. The safest first dispatch
cleanup is value materialization, because it is a concrete responsibility that
can be separated from instruction routing without redesigning all control-flow
publication and edge-copy behavior at once.

## In Scope

- Audit `dispatch_value_materialization.*` and direct materialization helpers
  used from dispatch.
- Define whether each helper belongs to prepared facts, shared MIR utilities,
  or AArch64 target emission.
- Move or rename helpers to match their true owner.
- Keep `dispatch.cpp` focused on traversal/routing.
- Add focused tests for materialized values feeding later AArch64 operations.

## Out Of Scope

- Edge-copy and publication cleanup; those should follow separately.
- Calls cleanup not already completed.
- Replacing the whole AArch64 codegen pipeline.

## Acceptance Criteria

- Value materialization has a named owner separate from generic dispatch.
- `dispatch.cpp` no longer hides materialization decisions.
- Existing AArch64 materialization regressions stay fixed.
- Focused backend tests and build proof pass.

## Reviewer Reject Signals

- A patch only renames dispatch files without changing responsibility.
- A patch moves materialization into another vague bucket.
- A patch reintroduces text-emission as an internal contract.
- A patch expands testcase-shaped matching instead of using semantic facts.

## Closure Summary

Closed after Step 6 validation and final route review.

- Value materialization authority is separated from generic AArch64 dispatch
  traversal into named owner surfaces.
- `dispatch.cpp` remains focused on traversal/routing and delegates remaining
  materialization work to named owners.
- Final review artifact:
  `review/dispatch_value_materialization_final_route_review.md`.
- Final review found no route drift, testcase overfit, expectation weakening,
  unsupported downgrade, hidden broad cleanup, or insufficient proof.
- Canonical proof artifact: `test_after.log`.
- Final proof included `cmake --build --preset default`, the focused 14-test
  materialization subset at 14/14 passing, and broader backend validation via
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` at 162/162
  passing.
- Close-time regression guard was run with
  `.codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py`
  against `test_before.log` and `test_after.log`; it reported PASS with no new
  failures.
