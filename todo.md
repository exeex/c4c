Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize HFA/Floating Caller Argument Publication Residual

# Current Packet

## Just Finished

Step 1 localized the current `c_testsuite_aarch64_backend_src_00204_c`
`Arguments:` first bad fact for `fa_hfa11(hfa11)`. Runtime output prints
`0.0`; the source expects `11.1` from `struct hfa11 { float a; } hfa11 =
{ 11.1 };`.

The generated caller still shows the bad publication trace immediately after
`fa_s17`: `ldr s13, [hfa11]`, `str s13, [sp, #784]`, `str s8, [sp, #788]`,
`ldr s13, [sp, #788]`, `fmov s0, s13`, `bl fa_hfa11`. The callee stores and
loads incoming `s0`, then converts it for `printf`, so the first bad fact is
on the caller side before the call.

The earliest localized bad state is semantic BIR for `arg`, before AArch64
call-boundary printing: the one-lane HFA copy lowers as
`bir.store_local %t20.0, float %t19`, then `%t22 = bir.load_local float
%t20.0`, then `bir.call void fa_hfa11(float %t22)`. The adjacent two-lane
HFA case uses explicit lane copy loads, e.g. `%t24.aggregate.copy.0 =
bir.load_local float %t23.0` before storing `%t24.0`. That points at
aggregate/HFA scalar lane copy lowering before call-argument publication,
not at `fa_hfa11` callee consumption or stdarg/`va_start`.

Prepared call metadata for the focused `%t22` value is internally consistent:
`callsite block=0 inst=328 ... callee=fa_hfa11 ... arg0 bank=fpr
from=register:d13 to=s0`, with a before-call move
`destination_kind=call_argument_abi`, `placement=fpr:call_argument#0/w1`,
`reg=s0`, `reason=call_arg_register_to_register`. The stale slot therefore
appears to be created by the bad one-lane HFA aggregate copy feeding `%t22`,
not by AArch64 choosing the wrong destination argument register.

## Suggested Next

Execute Step 2 by repairing the general one-lane HFA/floating aggregate copy
path so a singleton HFA copies from the explicit scalar lane, matching the
existing multi-lane HFA behavior, before the scalar FP call argument is
published.

Smallest credible proof command for the next code slice:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer|c_testsuite_aarch64_backend_src_00204_c)$'`.

Focused generated-state probes to keep using:
`./build/c4cll --target aarch64-linux-gnu --dump-bir --mir-focus-function arg tests/c/external/c-testsuite/src/00204.c | sed -n '300,346p'`,
`./build/c4cll --target aarch64-linux-gnu --dump-prepared-bir --mir-focus-function arg --mir-focus-value t22 tests/c/external/c-testsuite/src/00204.c | sed -n '1,90p'`, and
`./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00204.c | sed -n '/bl fa_s17/,+18p'`.

## Watchouts

Primary owning surfaces for Step 2:
`src/backend/bir/lir_to_bir/aggregate.cpp` local aggregate copy helpers,
`src/backend/bir/lir_to_bir/calling.cpp` HFA/direct-call scalar lane lowering,
and the focused BIR coverage in
`tests/backend/bir/backend_lir_to_bir_notes_test.cpp`. AArch64 prepared
move/codegen consumers are secondary verification surfaces:
`src/backend/prealloc/regalloc/call_moves.cpp`,
`src/backend/prealloc/regalloc/call_return_abi.cpp`,
`src/backend/prealloc/call_plans.cpp`, and
`src/backend/mir/aarch64/codegen/calls.cpp`.

Representative next tests should cover singleton and multi-lane HFA copies
through the same semantic rule, not only `00204.c`. Existing HFA BIR tests
cover scalar FP lane call ABI for two lanes but do not appear to pin the
singleton aggregate-copy source lane that produces `%t20.0`.

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

Localization-only packet; no implementation or test files changed. Commands
used:

- `./build/c_testsuite_aarch64_backend/src/00204.c.bin | sed -n '1,80p'`
- `rg -n "fa_hfa11|hfa11|Arguments:|\\.LC.*11\\.1|bl\\s+fa_hfa11|str\\s+s8|str\\s+s13|ldr\\s+s13|fmov\\s+s0" build/c_testsuite_aarch64_backend/src/00204.c.s`
- `./build/c4cll --target aarch64-linux-gnu --dump-bir --mir-focus-function arg tests/c/external/c-testsuite/src/00204.c | sed -n '300,346p'`
- `./build/c4cll --target aarch64-linux-gnu --dump-prepared-bir --mir-focus-function arg --mir-focus-value t22 tests/c/external/c-testsuite/src/00204.c | sed -n '1,90p'`
- `./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00204.c | sed -n '/bl fa_s17/,+18p'`
- `c4c-clang-tool-ccdb function-signatures` on
  `src/backend/prealloc/regalloc.cpp`,
  `src/backend/prealloc/regalloc/call_return_abi.cpp`,
  `src/backend/prealloc/regalloc/call_moves.cpp`,
  `src/backend/mir/aarch64/codegen/calls.cpp`,
  `src/backend/bir/lir_to_bir/calling.cpp`, and
  `src/backend/bir/lir_to_bir/aggregate.cpp`.

Final proof for this todo-only slice: `git diff --check`.
