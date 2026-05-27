# Current Packet

Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Replace same-block load-local producer recovery

## Just Finished

Step 4 moved same-block unpublished `load_local` producer recovery out of
`alu.cpp`.

`make_unpublished_load_local_source_operand` now consumes the shared prepared
same-block load-local source-producer query. The query is keyed by source value
and consumer instruction index, validates the prepared source producer and
memory access, and owns the no-intervening-store safety check before ALU reads
the prepared frame-slot source.

This removes ALU-local producer/no-intervening-store scans while preserving the
Step 3 behavior split: both focused probes and `00176`/`00181` pass, and the
known `00164`/`00204` runtime failures remain.

## Suggested Next

Run Step 5 by replacing before-return ABI and return-chain recovery with shared
prepared move or return-chain authority.

## Watchouts

The Step 4 proof is monotonic against the delegated baseline, not fully green:
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c` still fail, while the two focused
probes plus `00176`/`00181` pass.

The remaining stale-home behavior is not fixed by moving same-block
unpublished `load_local` producer and store-interference decisions into shared
authority. The next packet should not add ALU-local fallbacks for these paths.

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

Delegated Step 4 proof ran and is preserved in `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest returned the delegated baseline split, 4/6
passed. Passing tests:
`backend_codegen_route_aarch64_alu_unpublished_load_local_after_call`,
`backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary`,
`c_testsuite_aarch64_backend_src_00176_c`, and
`c_testsuite_aarch64_backend_src_00181_c`. Failing tests:
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c`.
