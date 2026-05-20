Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the remaining callee-side four-lane HFA/floating return
publication residual. AArch64 before-return move lowering now supports
`FunctionReturnAbi` FPR moves whose source home is a prepared frame slot, so a
spilled lane can be reloaded directly into the ABI result register instead of
being silently skipped.

Focused backend coverage was added in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` as
`before_return_stack_hfa_lanes_load_to_abi_result_lanes`. The test models
stack-backed four-lane HFA lane 3 return publications for both float and
double, proving the selected before-return moves print `ldr s3, [sp, #16]`
and `ldr d3, [sp, #32]`.

Generated-code evidence in `build/c_testsuite_aarch64_backend/src/00204.c.s`:
`fr_hfa14` now reloads spilled lane 3 with `ldr s3, [sp, #16]` before return,
and `fr_hfa24` now reloads spilled lane 3 with `ldr d3, [sp, #32]`. The
return-value output advances through `14.1 14.4` and `24.1 24.4`, preserving
the committed lane-home and caller-side ABI lane store repairs.

## Suggested Next

Continue Step 2 in the next return-value residual. The new first bad return
fact is the long double HFA/sret path: `fr_hfa31()` still leaves the caller's
`x8` sret destination uninitialized, so `printf("%.1Lf\n", fr_hfa31().a)`
prints a huge garbage long-double value instead of `31.1`.

## Watchouts

Do not revert the `x8` sret transport or the large-byval indirect pointer
classification to chase the HFA return failure. Those changes are what restore
the `cdefghijklmnopqrs` large-aggregate output and the aggregate string return
block.

Do not broaden this repair into an unconditional frame-slot retarget. The
current fix is intentionally scoped to frame-slot load results whose prepared
value feeds a same-block before-return FPR ABI register move. GPR scalar
returns must continue to use the return ABI register selected by return
lowering.

Do not undo the before-return frame-slot source support to chase the remaining
`00204.c` failure. The float/double HFA return-value lines now pass through
`fr_hfa14` and `fr_hfa24`; the remaining first bad fact is in the long-double
sret copy path, not the four-lane float/double ABI register lane publication.

Untracked `review/*.md` files were present before this executor packet and were
left untouched.

Files changed in this packet: `src/backend/mir/aarch64/codegen/calls.cpp`,
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`, and
`todo.md`.

## Proof

`git diff --check` passed.

Ran the delegated proof command:
`git diff --check`, then
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: `git diff --check` passed; build succeeded;
`backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, `backend_aarch64_instruction_dispatch`, and
`backend_aarch64_return_lowering` passed.
`c_testsuite_aarch64_backend_src_00204_c` still failed with `RUNTIME_NONZERO`
/ segmentation fault. `test_after.log` is the fresh proof log for the delegated
build and CTest command.

The overall `00204.c` output still shows earlier HFA argument wrong-output
lines before `Return values:`. Within the return-value portion, float/double
HFA returns now advance through `fr_hfa14` and `fr_hfa24`; the first bad return
fact moves to the long-double HFA/sret path at `fr_hfa31().a`, where the caller
prints a huge garbage value because the callee does not copy the `f128` return
lane into the `x8` sret destination. This slice is real progress because
four-lane float/double HFA return lanes now publish through all ABI result
lanes, including `s3`/`d3`, while the previous `x8` sret, large-byval, callee
lane-home, caller-side ABI lane store, and scalar GPR return-lowering repairs
remain preserved.
