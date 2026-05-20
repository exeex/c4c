Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the classified singleton HFA aggregate-copy owner. AArch64
aggregate stores now prefer the aggregate alias copy path for aliased one-lane
HFA values instead of treating the aggregate SSA name as the scalar lane. The
singleton path now matches the multi-lane path by loading the explicit source
leaf before storing the destination leaf.

Focused BIR coverage was added for a small AArch64 local aggregate-copy
fixture that copies both `%struct.Hfa1 = type { float }` and `%struct.Hfa2 =
type { float, float }`. The test requires `%dst1.aggregate.copy.0 =
load_local %src1.0`, rejects a direct `%single` store into `%dst1.0`, and
keeps the adjacent two-lane copy checks in the same semantic rule.

The representative `fa_hfa11(hfa11)` first bad fact is gone. Generated BIR
for `arg` now shows `%t20.aggregate.copy.0 = bir.load_local float %t19.0`,
`bir.store_local %t20.0, float %t20.aggregate.copy.0`, `%t22 =
bir.load_local float %t20.0`, then `bir.call void fa_hfa11(float %t22)`.
Generated assembly now reloads `[sp, #784]` into `[sp, #788]` before
publishing `s0`, and runtime output reaches the expected `11.1` line.

## Suggested Next

Execute Step 4-style residual classification for the next `00204.c` first bad
fact exposed after the singleton HFA copy repair. The targeted Step 2 owner is
fixed, but the representative still fails later in `Arguments:` and then in
stdarg/HFA/MOVI output.

Smallest credible proof command to keep using:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer|c_testsuite_aarch64_backend_src_00204_c)$'`.

Focused generated-state probes to keep using:
`./build/c4cll --target aarch64-linux-gnu --dump-bir --mir-focus-function arg tests/c/external/c-testsuite/src/00204.c | sed -n '300,346p'`,
`./build/c4cll --target aarch64-linux-gnu --dump-prepared-bir --mir-focus-function arg --mir-focus-value t22 tests/c/external/c-testsuite/src/00204.c | sed -n '1,90p'`, and
`./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00204.c | sed -n '/bl fa_s17/,+18p'`.

## Watchouts

The remaining `00204.c` first visible mismatch is no longer
`fa_hfa11(hfa11)`: the `Arguments:` HFA scalar lines through `34.1 34.2
34.3 34.4` match, then the expected string line `stu ABC JKL TUV 456 ghi`
prints as `stu ABC I JKL RS TUV`. That residual should be localized as a new
first bad fact before further implementation.

The implementation branch that owned the stale singleton path was in
`src/backend/bir/lir_to_bir/memory/local_slots.cpp`, where aggregate stores
select between HFA lane materialization and alias-based aggregate copy. The
focused coverage lives in `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`.

Do not reopen stdarg cursor/format or `va_start` overflow cursor
initialization without direct generated-code evidence that the first bad fact
moved back to that owner.

Do not reopen HFA/floating return publication, sret `x8`, large byval indirect
pointer transport, byval aggregate register-lane allocation, fragmented byval
lane publication, non-HFA aggregate `va_arg` materialization, fixed-formal
entry publication, local/value-home publication, frame/formal publication, the
scalar separator call-argument repair, or the byval overflow stack publication
repair without direct generated-code evidence that the first bad fact moved
back to that owner.

Do not special-case `00204.c`, `arg`, `fa_hfa11`, `hfa11`, one HFA shape, one
float value, one stack offset, one register, or one emitted instruction
sequence.

## Proof

Proof log: `test_after.log`.

Command run:
`git diff --check && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer|c_testsuite_aarch64_backend_src_00204_c)$'`.

Result: `git diff --check` passed, build passed, and
`backend_lir_to_bir_notes`, `backend_aarch64_instruction_dispatch`, and
`backend_aarch64_machine_printer` passed. `c_testsuite_aarch64_backend_src_00204_c`
still fails with a moved first bad fact after the repaired `fa_hfa11` output.
