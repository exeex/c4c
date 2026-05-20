Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the next large-aggregate/sret owner after the `fa4` fixed
formal repair. AArch64 now classifies byval aggregates larger than 16 bytes as
indirect pointer arguments in a GPR instead of stack payload lanes, and sret
pointers now use the AAPCS64 `x8` transport without consuming normal `x0..x7`
argument slots.

The prepared call plan now derives memory-return homes from split aggregate
frame slots such as `%t0.0`, `%t0.1`, etc. when no exact `%t0` object exists.
Call-boundary lowering can materialize frame-slot addresses for sret storage
and indirect large-byval aggregate arguments while preserving the existing
small byval lane-copy route.

Focused backend coverage was added in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` for the
repaired owners:
`semantic_split_sret_memory_return_discovers_first_aggregate_frame_slot`,
`sret_call_argument_materializes_x8_and_keeps_next_gpr_at_x0`,
`large_byval_aggregate_indirect_argument_materializes_frame_address`, and
`before_return_fpr_move_retargets_source_view_for_hfa_lane`.

Generated-code evidence in `build/c_testsuite_aarch64_backend/src/00204.c.s`:
`fa_s17` is now called with `add x0, sp, #1064; bl fa_s17` and the callee reads
through `x0`; sret return callees such as `fr_s4` use `%ret.sret` in `x8` and
store bytes through `[x8]`. The runtime output now includes the previously
missing `cdefghijklmnopqrs` argument line and all aggregate string return lines
through `cdefghijklmnopqrs`.

The remaining first bad fact is now HFA/floating return publication. Prepared
BIR for `fr_hfa12` says lane 1 lives in `%t0.ret.aggregate.lane.1` with home
`d20` and a before-return move to `s1`; generated assembly loads the lane from
the frame slot into scratch `s8`, not into that prepared home. The return
boundary can now view-retarget `d20` as `s20` for the ABI move, but the upstream
HFA lane-home publication is still missing, so `fr_hfa12()` prints `0.0 12.1`
instead of `12.1 12.2`.

## Suggested Next

Continue Step 2 in the HFA/floating lane-home publication owner. The narrow
owner is frame-slot/global aggregate lane loads and before-return/call-result
publication for FPR homes: prepared homes name `d20`/`d21`, while generated
loads and consumers still use scratch `s8`/`s9`/`s16` in several HFA paths.

## Watchouts

Do not revert the `x8` sret transport or the large-byval indirect pointer
classification to chase the HFA return failure. Those changes are what restore
the `cdefghijklmnopqrs` large-aggregate output and the aggregate string return
block.

Do not broaden frame-slot load retargeting blindly. A trial retarget of all
prepared frame-slot load results to their register homes made HFA homes visible
but left dependent scalar uses on the old scratch register, corrupting earlier
HFA argument output. The next fix needs to update the lane-home publication and
consumer authority together.

Untracked `review/*.md` files were present before this executor packet and were
left untouched.

Files changed in this packet: `src/backend/bir/lir_to_bir/calling.cpp`,
`src/backend/prealloc/target_register_profile.cpp`,
`src/backend/prealloc/regalloc/call_return_abi.cpp`,
`src/backend/prealloc/regalloc/value_homes.cpp`,
`src/backend/prealloc/call_plans.cpp`,
`src/backend/mir/aarch64/codegen/dispatch.cpp`,
`src/backend/mir/aarch64/codegen/calls.cpp`,
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`, and
`todo.md`.

## Proof

`git diff --check` passed.

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded;
`backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, and
`backend_aarch64_instruction_dispatch` passed. `c_testsuite_aarch64_backend_src_00204_c`
still failed with `RUNTIME_NONZERO` / segmentation fault after printing the
restored large-aggregate argument and aggregate-return strings. The first
remaining bad return line is `0.0 12.1` for `fr_hfa12()`; `test_after.log` is
the fresh proof log for this packet and remaining blocker.
