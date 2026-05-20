Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Representative And Classify Residuals

# Current Packet

## Just Finished

Step 4 classified the new `00204.c` first visible mismatch after the singleton
HFA aggregate-copy repair. The repaired HFA/floating argument block now reaches
the expected scalar lines through `34.1 34.2 34.3 34.4`. The next first bad
fact is in the following fixed aggregate call:

- expected: `stu ABC JKL TUV 456 ghi`
- observed: `stu ABC I JKL RS TUV`

The observing source is `fa1(struct s8 a, struct s9 b, struct s10 c, struct
s11 d, struct s12 e, struct s13 f)`, which prints the first three bytes of
six non-HFA struct arguments. Generated BIR for `arg` correctly loads the
global bytes for `%t103` through `%t108` from `s8` through `s13` and calls
`fa1(ptr byval(size=8) %t103, ptr byval(size=9) %t104, ptr byval(size=10)
%t105, ptr byval(size=11) %t106, ptr byval(size=12) %t107, ptr
byval(size=13) %t108)`.

Prepared/generated-code evidence places the mismatch after semantic byte
materialization, in AArch64 byval aggregate argument placement and entry
unpack:

- prepared `arg` reports `callsite ... callee=fa1 ... args=1` for focus value
  `%t103`, with only `arg0 ... to=x0` shown for the byval aggregate-register
  lane move.
- generated assembly for the `fa1` call publishes the contiguous aggregate
  chunks into `x0` through `x6`: `x0` has `s8`, `x1` has the first eight bytes
  of `s9`, `x2` has the trailing `I`, `x3` has `s10`, `x4` has the trailing
  `RS`, `x5` has `s11`, and `x6` has the trailing `YZ0`.
- generated `fa1` entry then materializes `%p.a` from `x0`, `%p.b` from `x1`,
  `%p.c` from `x2`, `%p.d` from `x3`, `%p.e` from `x4`, and `%p.f` from
  `x5`, which explains the observed `stu ABC I JKL RS TUV` line.

This residual does not remain under idea 326. It is a fixed non-HFA byval
aggregate register-lane / stack-transition ABI issue, not an HFA/floating
argument publication issue. It should be handed to plan-owner lifecycle
routing for a new or resumed non-HFA byval aggregate argument placement route.
Likely owning surfaces are `src/backend/prealloc/call_plans.cpp`,
`src/backend/prealloc/regalloc.cpp`, and
`src/backend/mir/aarch64/codegen/calls.cpp` for call-argument byval lane
planning/publication, plus `src/backend/mir/aarch64/codegen/dispatch.cpp` for
entry formal byval aggregate unpack.

## Suggested Next

Ask the plan owner to route the non-HFA byval aggregate residual out of idea
326. The next executable packet should localize AAPCS64 fixed aggregate
arguments across the caller and callee boundary for a small `s8, s9, s10, ...`
shape, including the register-to-stack transition when too few GPR argument
registers remain for the full aggregate.

Smallest next proof command after lifecycle routing:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer|c_testsuite_aarch64_backend_src_00204_c)$'`.

Focused generated-state probes to reuse:
`./build/c4cll --target aarch64-linux-gnu --dump-bir --mir-focus-function arg tests/c/external/c-testsuite/src/00204.c | sed -n '560,630p'`,
`./build/c4cll --target aarch64-linux-gnu --dump-prepared-bir --mir-focus-function arg --mir-focus-value t103 tests/c/external/c-testsuite/src/00204.c | sed -n '1,220p'`,
`./build/c4cll --target aarch64-linux-gnu --dump-prepared-bir --mir-focus-function fa1 tests/c/external/c-testsuite/src/00204.c | sed -n '1,220p'`, and
`./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00204.c | sed -n '/^arg:/,/^fr_s1:/p' | rg -n -C 35 'bl fa1|s8|s9|s10|s11|s12|s13|ldr x[0-7]|str x[0-7]|mov x[0-7]'`.

## Watchouts

Do not continue this residual under the HFA/floating idea without plan-owner
approval. The new bad line involves non-HFA `char[]` structs and matches a
fixed aggregate byval call-boundary chunking problem.

Do not repair this with one function, one string literal, one register, one
stack offset, or one emitted instruction sequence. The real contract is the
general AAPCS64 rule for small non-HFA aggregates: each aggregate consumes its
own rounded register/stack slots, and an aggregate that cannot fit in the
remaining GPR argument registers must transition coherently to stack passing
for both caller publication and callee entry materialization.

The previous singleton HFA owner remains fixed; do not reopen HFA/floating
return publication, stdarg cursor/format, `va_start` overflow cursor
initialization, non-HFA aggregate `va_arg` materialization, fixed-formal entry
publication, local/value-home publication, or frame/formal publication unless
fresh generated evidence moves the first bad fact back to one of those owners.

## Proof

Proof log: `test_after.log`.

Commands/probes run:
`rg -n 'stu ABC|Arguments:|34\.1|c_testsuite_aarch64_backend_src_00204_c|Expected|Actual|FAILED' test_after.log`;
`./build/c4cll --target aarch64-linux-gnu --dump-bir --mir-focus-function arg tests/c/external/c-testsuite/src/00204.c | sed -n '560,630p'`;
`./build/c4cll --target aarch64-linux-gnu --dump-prepared-bir --mir-focus-function arg --mir-focus-value t103 tests/c/external/c-testsuite/src/00204.c | sed -n '1,220p'`;
`./build/c4cll --target aarch64-linux-gnu --dump-prepared-bir --mir-focus-function fa1 --mir-focus-value lv.param.p.c tests/c/external/c-testsuite/src/00204.c | sed -n '1,220p'`;
`./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00204.c | sed -n '/^arg:/,/^fr_s1:/p' | rg -n -C 35 'bl fa1|s8|s9|s10|s11|s12|s13|ldr x[0-7]|str x[0-7]|mov x[0-7]'`.

Result: classification complete from existing `test_after.log` plus generated
BIR/prepared-BIR/assembly probes. No full build was required for this todo-only
classification. `test_after.log` remains the runtime proof log showing
`c_testsuite_aarch64_backend_src_00204_c` still fails with the moved first bad
fact after repaired HFA scalar output. `git diff --check` passed after the
`todo.md` update.
