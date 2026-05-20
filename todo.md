Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the long-double HFA/sret return residual. AArch64 F128 memory
transport now resolves prepared pointer-value bases through the same prepared
home/storage authority used by ordinary memory stores, so binary128 carrier
stores through a caller-provided sret pointer print through the real pointer
register instead of failing selection with an unprintable symbolic pointer.

Focused backend coverage was added in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` as
`block_dispatch_lowers_f128_pointer_store_from_prepared_carrier`. The test
models a full-width F128 carrier store to a prepared pointer home in `x8`,
proving the transport preserves pointer base authority and prints
`str q5, [x8]`.

Generated-code evidence in `build/c_testsuite_aarch64_backend/src/00204.c.s`:
`fr_hfa31` now copies the reloaded long-double lane into the caller-provided
sret destination with `str q13, [x8]`. The same generic pointer-store path
emits `[x8, #16]`, `[x8, #32]`, and `[x8, #48]` stores for the wider
`fr_hfa32`/`fr_hfa33`/`fr_hfa34` sret shapes. With unbuffered runtime output,
the return-value section now advances through `31.1`, `32.1 32.2`,
`33.1 33.3`, and `34.1 34.4`.

## Suggested Next

Continue Step 2 in the next `00204.c` residual. The new first bad fact is after
the return-value section: runtime output reaches `stdarg:` and then segfaults
before printing the expected first stdarg aggregate line
`ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI`.

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
`fr_hfa14` and `fr_hfa24`; the remaining first bad fact is after the
return-value block, not the four-lane float/double ABI register lane
publication.

Do not undo the F128 pointer-value memory transport repair to chase the
remaining stdarg segfault. Long-double HFA returns now copy through `x8`, and
the next failure is after the return-value block has completed.

Untracked `review/*.md` files were present before this executor packet and were
left untouched.

Files changed in this packet: `src/backend/mir/aarch64/codegen/memory.cpp`,
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
HFA returns still advance through `fr_hfa14` and `fr_hfa24`, and long-double
HFA/sret returns now advance through `fr_hfa31`..`fr_hfa34`. The new first bad
fact moves to the stdarg block: with unbuffered runtime output the program
prints `stdarg:` and then segfaults before the first expected stdarg aggregate
line. This slice is real progress because F128 HFA sret returns now copy their
payload into the caller-provided `x8` destination while the previous `x8` sret,
large-byval, callee lane-home, caller-side ABI lane store, spilled lane reload,
and scalar GPR return-lowering repairs remain preserved.
