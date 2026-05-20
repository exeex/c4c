Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the localized fixed mixed-HFA all-or-nothing failure for the
`fa3(hfa14, hfa23, hfa32)` boundary. The BIR module handoff now recognizes
consecutive flattened fixed-HFA lane names, applies AArch64 FP/SIMD register
pressure as a group, and marks the whole group stack-passed when
`next_fp + lane_count > 8`. This keeps the two-lane `f128` `hfa32` group out
of `q7` when only one FP/SIMD argument register remains.

The caller and callee now agree for the repaired `hfa32` group. Generated
`arg` code before `bl fa3` loads `%t136` and `%t138` from their frame slots and
stores them to outgoing stack offsets `[sp]` and `[sp,#16]`. Generated `fa3`
entry code loads incoming `[sp,#192]` and `[sp,#208]` after its frame
allocation and seeds the local F128 carrier slots before the existing
`store_local` lowering consumes `%p.c.hfa0` and `%p.c.hfa1`. The previous
split-register publication through `q7` is gone.

Focused backend coverage now pins the repaired owner. The
`backend_aarch64_instruction_dispatch` bucket includes a LIR-to-BIR mixed
fixed-HFA pressure fixture that verifies the overflowing two-lane `fp128`
group is stack-passed on both the callee formal ABI and caller call ABI, plus
dispatch fixtures for the F128 frame-slot to outgoing-stack handoff and the
callee entry publication that seeds stack-passed F128 HFA lanes with `none`
homes.

`00204.c` is still not green. The new first bad fact has moved: the observing
`fa3` line now prints the repaired binary128 values as `32.1 32.2`, but the
preceding `hfa23` double values still print as `0.0 0.0`, giving
`14.1 14.4 0.0 0.0 32.1 32.2`. The remaining corruption and later segfault are
therefore downstream of a different fixed-HFA/double publication or earlier
state issue, not the localized mixed-HFA `hfa32` all-or-nothing split.

## Suggested Next

Continue Step 2 with the new first bad fact: localize why the `fa3` `hfa23`
double lanes reaching `d4..d6` produce `0.0` for the observed first and third
double values while the repaired `hfa32` stack lanes are now correct. Keep this
separate from the fixed `hfa32` all-or-nothing repair; do not unwind the group
spill or F128 stack handoff paths without fresh regression evidence.

## Watchouts

The HFA grouping bridge is intentionally general over flattened lane names
ending in `.hfaN`; it is not a special case for `fa3`, `hfa32`, one register,
one stack slot, or one emitted instruction sequence. The AArch64 F128 stack
argument copy path uses existing prepared F128 carrier transport. The callee
entry bridge only seeds stack-passed F128 formals whose prepared home is still
`none`, matching the current prepared-route representation for these spilled
fixed-HFA lanes.

Untracked `review/*.md` files were present before this executor packet and were
left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded with no work to do;
`backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, and `backend_aarch64_instruction_dispatch`
passed, including the new focused HFA/F128 owner coverage.
`c_testsuite_aarch64_backend_src_00204_c` still failed with `RUNTIME_NONZERO`
/ segmentation fault. The fresh first bad output is still the
mixed-HFA `fa3` line `14.1 14.4 0.0 0.0 32.1 32.2`, followed by corrupted
later output. `test_after.log` is the fresh proof log for this repair and
remaining blocker.

Also ran `git diff --check`; it passed with no whitespace errors.
