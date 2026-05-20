Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Representative And Classify Residuals

# Current Packet

## Just Finished

Plan step 4 localized the advanced AArch64 `00204.c` stdarg residual after the
materialized-bool branch repair. The first `stdarg:` payload line remains
advanced past the prior owner and prints all six `ABCDEFGHI` fields. The fresh
first bad fact is the second payload line:

Expected:
`lmnopqr ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI`

Observed:
`lmnopqr ABCDEFGH ABCDEFGH ABCDEFGH ABCDEFGH ABCDEFGH`

The first generated-code divergence is in the caller-side materialization and
publication for the second `stdarg` call, before `myprintf` observes the
arguments. Semantic/prepared BIR for `stdarg` still carries each ninth byte:
for the first `%9s` after `%7s`, `%t26.1.memcpy.global.copy.8` loads `s9+8`
and stores it to `%t26.1`, then `%t27.array.aggregate.load.8` is meant to build
the high 8-byte byval lane. The generated AArch64 assembly instead stores the
byte to a separate temporary stack slot (`strb ... [sp,#8132]`) and then builds
the high lane from the aggregate lane slot that was never populated
(`ldr x9, [sp,#112]; ... str x9, [sp,#4448]`). The same pattern repeats for
the later `%9s` arguments (`[sp,#128]`, `[sp,#144]`, `[sp,#160]`, `[sp,#176]`),
so `myprintf` receives `ABCDEFGH\0...` in each split aggregate.

`myprintf` is not the first owner: its `%9s` va_arg path advances the GP cursor
by 16 bytes, selects the expected register-save/overflow source, copies bytes
0 through 8 into `%lv.t9`, and passes that buffer to `printf("%.9s", ...)`.
The source byte is already absent from the caller-published second lane, so the
visible truncation is not format traversal, va_arg cursor progression,
va_arg copy byte count, or destination string buffering.

Classification: caller-side split aggregate publication/materialization for
small AArch64 byval/variadic aggregate arguments. The narrowest likely repair
surface is the aggregate local-slot to byval lane handoff used by the AArch64
call boundary, especially `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
for partial upper-lane aggregate loads/stores and
`src/backend/mir/aarch64/codegen/calls.cpp` helpers around
`call_arg_byval_aggregate_register_lanes`,
`make_byval_register_lane_prepared_source`, and fragmented aggregate register
lane publication. The prepared call plan for `stdarg` call inst 333 is
credible: the `%7s` consumes `x1`, and each `%9s` is a two-register byval
argument (`x2/x3`, `x4/x5`, `x6/x7`, then stack). The emitted source bytes for
the second lane are wrong before those destinations are loaded.

## Suggested Next

Repair the general AArch64 caller-side split aggregate lane materialization so
a byval aggregate whose final ABI lane is only partially populated still copies
the live tail bytes into the lane consumed by call publication. Use the
`%7s %9s ...` shape as a focused proof, but keep the repair semantic rather
than keyed to `00204.c` or `s9`.

## Watchouts

The Step 2 repair intentionally applies only when the prepared condition value
is stack-backed; ordinary register-backed fused compare branches should continue
through the existing compare-branch path. The focused regression
`backend_aarch64_branch_control_lowering` covers a same-block compare whose
materialized bool register is clobbered before the terminator.

Do not reopen byval rounded-slot placement, outgoing stack argument base,
HFA/floating publication, non-HFA aggregate materialization, post-`va_arg` call
setup, or the `%9s` register-to-overflow decision unless generated-code
evidence moves the first bad fact back there.

The new residual is adjacent to call publication but should not be repaired by
forcing a named `%9s` callsite or by weakening expected output. A credible fix
should preserve the prior guardrails for AArch64 `va_start` overflow cursor
initialization, HFA/floating publication, fixed non-HFA byval rounded slot
placement, sret `x8`, non-HFA aggregate `va_arg`, post-`va_arg` call operand
publication, fixed-formal entry publication, local/value-home publication, and
frame/formal coverage.

## Proof

Focused generated evidence:
`./build/c4cll --target aarch64-linux-gnu --dump-prepared-bir --mir-focus-function myprintf tests/c/external/c-testsuite/src/00204.c > /tmp/00204_myprintf.prepared.txt`
and
`./build/c4cll --target aarch64-linux-gnu --dump-prepared-bir --mir-focus-function stdarg tests/c/external/c-testsuite/src/00204.c > /tmp/00204_stdarg.prepared.txt`.
The generated assembly probe was
`build/c_testsuite_aarch64_backend/src/00204.c.s`.

Smallest credible repair proof command:
`ctest --test-dir build --output-on-failure -R '^(backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1`.

Fresh `test_after.log` shows the prepared-BIR publication test passing and
`c_testsuite_aarch64_backend_src_00204_c` failing at the advanced runtime
mismatch recorded above. No implementation files were edited in this
localization slice.

`git diff --check` passed.
