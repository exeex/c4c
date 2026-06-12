Status: Active
Source Idea Path: ideas/open/219_phase_e1_control_flow_identity_helper_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Agreement-Gated Identity Read

# Current Packet

## Just Finished

Completed `plan.md` Step 2 for
`find_prepared_control_flow_branch_target_labels(...)`.

Added a narrow overload in `src/backend/prealloc/control_flow.hpp` that accepts
the source `bir::Block`, reads `bir::Terminator::true_label_id` and
`false_label_id` only for conditional terminators, and returns those structured
ids only when both are valid and exactly agree with the retained prepared
target-label pair. Absent prepared labels, invalid BIR ids, non-conditional BIR
terminators, and BIR/prepared mismatches all preserve the previous prepared
fallback behavior.

Updated the existing compare resolver and the AArch64 non-compare conditional
branch facts path to pass their already-available source BIR block through the
selected helper. No branch predicate, compare-fusion, short-circuit, join,
wrapper, printer/debug, public API deletion, or expected-string behavior was
changed.

Added focused helper coverage in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` for agreeing
structured successor ids plus raw-label drift, invalid-id fallback, mismatch
fallback, non-conditional BIR fallback, and invalid prepared source-label
rejection.

## Suggested Next

Supervisor should decide the next packet from the active plan. If continuing
this contraction route, keep the next slice limited to one named helper surface
and preserve prepared-authoritative fallback on mismatch.

## Watchouts

- `clang-format` is not installed in this environment, so no formatter command
  was available after the patch.
- The new overload is intentionally behavior-preserving on mismatch: raw BIR
  labels and mismatched structured ids do not override prepared labels.
- Do not interpret this slice as compare-branch condition policy migration;
  compare predicate facts remain prepared-owned.

## Proof

Ran the delegated proof command exactly:

`( cmake --build --preset default --target backend_prepare_structured_context_test backend_prepare_block_only_control_flow_test backend_prepared_lookup_helper_test backend_aarch64_prepared_branch_records_test && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_prepare_block_only_control_flow|backend_prepared_lookup_helper|backend_aarch64_prepared_branch_records)$' --output-on-failure ) > test_after.log 2>&1`

Result: passed, 4/4 tests green. Proof log: `test_after.log`.

Supervisor regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed, before 4/4 and after 4/4 with no new failures.

Broader supervisor validation:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 180/180 backend tests green.
