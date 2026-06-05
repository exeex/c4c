Status: Active
Source Idea Path: ideas/open/110_aarch64_remaining_baseline_failure_recovery.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Pointer and String Mutation Failures

# Current Packet

## Just Finished

Step 3 repaired local array element address/decay publication for AArch64 call
arguments. `00180` now passes by materializing computed local frame addresses
such as `%lv.a.0 + 1` into the destination call ABI register instead of treating
the stale caller-saved home as already containing the computed pointer.

Files changed:
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Before/after subset movement:
- PASS `c_testsuite_aarch64_backend_src_00180_c` after the fix; generated
  assembly now emits `add x1, sp, #1` before `bl printf`.
- PASS `c_testsuite_aarch64_backend_src_00216_c`; this moved with the same
  local address materialization fix.
- PASS `c_testsuite_aarch64_backend_src_00204_c`; still stale-only/passing.
- FAIL `backend_aarch64_instruction_dispatch`; unchanged f64 call-ABI selected
  value publication expectation.
- FAIL `00172`; unchanged pointer compare/materialization runtime mismatch.
- FAIL `00220`; still exits with segmentation fault before output, so wide
  string traversal/materialization remains separate from this fixed local byte
  array address path.

## Suggested Next

Repair `00220` next as the remaining pointer/string mutation failure. Start
from the wide-string local/global materialization and traversal address path,
not from the now-fixed byte-array call argument path.

## Watchouts

`00216` passing is collateral movement from this semantic fix and should be
kept in the proving subset. Do not spend repair effort on `00204` in this route.
Avoid expectation rewrites. The remaining `00220` segfault did not move with
the local frame address call-argument repair, so treat it as a distinct
wide-string materialization/traversal issue.

## Proof

Ran:
`cmake --build --preset default; ctest --test-dir build -j1 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00(172|180|216|220|204)_c)$' > test_after.log 2>&1`

Build completed with no work to do. CTest result: 3/6 passed, 3/6 failed.
Passing targets: `00180`, `00216`, `00204`. Failing targets:
`backend_aarch64_instruction_dispatch`, `00172`, `00220`. Proof log:
`test_after.log`.
