Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the next fixed HFA/floating residual after the fixed `hfa14`
lane collision. Fresh generated-code evidence showed `fa_hfa34` received all
four binary128 lanes in `q0`, `q1`, `q2`, and `q3`, stored them at `[sp]`,
`[sp,#16]`, `[sp,#32]`, and `[sp,#48]`, but only published `v0`, `v1`, and
`v2` before its `printf` call; lane `d` was reloaded into a temporary and never
published as ABI argument `q3`.

The concrete owner was AArch64 call-boundary frame-slot argument publication in
`src/backend/mir/aarch64/codegen/calls.cpp`: stack-backed 16-byte `f128`
arguments selected for ABI q registers had prepared move/binding facts, but the
frame-slot source path only accepted scalar FPR/GPR destinations. The repair
adds a general prepared frame-slot `f128` to q-register call-argument path,
prints `ldr qN, [sp, #offset]`, and selects that node for prepared 16-byte
frame-slot sources.

Focused coverage now exercises the repaired owner in target instruction
records, machine printing, and instruction dispatch. Fresh generated
`fa_hfa34` assembly now emits the fourth-lane publication before `printf`, and
the proof output prints `34.1 34.2 34.3 34.4` for the fixed `hfa34` line.

## Suggested Next

Continue Step 2 on the newly exposed representative failure after fixed
`fa_hfa34`. The fresh proof still fails with `RUNTIME_NONZERO` / segmentation
fault. The next concrete first bad fact is the fixed mixed-HFA `fa3` output:
it starts with the repaired float lanes `14.1 14.4`, but then prints `0.0 0.0`
for the `hfa23` double lanes and corrupts the following `hfa32` long-double
lane. Generated `fa3` evidence shows `%p.c.hfa1` has `home kind=none`, the
caller publishes only `q7` for `hfa32`, and `fa3` reloads `q5` from `[sp,#16]`
without an initialized second binary128 lane. Localize whether the next owner
is HFA fixed-argument classification/publication when remaining FPR/SIMD
argument registers cannot hold the whole HFA, or a related formal-entry home
publication gap.

## Watchouts

The proof is not green. Treat this as a repaired `hfa14` lane collision plus
repaired binary128 frame-slot-to-q call-argument publication, not as completed
`00204.c` support. Do not special-case `fa_hfa34`, `fa3`, `fa4`, one lane
count, one literal, one emitted register, one stack slot, or one test case. Do
not reopen committed HFA lane-home, F128 transport/address, call ABI
register-indexing, binary128 literal, `fa_hfa13` scalar lane, `hfa14`, or
`fa_hfa34` frame-slot publication repairs without fresh evidence proving
regression.

Untracked `review/*.md` files were present before this executor packet and were
left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded; `backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, and `backend_aarch64_instruction_dispatch`
passed. `c_testsuite_aarch64_backend_src_00204_c` still failed with
`RUNTIME_NONZERO` / segmentation fault after the fixed `fa_hfa34` line and the
new mixed-HFA `fa3` first bad fact. `test_after.log` is the fresh proof log for
this dirty state.

Also ran `git diff --check`, which passed.
