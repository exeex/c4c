# Current Packet

Status: Active
Source Idea Path: ideas/open/56_aarch64_edge_terminator_consumer_preservation_repair.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Audit consumers of the preserved source

## Just Finished

Step 4 made AArch64 stack-published simple integer cast lowering consume the
prepared predecessor-edge preservation result.

`lower_scalar_cast_publication_to_prepared_stack` now detects a prepared
block-entry out-of-SSA edge preservation move targeting the current successor
and matching the cast operand/result value ids. When present, it reloads the
preserved stack destination and emits the cast from that stack value instead of
using stale emitted-register state for the source.

For the `myprintf` route, the stale `mov w9, w13; sxtw x9, w9` sequence is gone.
The generated assembly now uses the preserved stack result, for example
`ldr w9, [sp, #920]; sxtw x9, w9` and the analogous `#976` path. No tests or
expectation files were changed.

## Suggested Next

Implement the next narrow `00204` stdarg-string repair. The remaining mismatch
is after the stale scalar source path is removed: `00204` still fails in the
stdarg string output, producing duplicated/corrupt text around the first
`myprintf` string group while the HFA and MOVI sections continue to match.

## Watchouts

Do not edit or revert the existing dirty `memory.cpp` and
`dispatch_edge_copies.cpp` context during lifecycle-only work. They remain
incomplete implementation context until a supervisor accepts, splits, or
reroutes them.

Do not reload `%lv.ap.24` in the join block. Do not special-case `00204`,
`myprintf`, `%t35`, `%t45`, `%t49`, `vaarg.join.39`, or `x13`.

The cast-side query intentionally follows prepared predecessor block-entry
bundles by semantic source/destination value ids and successor label. It does
not use raw BIR spelling, block names, physical register text, or named-case
matching.

The focused subset still fails only `00204`. The remaining owner is no longer
the `%t35 -> %t45.byte_offset.i64` stale scalar source path; inspect the
remaining stdarg string pointer/copy route before touching scalar cast lowering
again.

## Proof

Focused proof command run for this packet:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result after this Step 4 packet: build passed, `7/8` focused tests passed, and
only `c_testsuite_aarch64_backend_src_00204_c` failed with
`[RUNTIME_MISMATCH]`. Same-feature probes
`backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy`,
`backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy`,
`backend_codegen_route_aarch64_alu_unpublished_load_local_after_call`, and
`backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary` passed.

Proof log: `test_after.log`.
