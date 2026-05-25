Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The Preservation Boundary Leak

# Current Packet

## Just Finished

Step 5 closure review: rejected closure for the frame-slot call argument
boundary narrowing checkpoint and regenerated the active runbook around the
next surviving prior-preservation lookup boundary.

Close-time regression guard used the existing canonical backend logs and
passed: 162/162 tests before and after, no new failures.

The source idea remains open because AArch64 calls consolidation still has
durable remaining scope in surviving `calls*.cpp` files and broad `calls.hpp`
helpers. The next checkpoint starts at preservation lookup rather than closing
or moving the idea to `ideas/closed/`.

## Suggested Next

Execute Step 1 by mapping `calls_preservation.cpp` prior-preserved-value lookup
helpers and their `calls_moves.cpp` consumers. Select one concrete boundary
leak, identify the intended prepared/emission owner or missing prepared fact,
and record the focused proof scope before implementation.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not broaden this checkpoint into full dispatch cleanup or a whole
  calls-family rewrite.
- Do not claim helper renames, expectation rewrites, or testcase-shaped
  shortcuts as progress.
- Keep source-idea closure rejected until all durable acceptance criteria are
  met, not merely this regenerated runbook.

## Proof

Lifecycle-only closure review. No implementation files, tests, build metadata,
`test_before.log`, or `test_after.log` were changed.

Regression guard command run by the plan owner:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS. Before: 162 passed, 0 failed, 162 total. After: 162 passed,
0 failed, 162 total. New failures: 0.
