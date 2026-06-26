Status: Active
Source Idea Path: ideas/open/383_rv64_global_aggregate_lane_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Global Aggregate Lane Facts

# Current Packet

## Just Finished

Lifecycle switch completed after closing
`ideas/closed/382_rv64_object_route_prepared_local_memory_addressing.md`.
The new active plan is
`ideas/open/383_rv64_global_aggregate_lane_materialization.md`, starting at
Step 1.

## Suggested Next

Audit the `src/20030914-2.c` prepared dump and RV64 object-route diagnostic
path for `main`, block `entry`, inst `0`:

```text
%t0.global.aggregate.load.0 = bir.load_local i32 %t0.0, addr gs
```

Record whether the source global aggregate facts needed for semantic lowering
are already published by prepared BIR/module data or whether this must route to
an upstream prepared-data publication owner.

## Watchouts

- Do not reopen idea 357's broad globals/strings/data-section scope unless the
  audit proves the prepared module already publishes complete aggregate data
  facts and only target consumption remains.
- Do not treat `load_local ... addr gs` as stack local memory or infer
  aggregate bytes from source syntax, raw offsets, or log text.
- `src/20030914-2.c` and `gs` are representative evidence only, not behavior
  keys.

## Proof

Closure gate for idea 382:

```sh
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_before.log
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_before.log \
  --after test_after.log \
  --allow-non-decreasing-passed
```

Result: passed. Both canonical logs report `1/1` passing tests; the
non-decreasing flag is used because this was a lifecycle-only close/switch
after the implementation slice had already landed.
