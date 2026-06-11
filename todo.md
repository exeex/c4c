Status: Active
Source Idea Path: ideas/open/199_full_suite_baseline_string_authority_timeout_attribution.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Decide the Repair or Follow-Up Route

# Current Packet

## Just Finished

Completed Step 4: Decide the Repair or Follow-Up Route.

First-bad rule:
- The first real PASS-to-FAIL transition remains
  `1496a715d -> 9e4892bcd`.
- The current guard diagnostic for that first-bad route is the exact
  declaration-level hit
  `src/backend/mir/aarch64/codegen/calls.cpp:8062` /
  `find_prepared_indirect_callee_stored_value_source_fallback` with pattern
  `lookup-helper-rendered-compatibility`.

Repair decision:
- Added exactly one path+symbol classification for
  `find_prepared_indirect_callee_stored_value_source_fallback`.
- Classified it as a `backend-aarch64` compatibility bridge for the Route 3
  indirect callee prepared fallback because structured Route 3 identity is
  preferred and the helper is the explicit prepared fallback for absent,
  mismatched, ambiguous, or policy-sensitive cases.
- Did not add broad exceptions and did not weaken
  `string_authority_guard`.

Preserved Step 2 classification:
- `c_testsuite_aarch64_backend_src_00040_c` remains separate timeout/noise from
  Step 2 and did not drive this Step 4 classification.

## Suggested Next

Supervisor should review the completed Step 4 classification slice and decide
whether to commit it or request broader validation.

## Watchouts

- The first bad commit is `9e4892bcd`; do not infer first-bad from later
  failing commits such as `521cf148f` or the rejected candidate `1d1c506f0`.
- Do not infer first-bad from `1d1c506f0`; Step 1 found
  `string_authority_guard` already failing in
  `log/baseline_521cf148fa7660766f5bfa8c0932833bfa987311.log`.
- The Step 2 isolated proof reproduced only `string_authority_guard`; `00040`
  passed and remains timeout/noise unless it reappears in broader validation.
- Do not weaken, disable, or bypass `string_authority_guard`.
- Do not broaden this classification into adjacent AArch64 prepared-call
  helpers without a separate first-bad diagnostic and route justification.

## Proof

Ran the supervisor-delegated Step 4 proof and captured all output in
`test_after.log`:

```sh
(
  set -o pipefail
  python3 scripts/string_authority_guard.py && \
  python3 scripts/test_string_authority_guard.py && \
  ctest --test-dir build --output-on-failure -R '^string_authority_guard(_self_test)?$'
) > test_after.log 2>&1
```

Result: passed. `test_after.log` records
`string authority guard passed: 231 classified declaration-level hits`, the
Python self-test passing 3 tests, and both CTest cases
`string_authority_guard` and `string_authority_guard_self_test` passing.
