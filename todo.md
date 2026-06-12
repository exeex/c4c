Status: Active
Source Idea Path: ideas/open/219_phase_e1_control_flow_identity_helper_contraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Required Fallback And Nearby Coverage

# Current Packet

## Just Finished

Completed `plan.md` Step 3 for
`find_prepared_control_flow_branch_target_labels(...)`.

Confirmed and completed the required fallback proof surface in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`. The selected
helper now has focused coverage for prepared-only nearby same-feature lookup,
absent prepared block fail-closed behavior, positive agreement with structured
BIR successor ids, raw output/string label drift stability, invalid BIR id
fallback, BIR/prepared mismatch fallback, non-conditional BIR policy fallback,
and invalid prepared source-label rejection.

Duplicate/conflict is not separately detectable by this selected helper's
current first-match prepared block lookup contract; the Step 3 conflict class
is accounted for through the direct BIR/prepared mismatch fallback assertion.
No helper names, baselines, expected strings, branch spelling, edge-copy
scheduling, move policy, wrapper behavior, printer/debug rows, or helper-oracle
output were weakened or rewritten.

## Suggested Next

Supervisor should decide whether to delegate `plan.md` Step 4 validation and
lifecycle handoff for this selected helper slice.

## Watchouts

- `clang-format` is not installed in this environment, so no formatter command
  was available after the Step 3 test patch.
- The selected helper remains agreement-gated: raw BIR labels and mismatched
  structured ids do not override prepared labels.
- Do not interpret this slice as compare-branch condition policy migration;
  compare predicate facts remain prepared-owned.

## Proof

Ran the delegated Step 3 proof command exactly:

`( cmake --build --preset default --target backend_prepare_structured_context_test backend_prepare_block_only_control_flow_test backend_prepared_lookup_helper_test backend_aarch64_prepared_branch_records_test && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_prepare_block_only_control_flow|backend_prepared_lookup_helper|backend_aarch64_prepared_branch_records)$' --output-on-failure ) > test_after.log 2>&1`

Result: passed, 4/4 tests green. Proof log: `test_after.log`.

Supervisor regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed, before 4/4 and after 4/4 with no new failures.
