Status: Active
Source Idea Path: ideas/open/382_aarch64_dynamic_stack_vla_fixed_slot_addressing.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reconfirm Stage and First Bad Address

# Current Packet

## Just Finished

Step 1 reconfirmed `00207` as a generated-program runtime timeout. The
delegated proof rebuilt successfully, then
`c_testsuite_aarch64_backend_src_00207_c` timed out after 5.03s during runtime;
fresh artifacts were emitted first:
`build/c_testsuite_aarch64_backend/src/00207.c.s` at 2026-05-21 23:05:15 UTC
and `build/c_testsuite_aarch64_backend/src/00207.c.bin` at 2026-05-21
23:05:15 UTC.

Bounded direct runtime evidence is in
`build/c_testsuite_aarch64_backend/src/00207.runtime.1s.summary.txt`: a 1s run
exited 124 after producing 47,207,765 lines, sampled as repeated `boom!`. The
source expected output remains only two `boom!` lines followed by `11`, `12`,
`0`, and `1`.

The first fixed-home/moving-`sp` address is `f1`'s `argc` home, prepared as
`%lv.param.argc` slot #1 offset 8. Semantic BIR stores `%p.argc`, performs
`llvm.stacksave`, reloads `%lv.param.argc`, performs `llvm.dynamic_alloca.i8`,
then later reloads and updates `%lv.param.argc` for `argc--`. Prepared BIR
correctly records `f1 has_dynamic_stack=yes fixed_slots_use_fp=yes`, but the
generated AArch64 still emits the `argc` home as `[sp, #8]`: initial
`str w0, [sp, #8]`, pre-VLA `ldr w13, [sp, #8]`, and loop-control
reload/update after `mov sp, x17` at `ldr w13, [sp, #8]` / `str w9, [sp, #8]`.
That loop-control home is the first fixed home responsible for the repeated
output.

## Suggested Next

Execute Step 2 from `plan.md`: add focused backend coverage for a dynamic
stack/VLA function whose fixed scalar local is reloaded and updated after the
dynamic stack pointer moves, expecting the fixed home to use the stable frame
anchor rather than `sp`.

## Watchouts

- Closed idea 381 owns `00200` shift/type-promotion backend asm-generation
  scalability and should stay closed unless fresh evidence invalidates its
  accepted repair.
- Do not edit tests, expectations, unsupported classifications, timeout policy,
  runners, CTest registration, proof-log policy, or c-testsuite sources.
- Do not special-case `00207`, `f1`, `boom!`, one label, one stack offset, one
  register, or one emitted instruction neighborhood.
- Prepared-BIR evidence already says `fixed_slots_use_fp=yes`; the bug appears
  to be in the AArch64 lowering/emission path still materializing slot #1
  through `sp` in generated assembly.
- The generated `f1` assembly also stores the VLA pointer home around
  `[sp]`/`[x29, #16]`, but the localized first loop-control failure is the
  `argc` home at `[sp, #8]`.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00207_c$' > test_after.log 2>&1`

Result: build succeeded; CTest failed by timeout for the single selected test.
`test_after.log` is the canonical proof log and shows 0/1 passed,
`c_testsuite_aarch64_backend_src_00207_c ...***Timeout 5.03 sec`.
Diagnostic artifacts used for localization:
`build/c_testsuite_aarch64_backend/src/00207.dump-bir.txt`,
`build/c_testsuite_aarch64_backend/src/00207.f1.prepared-bir.txt`, and
`build/c_testsuite_aarch64_backend/src/00207.runtime.1s.summary.txt`.
