Status: Active
Source Idea Path: ideas/open/14_c_aggregate_initializer_compound_literal_layout.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce And Localize Aggregate Layout Failure

# Current Packet

## Just Finished

Step 1 reproduced `c_testsuite_aarch64_backend_src_00216_c` with the delegated
proof and localized the first bad owner. The test builds and links, then
segfaults at runtime. `gdb` shows the crash in `print_` while called from
`foo`, specifically the second local print (`print(ls2)`): `print_` receives a
bad data pointer in `x1` and faults on `ldrb w9, [x13]`.

The frontend/HIR-to-BIR fact is still correct at that point:
`--dump-bir --target aarch64-unknown-linux-gnu` emits
`bir.call void print_(ptr @.str4, ptr %lv.ls2, i64 4)`. The first bad fact
appears after preparation/AArch64 lowering. `--dump-prepared-bir
--mir-focus-function foo` assigns `%lv.ls2` value id 269 to
`stack_slot:slot#434:stack+552` and emits a before-call move with reason
`call_arg_stack_to_register`; the final AArch64 asm for that call then uses
`ldr x1, [sp, #0x228]` before `bl print_`. No instruction stores a frame
address for `%lv.ls2` into `[sp,#0x228]`, so the call argument is loaded from
an uninitialized spill slot instead of materializing the local aggregate frame
address (`sp + 4`).

## Suggested Next

Delegate Step 2 to build a focused reduced AArch64 backend case for passing the
address of a local aggregate/array object to a direct call after an earlier
call. The expected missing rule is that frame-address values such as
`%lv.ls2` must be materialized from their local stack slot address for call
arguments, not loaded from a separate uninitialized preserved-value spill slot.

## Watchouts

This is not currently localized to aggregate initializer semantics: the local
bytes for `ls2` are stored correctly and BIR names `%lv.ls2` correctly. The
suspect path is the prepared value-location/call move handoff into the AArch64
call argument emitter, around `call_arg_stack_to_register` handling for local
frame-address operands. Do not repair this by special-casing `00216`, a field
layout, or one initializer spelling.

## Proof

Ran exactly:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00216_c$') > test_after.log 2>&1`

Result: build succeeded, test failed with `[RUNTIME_NONZERO]` and
`exit=Segmentation fault`. Proof log: `test_after.log`.
