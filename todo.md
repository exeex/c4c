Status: Active
Source Idea Path: ideas/open/556_prepared_move_bundle_ambiguous_stack_destination_src_960209_1.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Capture Bundle Facts

# Current Packet

## Just Finished

Lifecycle transition closed the exhausted local-memory plan for
`src/960209-1.c` and activated the prepared move-bundle classifier blocker as
a separate idea.

## Suggested Next

Execute Step 1 from `plan.md`: reproduce or inspect the row, capture the first
ambiguous prepared move-bundle facts, and decide whether the row matches the
existing fail-closed classifier contract or exposes missing producer authority.

## Watchouts

- Do not continue this work as RV64 local-memory addressing.
- Do not weaken gcc_torture expectations, unsupported markers, allowlists, or
  runtime comparison behavior.
- Do not special-case `src/960209-1.c` or materialize ambiguous bundles in
  RV64 by guessing source ownership.

## Proof

Close-time backend regression guard for the lifecycle-only transition:

```sh
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_before.log
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: pass. Both logs report 345/345 backend tests passing with no new
failures.
