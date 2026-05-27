# Current Packet

Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish the remaining stale-home baseline

## Just Finished

Lifecycle switched from the parked dispatch value materialization route to the
ALU prepared-authority route. The old dispatch source idea remains open; it was
not closed because Step 8 broader validation exposed an ALU-owned blocker
rather than proving dispatch source-idea completion.

## Suggested Next

Establish the fresh four-test baseline for:
`c_testsuite_aarch64_backend_src_00164_c`,
`c_testsuite_aarch64_backend_src_00176_c`,
`c_testsuite_aarch64_backend_src_00181_c`, and
`c_testsuite_aarch64_backend_src_00204_c`.

After the baseline, split the remaining stale-home seam into focused probes
under `tests/backend/case/` before changing ALU lowering.

## Watchouts

Previous executor proof for the same four-test family passed `00176` and
`00181` after the select-chain label-identity repair, but still failed runtime
checks in `00164` and `00204`.

The known `00164` failure shape is ordinary ALU operands reading stale or
uninitialized homes after the `%t106` `printf` call, including stack slots for
`%t110`/`%t111`/`%t112` and register `x13` for `%t109`. The concrete sequence
reported by the executor was `%t113 = %t111 & %t112`,
`%t114 = %t110 ^ %t113`, and `%t115 = %t109 | %t114`.

Do not widen directly into `calls.cpp` under this plan. If the probes prove the
remaining stale-home decision is owned by call argument or call boundary
materialization, stop and route through
`ideas/open/52_aarch64_calls_prepared_authority_repair.md`.

Do not resume `dispatch_value_materialization.cpp` close-readiness work until
the ALU-owned stale-home baseline is reduced or reclassified.

## Proof

No validation run by this lifecycle-only switch. Next proof should run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`
