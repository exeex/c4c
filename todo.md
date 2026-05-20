Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the classified HFA/floating return lane-home publication owner
for frame-slot lane loads that feed before-return ABI moves. AArch64 memory
lowering now retargets a frame-slot load result to the prepared register home
when that prepared value is the source of a same-block `BeforeReturn`
`FunctionReturnAbi` FPR register move. This keeps the HFA/floating load
publication owner and the return-boundary consumer on the same FPR home without
retargeting GPR scalar frame-slot returns away from their ABI return register.

Focused backend coverage was added in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` as
`hfa_return_frame_slot_lane_load_publishes_prepared_home`. The test builds a
prepared HFA lane load whose storage plan starts in scratch `s8`, whose value
home is `d20`, and whose before-return ABI move targets `s1`; it proves the
lowered load publishes `s20` and the return move consumes `s20`.

Generated-code evidence in `build/c_testsuite_aarch64_backend/src/00204.c.s`:
`fr_hfa12` now lowers lane 1 as `ldr s20, [sp, #4]` followed by `fmov s1, s20`
instead of loading the lane into scratch `s8` while the return move observed
`s20`. `fr_hfa13` now publishes lanes through `s20`/`s21`, and the double HFA
paths publish through `d20`/`d21` before return.

The delegated proof still exposes a later HFA return residual in the caller.
After `bl fr_hfa12`, generated code stores `s9`/`s13` to the local aggregate
result area instead of consuming the return ABI lanes from `s0`/`s1`, so the
return-value runtime line remains `2.0 12.1`. This is no longer the original
callee-side lane home mismatch: the callee publishes lane 1 through the
prepared `s20` home and returns it with `fmov s1, s20`.

The blocking `backend_aarch64_return_lowering` regression from the first
frame-slot retarget was fixed by scoping the new path to FPR return ABI moves.
The existing scalar memory-loaded return still honors `FunctionReturnAbi w0`.

## Suggested Next

Continue Step 2 in the caller-side HFA/floating call-result publication owner.
The new first bad fact is after-call aggregate result storage for HFA returns:
the caller should publish and consume ABI result lanes from `s0`/`s1` before
storing the local aggregate, but `ret` after `bl fr_hfa12` stores stale or
wrong scratch/home registers such as `s9`/`s13`.

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

Four-lane HFA returns still expose a separate residual: lane 3 can spill through
`s16`/`d16` to a stack slot without a corresponding `s3`/`d3` return move. Treat
that as a lane-capacity/call-result publication follow-up, not a reason to undo
the repaired `d20`/`d21` lane-home publication.

Untracked `review/*.md` files were present before this executor packet and were
left untouched.

Files changed in this packet: `src/backend/mir/aarch64/codegen/dispatch.cpp`,
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`, and
`todo.md`.

## Proof

`git diff --check` passed.

Ran the delegated proof command:
`git diff --check`, then
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_return_lowering|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: `git diff --check` passed; build succeeded;
`backend_aarch64_return_lowering` and `backend_aarch64_instruction_dispatch`
passed. `c_testsuite_aarch64_backend_src_00204_c` still failed with
`RUNTIME_NONZERO` / segmentation fault. `test_after.log` is the fresh proof log
for the delegated build and CTest command.

The overall `00204.c` output still shows earlier HFA argument/caller-side
wrong-output lines before `Return values:`. Within the return-value portion, the
first bad return fact remains caller-side HFA call-result publication after
`bl fr_hfa12`, where the caller stores `s9`/`s13` instead of ABI result lanes
`s0`/`s1` before reloading the aggregate for printing. This slice is real
progress because callee-side HFA return lane-home publication now emits
`ldr s20, [sp, #4]` and `fmov s1, s20` for `fr_hfa12`, plus `s20`/`s21` and
`d20`/`d21` lane-home publication for nearby HFA return shapes, while the
scalar GPR return-lowering regression is fixed.
