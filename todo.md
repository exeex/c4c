Status: Active
Source Idea Path: ideas/open/330_aarch64_non_hfa_aggregate_va_arg_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Non-HFA Aggregate Va Arg Materialization

# Current Packet

## Just Finished

Step 1 localized the non-HFA aggregate `va_arg` materialization failure in
`c_testsuite_aarch64_backend_src_00204_c` without implementation edits.
The failing branches are `myprintf` `match(&s, "%7s")` and
`match(&s, "%9s")` in `tests/c/external/c-testsuite/src/00204.c`.

Prepared BIR for `--target aarch64-linux-gnu --dump-prepared-bir
--mir-focus-function myprintf` has the intended materialization:
`%7s` selects `%t24` from `%lv.ap.8 + %t10` or `%lv.ap.0`, loads 7 bytes
through `%t25.0`..`%t25.6`, and stores them to `%lv.t7.0`..`%lv.t7.6`.
`%9s` selects `%t49` from `%lv.ap.8 + %t35` or overflow `%t23`, loads
9 bytes through `%t50.0`..`%t50.8`, and stores them to `%lv.t9.0`..`%lv.t9.8`.
The expected copied bytes are `lmnopqr` for `%7s` and `ABCDEFGHI` for `%9s`.

The generated assembly in `build/c_testsuite_aarch64_backend/src/00204.c.s`
loses those copies. In `%7s`, `.LBB154_16`/`.LBB154_19`/`.LBB154_20`
select a source pointer into `x13` from `[sp, #928]` or `[sp, #24]`, then
immediately call `printf` with `.str31` in `x0` and `add x1, sp, #8`;
there are no `ldrb`/`strb` copies from `x13` into `[sp, #8]`..`[sp, #14]`.
In `%9s`, `.LBB154_23`/`.LBB154_26`/`.LBB154_27`/`.LBB154_25` select a
source pointer into `x13` from `[sp, #976]` or `[sp, #632]`, advance the
overflow pointer, then call `printf` with `.str33` in `x0` and
`add x1, sp, #15`; there are no copies from `x13` into
`[sp, #15]`..`[sp, #23]`.

Owner is AArch64 lowering of prepared pointer-addressed memory copies, not
source selection or call-operand publication. Concrete code surfaces:
`src/backend/mir/aarch64/codegen/dispatch.cpp` publication helpers
(`emit_value_publication_to_register`, `emit_edge_load_local_to_register`,
`lower_store_local_with_address_materialization`, ordinary memory dispatch)
and `src/backend/mir/aarch64/codegen/memory.cpp` prepared memory record
construction/lowering for `LoadLocalInst` and `StoreLocalInst` with
`PreparedAddressBaseKind::PointerValue`. The prepared planner surface
`src/backend/prealloc/variadic_entry_plans.cpp` should stay a guardrail:
the first two non-HFA branches are already represented as byte-copy
prepared BIR, while later HFA `llvm.va_arg.aggregate` helper paths use
`src/backend/mir/aarch64/codegen/variadic.cpp`.

## Suggested Next

Delegate Step 2 repair for the general AArch64 pointer-addressed
`LoadLocalInst` to destination stack-slot `StoreLocalInst` byte-copy path
that non-HFA aggregate `va_arg` materialization depends on. A minimal local
proof should cover a non-HFA aggregate `va_arg` byte-copy shape before the
external representative; then run:
`ctest --test-dir build -j --output-on-failure -R '^(backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|c_testsuite_aarch64_backend_src_00204_c)$'`.

## Watchouts

Do not repair this by special-casing `00204.c`, `%7s`, `%9s`, `x13`,
`sp + 8`, or `sp + 15`. The source-selection branches and final call
publication are currently good enough to expose the missing byte copy:
`.str31`/`.str33` reach `x0` and destination addresses reach `x1`. Keep
HFA `llvm.va_arg.aggregate` helper lowering separate unless the non-HFA
copy repair advances the representative to a fresh HFA first bad fact.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|c_testsuite_aarch64_backend_src_00204_c$)' | tee test_after.log`.
Build was already up to date. Result: 140/141 selected tests passed; the
only failure is the representative
`c_testsuite_aarch64_backend_src_00204_c`, still with corrupted `stdarg`
output. Proof log path: `test_after.log`.
