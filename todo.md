# Current Packet

Status: Active
Source Idea Path: ideas/open/56_aarch64_edge_terminator_consumer_preservation_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Validate and decide dirty-context acceptance

## Just Finished

Step 5 classified the remaining `00204` stdarg string mismatch after the
edge-preservation and cast-source fixes.

The existing dirty `memory.cpp` cursor-update path already emits the semantic
overflow cursor update from the loaded va_list field value, and the generated
`myprintf` `%7s`/`%9s` va_arg cursor shape matches the AAPCS64 register-save
and overflow-area pattern. The existing dirty `dispatch_edge_copies.cpp`
source-register reuse is not enough to make the focused subset green.

The remaining stdarg corruption is not owned by the delegated files. The
failure is in the by-value aggregate variadic call argument lane feeding
`myprintf`'s register-save area: the first `%9s` call still corrupts the third
and later string payloads even though `myprintf` recomputes the va_arg source
from `gr_top + gp_offset` and advances the cursor by the semantic aggregate
stride. That owner is the AArch64 call/byval argument publication route, not
`memory.cpp` va_list field stores or `dispatch_edge_copies.cpp` edge source
selection.

## Suggested Next

Delegate a narrow call/byval argument-publication packet to the owner of
AArch64 variadic by-value aggregate argument lanes. The packet should inspect
how 9-byte and 7-byte aggregate payload lanes are materialized into GPRs and
overflow stack argument slots for direct variadic calls, using the existing
`00204` first two `myprintf` string calls and the focused byval helper runtime
coverage as probes.

## Watchouts

Do not reload `%lv.ap.24` in the join block. Do not special-case `00204`,
`myprintf`, `%t35`, `%t45`, `%t49`, `vaarg.join.39`, or `x13`.

Do not reopen scalar cast source selection for `%t35 -> %t45.byte_offset.i64`;
the stale cast source sequence remains absent in generated assembly. Do not
treat the existing dirty `memory.cpp` and `dispatch_edge_copies.cpp` edits as
accepted while `00204` remains red.

The remaining investigation should distinguish call-argument lane publication
from callee va_arg consumption. In the current generated `myprintf`, `%9s`
register-save consumption uses `gr_top + gp_offset` and a 16-byte progression;
the corrupt payload therefore points upstream at the values placed in those
lanes for the variadic call.

## Proof

Focused proof command run for this packet:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result after this Step 5 packet: build passed, `7/8` focused tests passed, and
only `c_testsuite_aarch64_backend_src_00204_c` failed with
`[RUNTIME_MISMATCH]`. Same-feature probes
`backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy`,
`backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy`,
`backend_codegen_route_aarch64_alu_unpublished_load_local_after_call`, and
`backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary` passed.

Proof log: `test_after.log`.
