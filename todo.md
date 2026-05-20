Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the caller-side HFA/floating call-result publication owner for
aggregate result stores after a call. AArch64 dispatch now records prepared
after-call `CallResultAbi` FPR lane moves as live scalar publications and lets
floating local stores consume that live call-result publication instead of the
stale storage-plan/home register when the stored value is an HFA/floating call
result lane.

The repair also fixes AArch64 prepared `CallResult` placement conversion for
multi-lane results. `CallResult` placements now convert slots 0..7 through the
same ABI register mapping as call arguments, so HFA result lane 1+ can publish
as `s1`/`d1`, `s2`/`d2`, and so on instead of only slot 0 being convertible.

Focused backend coverage was added in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` as
`block_dispatch_stores_hfa_call_result_lanes_from_abi_publication`. The test
models a prepared two-lane F32 HFA result whose stale storage plan names FPR
homes such as caller/callee-saved FPR placements, while the call-result ABI
bundle names `s0`/`s1`; it proves the selected aggregate stores consume
`s0`/`s1` with `CallAbi` FPR authority.

Generated-code evidence in `build/c_testsuite_aarch64_backend/src/00204.c.s`:
after `bl fr_hfa12`, the caller now stores `s0` and `s1` to the aggregate
result area instead of `s9`/`s13`. Nearby repaired shapes also now store from
ABI result lanes: `fr_hfa13` uses `s0`/`s1`/`s2`, `fr_hfa14` uses
`s0`/`s1`/`s2`/`s3`, `fr_hfa23` uses `d0`/`d1`/`d2`, and `fr_hfa24` uses
`d0`/`d1`/`d2`/`d3`.

## Suggested Next

Continue Step 2 in the remaining callee-side HFA/floating lane publication
residual. The new first bad fact is no longer caller-side aggregate storage
after `bl fr_hfa12`; that now uses `s0`/`s1`. `00204.c` first goes wrong at
the four-lane float HFA return: `fr_hfa14` prepares lanes `s0`/`s1`/`s2` but
does not publish lane 3 into `s3`, so `printf("%.1f %.1f\n", fr_hfa14().a,
fr_hfa14().d)` prints `14.1 0.0`.

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

Four-lane HFA returns still expose a separate callee-side residual: lane 3 can
spill through `s16`/`d16` to a stack slot without a corresponding `s3`/`d3`
return move. Treat that as the next lane-capacity publication repair, not a
reason to undo the repaired caller-side ABI lane stores or the `d20`/`d21`
lane-home publication.

Untracked `review/*.md` files were present before this executor packet and were
left untouched.

Files changed in this packet: `src/backend/mir/aarch64/abi/abi.cpp`,
`src/backend/mir/aarch64/codegen/dispatch.cpp`,
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
lines before `Return values:`. Within the return-value portion, caller-side HFA
aggregate result storage now advances through `fr_hfa12`/`fr_hfa13`; the first
bad return fact is the callee-side four-lane `fr_hfa14` lane 3 publication,
where `.d` prints `0.0`. This slice is real progress because the caller-side
aggregate stores now consume ABI lanes (`s0`/`s1` for `fr_hfa12`, wider
`s0..s3` and `d0..d3` for nearby shapes) instead of stale scratch/home FPRs,
while the previous `x8` sret, large-byval, callee lane-home, and scalar GPR
return-lowering repairs remain preserved.
