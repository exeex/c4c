Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the classified AArch64 HFA/floating owner for the original
multi-lane fixed-HFA mismatch. Fresh generated-code evidence showed `arg()`
already had prepared BIR moves for `fa_hfa13` lanes into `s0`, `s1`, and `s2`,
but the emitted assembly published only `s0`. The concrete owner was AArch64
MIR call-boundary lowering/selection for prepared frame-slot-to-FPR call
argument moves.

`src/backend/mir/aarch64/codegen/calls.cpp` now accepts FPR source registers in
value stack moves, selects prepared frame-slot loads into FPR call ABI
registers, preserves scalar S/D destination bank/view for value moves, and
allows call-boundary frame-slot loads into FPR destinations. The generated
`build/c_testsuite_aarch64_backend/src/00204.c.s` now publishes `fa_hfa13`
argument lanes as `s0`, `s1`, and `s2` before `bl fa_hfa13`.

The next first bad fact in `fa_hfa13` was not ABI lane placement but an
unpublished stack-backed scalar FP width cast: `%t9 = fpext float ... to
double` had a prepared result stack slot, but no emitted `fcvt`/store before
the later `ldr d2, [sp, #16]`. `src/backend/mir/aarch64/codegen/cast_ops.cpp`
now emits a focused scalar F32/F64 width-cast fallback for stack-backed or
prepared-register results when the regular scalar cast record is absent. The
generated `fa_hfa13` path now loads the float lane, emits `fcvt d16, s8`,
stores the double result to the prepared stack slot, and publishes `d2`.

The original visible mismatch starting at `13.1 0.0 -nan` is repaired in the
fresh proof output: the `fa_hfa13` line now prints `13.1 13.2 13.3`.

## Suggested Next

Continue Step 2 on the remaining representative failure after the repaired
`fa_hfa13` owner. The fresh proof still fails with `RUNTIME_NONZERO` /
segmentation fault. The next concrete output facts are:

- fixed `hfa14` still prints `14.1 14.2 13.2 13.3` where the expected line is
  `14.1 14.2 14.3 14.4`
- long-double HFA publication still prints `33.1 0.0 0.0` and
  `34.1 0.0 0.0 ...`, followed by a very large corrupted floating value before
  the segfault

Localize whether the next owner is HFA14 lane source/home collision, binary128
HFA multi-lane transport, or HFA return publication. Do that from generated
assembly/dumps rather than from expectation or runner changes.

## Watchouts

The proof is not green. Treat this as a repaired first mismatch plus recorded
residuals, not as completed `00204.c` support. Do not special-case `hfa13`,
`fa_hfa13`, one lane count, one literal, one emitted register, or one test
case. Do not reopen committed HFA lane-home, F128 transport/address, call ABI
register-indexing, or long-double literal payload repairs without fresh
evidence.

Untracked `review/*.md` files were present before this executor packet and were
left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded; `backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, and `backend_aarch64_instruction_dispatch`
passed. `c_testsuite_aarch64_backend_src_00204_c` still failed with
`RUNTIME_NONZERO` / segmentation fault. `test_after.log` is the fresh proof log
for this dirty state.

Also ran `git diff --check`, which passed.
