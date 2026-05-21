Status: Active
Source Idea Path: ideas/open/380_aarch64_function_pointer_table_relocation_dispatch.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Table Relocation First Bad Fact

# Current Packet

## Just Finished

Lifecycle switch from closed idea 379 to focused follow-up idea 380.

## Suggested Next

Delegate Step 1: localize why `00216.c` `test_multi_relocs` prints
`two/two/two` instead of `one/two/three`, tracing `table[0]`, `table[1]`, and
`table[2]` through initializer handling, relocation/global-data emission,
AArch64 address materialization, and indexed indirect-call dispatch.

## Watchouts

Do not fold this back into aggregate copy/load publication without fresh
evidence of a shared lowering boundary. Idea 379 is closed because `ls21`,
`lu22`, `lv3`, and `flow` now match expected output and the remaining mismatch
is the previously parked function-pointer-table/relocation route.

## Proof

Close/switch gate used the existing canonical logs from the delegated
backend-focused proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|c_testsuite_aarch64_backend_src_00216_c$)'; } > test_after.log 2>&1`

`test_after.log` reports all 146 backend tests passed and the same single
`c_testsuite_aarch64_backend_src_00216_c` failure remains. The failure advanced
out of idea 379 scope: `ls21`, `lu22`, `lv3`, and `flow` match expected output,
and the first remaining mismatch is `test_multi_relocs` printing `two/two/two`
instead of `one/two/three`.

Regression guard note: strict monotonic mode rejected the close because the
total pass count stayed 146/147 while the same representative test still fails.
Non-decreasing mode passed with no new failures:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`.
