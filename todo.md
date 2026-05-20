Status: Active
Source Idea Path: ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Variadic Aggregate Va Arg Post-Consumption Call Setup

# Current Packet

## Just Finished

Step 1 localized the post-`va_arg` call-setup gap without implementation
edits. The failing source branches are `myprintf` `%7s` and `%9s` in
`tests/c/external/c-testsuite/src/00204.c`: `%7s` consumes
`va_arg(ap, struct s7)` then calls `printf("%.7s", t7.x)`, and `%9s`
consumes `va_arg(ap, struct s9)` then calls `printf("%.9s", t9.x)`.

Semantic BIR is correct for both branches. The `%7s` path reaches
`vaarg.join.14`, copies seven aggregate bytes through `%t25.*` into
`%lv.t7.*`, then records `bir.call i32 printf(ptr @.str31, ptr %lv.t7.0)`.
The `%9s` path reaches `vaarg.join.39`, copies nine bytes through `%t50.*`
into `%lv.t9.*`, then records `bir.call i32 printf(ptr @.str33, ptr %lv.t9.0)`.

Prepared BIR is also correct at the call-plan level. The AArch64 prepared
callsite for `%7s` is `block=9 inst=31`, `direct_extern_variadic`, `args=2`,
with `arg0 bank=gpr from=symbol_address:@.str31 to=x0` and
`arg1 bank=gpr from=frame_slot:stack+672 to=x1`. The `%9s` callsite is
`block=16 inst=41`, also `direct_extern_variadic`, with
`arg0 bank=gpr from=symbol_address:@.str33 to=x0` and
`arg1 bank=gpr from=frame_slot:stack+696 to=x1`. Prepared addressing records
the matching callsite address materializations:
`@.str31` plus `%lv.t7.0` at `vaarg.join.14 inst_index=29`, and `@.str33`
plus `%lv.t9.0` at `vaarg.join.39 inst_index=37`.

Generated AArch64 drops only the fixed format operand publication. In
`build/c_testsuite_aarch64_backend/src/00204.c.s`, `.str31` is the bytes for
`"%.7s"` and `.str33` is the bytes for `"%.9s"`, but the `%7s` branch emits
only `add x1, sp, #8; bl printf`, and the `%9s` branch emits only
`add x1, sp, #15; bl printf`. There is no adjacent
`adrp x0, .str31; add x0, x0, :lo12:.str31` or
`adrp x0, .str33; add x0, x0, :lo12:.str33` before either branch. The
aggregate member address is therefore present in `x1`, while `x0` still holds
the previous `match` result value (`1`) when libc `printf` is called.

Likely owner is AArch64 prepared callsite emission, not semantic BIR,
prepared call planning, string-literal lowering, or aggregate `va_arg`
classification. The narrow code surfaces are
`src/backend/mir/aarch64/codegen/dispatch.cpp` call dispatch ordering around
`lower_address_materializations`, `lower_before_call_moves`,
`materialize_call_boundary_source_to_destination`, and
`materialize_missing_frame_slot_call_arguments`;
`src/backend/mir/aarch64/codegen/calls.cpp` around
`lower_before_call_move`, where symbol-address before-call moves return
`nullopt` when a callsite address materialization exists; and
`src/backend/mir/aarch64/codegen/globals.cpp` around
`lower_address_materializations` plus `make_call_argument_address_record`,
which should bridge a call-argument symbol-address materialization to its ABI
destination register.

## Suggested Next

Repair the general AArch64 callsite path so symbol-address call arguments
materialized at a callsite are emitted into their prepared ABI destination
register before frame-slot aggregate member address publication and before
`bl`. The smallest repair packet should target the prepared AArch64 call
emission path, then prove that `%7s` emits `.str31` into `x0` and `%9s` emits
`.str33` into `x0` while preserving `%lv.t7.0`/`%lv.t9.0` in `x1`.

## Watchouts

Keep the repair semantic and call-plan driven. Do not special-case `00204.c`,
`myprintf`, `%7s`, `%9s`, `.str31`, `.str33`, `x0 == 0x1`, or the observed
stack offsets. Do not reopen idea 328 byval aggregate publication: fixed
aggregate callsites still publish lane data, and this packet localizes a later
post-`va_arg` ordinary call setup gap. HFA/floating residuals remain parked
until this non-HFA aggregate path advances past the format-string publication.

Representative tests for the next packet are the existing focused dump and
machine tests plus the external representative:
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`,
`backend_aarch64_machine_printer`, `backend_aarch64_instruction_dispatch`,
`backend_aarch64_target_instruction_records`, `backend_prepare_liveness`, and
`c_testsuite_aarch64_backend_src_00204_c`. Add focused local coverage in the
repair packet so prepared callsite `symbol_address -> x0` publication after
aggregate `va_arg` is asserted directly, not only through the external runtime.

## Proof

Ran the delegated proof command exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|backend_prepare_liveness|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00204_c)$'`.

Result: build completed with `ninja: no work to do`; 11 of 12 tests passed.
`c_testsuite_aarch64_backend_src_00204_c` still fails with runtime
`Segmentation fault`, matching the localized remaining bug. Proof log:
`test_after.log`.
