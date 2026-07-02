Status: Active
Source Idea Path: ideas/open/523_bir_route2_select_chain_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Regression and review checkpoint

# Current Packet

## Just Finished

Completed Step 4 of `plan.md`: recorded validation and baseline-review
disposition for the route2 select-chain body extraction.

Accepted Step 3 proof:

- `git diff --check` passed.
- `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1` passed.
- Build passed and the backend subset passed with `345/345` tests.

Accepted supervisor regression guard:

- Ran with `--allow-non-decreasing-passed`.
- Before: `345/345` passed.
- After: `345/345` passed.
- No new failures and no new tests over 30 seconds.

The accepted `test_after.log` proof was rolled forward to `test_before.log`.
The hook-produced full-suite `test_baseline.new.log` candidate was rejected and
deleted because it introduced failing `string_authority_guard` results versus
`test_baseline.log`. Baseline review pending is cleared.

## Suggested Next

Execute Step 5: perform close-readiness bookkeeping and hand off to the plan
owner for lifecycle close review if the supervisor accepts the route2 slice.

## Watchouts

- `src/backend/bir/bir_route2.cpp` is intentionally body-only; declarations
  remain in `bir.hpp`.
- `route2_find_direct_global_dependency` remains private to the route2 TU.
- Route6 still consumes route2 through public `route2_select_chain_value_record`.
- The rejected `test_baseline.new.log` must not be treated as accepted baseline
  state; `test_baseline.log` remains the baseline reference.

## Proof

No new build or ctest was required for this bookkeeping packet.

Recorded accepted prior proof:

```sh
git diff --check
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

Result: build passed, backend subset passed with `345/345` tests.

Recorded supervisor regression guard: before `345/345`, after `345/345`, no new
failures, no new tests over 30 seconds, and `test_after.log` rolled forward to
`test_before.log`.

This packet's local validation: `git diff --check -- todo.md`.
