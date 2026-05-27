# Current Packet

Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish the remaining stale-home baseline

## Just Finished

Step 1 established the fresh four-test stale-home baseline for the active ALU
prepared-authority route. The delegated proof rebuilt successfully, then ran
the exact four-test subset. `00176` and `00181` passed; `00164` and `00204`
failed with runtime mismatches.

The first concrete stale-home sequence to reduce next remains the `00164`
ordinary ALU chain after the `%t106` `printf` call: `%t113 = %t111 & %t112`,
`%t114 = %t110 ^ %t113`, and `%t115 = %t109 | %t114`. The fresh output shows
the same failure slot shape: expected `46`, `1916`, `1916` became unstable
garbage values and `9` in the corresponding actual lines.

## Suggested Next

Create a focused probe under `tests/backend/case/` that isolates the `00164`
post-call ordinary ALU chain and distinguishes whether the stale homes enter at
the assigned result homes for `%t109`/`%t110`/`%t111`/`%t112` or at the ALU
operand-selection point for `%t113`/`%t114`/`%t115`.

## Watchouts

Fresh baseline pass/fail split:
`c_testsuite_aarch64_backend_src_00164_c` failed,
`c_testsuite_aarch64_backend_src_00176_c` passed,
`c_testsuite_aarch64_backend_src_00181_c` passed, and
`c_testsuite_aarch64_backend_src_00204_c` failed.

The `00204` failure still starts in the stdarg output after otherwise-correct
argument and return-value sections; keep it as a secondary signal until the
smaller `00164` ALU chain is reduced.

Do not widen directly into `calls.cpp` under this plan. If the probes prove the
remaining stale-home decision is owned by call argument or call boundary
materialization, stop and route through
`ideas/open/52_aarch64_calls_prepared_authority_repair.md`.

Do not resume `dispatch_value_materialization.cpp` close-readiness work until
the ALU-owned stale-home baseline is reduced or reclassified.

## Proof

Fresh baseline proof ran and is preserved in `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest reported 2/4 passed. Failed:
`c_testsuite_aarch64_backend_src_00164_c`,
`c_testsuite_aarch64_backend_src_00204_c`. Passed:
`c_testsuite_aarch64_backend_src_00176_c`,
`c_testsuite_aarch64_backend_src_00181_c`.
