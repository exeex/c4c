Status: Active
Source Idea Path: ideas/open/555_rv64_prepared_local_memory_addressing_src_960209_1.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Classify Local-Memory Facts

# Current Packet

## Just Finished

Lifecycle routing closed the completed out-of-SSA parallel-copy move-bundle
publication route and activated the RV64 prepared local-memory addressing
route for the new `src/960209-1.c` first blocker.

## Suggested Next

Execute Step 1 by reproducing or inspecting the current
`unsupported_local_memory_access` case log and classifying whether the failing
operation has prepared frame-slot or pointer-value base-plus-offset facts
available to RV64 object emission.

## Watchouts

- Do not infer local-memory addresses from raw target or testcase shape.
- If prepared address facts are missing, record the producer-owned missing
  facts and request lifecycle routing instead of implementing an RV64 guess.
- Keep idea 547 as broader bucket-review context; do not silently expand this
  route into all local-memory rows.

## Proof

Close gate for idea 554 used existing matching `test_before.log` and
`test_after.log` composite proof logs. The CTest subset stayed green, the
one-row scan advanced past `prepared_consumer_category=missing_move_bundle`,
and regression guard passed with non-decreasing pass-count policy:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Supervisor acceptance also reported:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

The backend bucket passed `345/345`.
