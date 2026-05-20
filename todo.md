Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the next fixed HFA residual after the `fa_hfa13` repair. Fresh
generated-code evidence showed the `arg()` call site already published `hfa14`
lanes into `s0`, `s1`, `s2`, and `s3`, but `fa_hfa14` stored those lanes at
`[sp]`, `[sp,#4]`, `[sp,#8]`, and `[sp,#12]` and then converted lanes `c/d`
from stale spill homes at `[sp,#20]` and `[sp,#24]`.

The concrete owner was the stack-backed scalar F32/F64 width-cast fallback in
`src/backend/mir/aarch64/codegen/cast_ops.cpp`: for a cast operand that is a
prepared `load_local` result, it used the regalloc spill home as the source
address even when prepared addressing had the authoritative frame-slot load
home. The fallback now prefers the unique prepared frame-slot load access for
the operand value before falling back to the value-home offset.

The fresh generated `fa_hfa14` assembly now converts lane `c` from `[sp,#8]`
and lane `d` from `[sp,#12]`, and the proof output now prints
`14.1 14.2 14.3 14.4` for the fixed `hfa14` line.

## Suggested Next

Continue Step 2 on the remaining representative failure after fixed `hfa14`.
The fresh proof still fails with `RUNTIME_NONZERO` / segmentation fault. The
next concrete output facts are long-double HFA publication still printing
`33.1 0.0 0.0` and `34.1 0.0 0.0 ...`, followed by very large corrupted
floating output before the segfault. Localize whether the next owner is
binary128 HFA multi-lane transport, HFA return publication, or variadic HFA
materialization from generated assembly/dumps.

## Watchouts

The proof is not green. Treat this as a repaired `hfa14` lane collision plus
recorded residuals, not as completed `00204.c` support. Do not special-case
`hfa14`, `fa_hfa14`, one lane count, one literal, one emitted register, one
stack slot, or one test case. Do not reopen committed HFA lane-home, F128
transport/address, call ABI register-indexing, binary128 literal, or `fa_hfa13`
scalar lane repairs without fresh evidence proving regression.

Untracked `review/*.md` files were present before this executor packet and were
left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded; `backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, and `backend_aarch64_instruction_dispatch`
passed. `c_testsuite_aarch64_backend_src_00204_c` still failed with
`RUNTIME_NONZERO` / segmentation fault after the fixed `hfa14` line.
`test_after.log` is the fresh proof log for this dirty state.

Also ran `git diff --check`, which passed.
