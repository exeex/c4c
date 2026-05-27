# Current Packet

Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Repair before-return and return-chain authority

## Just Finished

Step 5 routed before-return ABI and return-chain register recovery through
shared prepared authority.

`find_return_abi_register` now consumes the prepared before-return ABI move
lookup keyed by source value id, block index, and destination register bank
instead of scanning raw move bundles in ALU.

Return-chain recovery now consumes a shared prepared return-chain lookup built
with the per-function prepared lookups. ALU no longer walks forward through BIR
name chains locally before retargeting a return ABI register.

The delegated proof preserved the Step 4 behavior split: both focused ALU
probes and `00176`/`00181` pass, while the known `00164`/`00204` stale-home
runtime failures remain.

## Suggested Next

Run Step 6 by routing `lower_scalar_select_publication` through shared select
authority, unless the supervisor first wants a review because Step 5 was
monotonic rather than green.

## Watchouts

The Step 5 proof is monotonic against the delegated Step 4 baseline, not fully
green:
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c` still fail, while the two focused
probes plus `00176`/`00181` pass.

The remaining stale-home behavior is not fixed by moving before-return ABI and
return-chain decisions into shared prepared authority. The next packet should
not add ALU-local fallbacks for these paths.

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

Delegated Step 5 proof ran and is preserved in `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest returned the delegated baseline split, 4/6
passed. Passing tests:
`backend_codegen_route_aarch64_alu_unpublished_load_local_after_call`,
`backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary`,
`c_testsuite_aarch64_backend_src_00176_c`, and
`c_testsuite_aarch64_backend_src_00181_c`. Failing tests:
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c`.
