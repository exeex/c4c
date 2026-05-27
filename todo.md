# Current Packet

Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Route scalar load source lookup through prepared memory authority

## Just Finished

Step 3 moved ALU scalar load source lookup off ALU-local value-spelling scans
over `PreparedAddressingFunction::accesses`.

`make_prepared_scalar_load_source` now asks prepared addressing for a unique
memory access by prepared result value id. The unpublished same-block
`load_local` path now asks prepared addressing for a unique same-block,
before-consumer memory access by prepared result value id, then keeps the
ALU-side instruction/memory-access validation before accepting it as a scalar
load operand.

This preserves the current behavior split while relocating the lookup authority
out of `alu.cpp`: the focused probes and `00176`/`00181` pass, and the known
`00164`/`00204` stale-home failures remain for the next producer-recovery
packet.

## Suggested Next

Run Step 4 by replacing same-block unpublished `load_local` producer recovery
with shared prepared scalar-publication or source-producer authority keyed by
source value and consumer instruction index.

## Watchouts

The Step 3 proof is monotonic against the delegated baseline, not fully green:
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c` still fail, while the two focused
probes plus `00176`/`00181` pass.

The remaining stale-home behavior is not fixed by moving the memory-access
lookup authority alone. The next packet should own the same-block source
producer/publication decision rather than adding ALU-local fallbacks.

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

Delegated Step 3 proof ran and is preserved in `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest returned the delegated baseline split, 4/6
passed. Passing tests:
`backend_codegen_route_aarch64_alu_unpublished_load_local_after_call`,
`backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary`,
`c_testsuite_aarch64_backend_src_00176_c`, and
`c_testsuite_aarch64_backend_src_00181_c`. Failing tests:
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c`.
