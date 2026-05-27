# Current Packet

Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Route ALU select-chain publication through shared select authority

## Just Finished

Step 6 routed `lower_scalar_select_publication` through shared prepared
select-chain authority.

The direct-global select-chain scalar publication path now asks
`find_prepared_direct_global_select_chain_dependency` for the prepared root and
passes that dependency into `emit_select_chain_value_to_register` instead of
using the ALU-owned `select_chain_contains_direct_global_load` shortcut.

The delegated proof preserved the Step 5 behavior split: both focused ALU
probes and `00176`/`00181` pass, while the known `00164`/`00204` stale-home
runtime failures remain.

## Suggested Next

Run the next ALU prepared-authority packet against the remaining `00164`
post-`%t106` stale-home chain, or request a route review if the supervisor
wants confirmation before widening beyond select-chain scalar publication.

## Watchouts

The Step 6 proof is monotonic against the delegated Step 5 baseline, not fully
green:
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c` still fail, while the two focused
probes plus `00176`/`00181` pass.

The remaining stale-home behavior is not fixed by moving ALU select-chain
publication into shared prepared authority. The next packet should not add
ALU-local fallbacks for this path.

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

Delegated Step 6 proof ran and is preserved in `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest returned the delegated baseline split, 4/6
passed. Passing tests:
`backend_codegen_route_aarch64_alu_unpublished_load_local_after_call`,
`backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary`,
`c_testsuite_aarch64_backend_src_00176_c`, and
`c_testsuite_aarch64_backend_src_00181_c`. Failing tests:
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c`.
