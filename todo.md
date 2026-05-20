Status: Active
Source Idea Path: ideas/open/333_aarch64_opi_pointer_integer_operation_result_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Owner

# Current Packet

## Just Finished

Completed the todo-only localization packet for
`tests/c/external/c-testsuite/src/00204.c:500`, `pll(lsli1(x))`. The first
generated-code divergence is in the callee-side scalar shift operation
lowering/publication owner: semantic BIR and prepared homes describe
`%t0 = %p.x << 1`, but final AArch64 emission never emits the shift into the
prepared result home before publishing the return value.

Source value and prepared facts:

- Source input is `x = 1000` (`0x3e8`), passed to `lsli1` as `%p.x`.
- BIR is correct:
  `%t0 = bir.shl i32 %p.x, 1`; `bir.ret i32 %t0`.
- Prepared type/width is `i32` for `%p.x` and `%t0`.
- Prepared source home: `%p.x` value id `3689`, register `x0`.
- Shift operand/source: immediate shift amount `1` from the BIR RHS.
- Prepared result home: `%t0` value id `3690`, register `x13`.
- Prepared before-instruction move copies `%p.x` to `%t0` for the consumer
  register path, and prepared before-return move publishes `%t0` to return ABI
  register `x0`.

The emitted `lsli1` sequence is:

```asm
lsli1:
    mov x0, x13
    mov w0, w13
    ret
```

There is no `lsl`/shift instruction, so the callee returns unshifted `0x3e8`
instead of expected `0x7d0`.

Caller publication is coherent and is not the owner. In `opi`, `%t72` is loaded
from `x` into `x13`, the call argument move publishes it to `x0`, `%t73` is
captured from call result `x0` into `x20`, the zext path reads `%t73` from
`x20`, and `pll` receives the caller-published operand through `x0`. The
caller-side sequence around `lsli1` is:

```asm
ldr w13, [sp]
mov w0, w13
bl lsli1
mov x20, x0
ubfx x13, x20, #0, #32
mov x0, x13
bl pll
```

## Suggested Next

Repair the scalar shift/SHL lowering path so `bir.shl i32 %p.x, 1` emits a
coherent shift into the prepared result home before the return-ABI publication
for `lsli1`, for example a width-correct `lsl` from `%p.x` to `%t0` followed by
the existing return move.

## Watchouts

Do not reopen the repaired MOVI BIR immediate cast fold unless new evidence
shows a remaining MOVI mismatch. Keep HFA/byval/stdarg/fixed-formal/local-
value guardrails and `review/326_stdarg_byval_route_review.md` untouched.

Do not undo the scalar ALU result-home fix, cast source publication fix, or the
rematerializable-immediate clobber fix to chase the shift failure. This owner is
separate from MOVI, HFA/floating, byval, stdarg cursor, fixed-formal,
local/value, frame/formal, return lowering, scalar ALU result-home publication,
caller call-result/cast publication, and rematerializable-immediate operand
publication/live-source clobbering.

## Proof

Read-only localization commands used:

- `build/c4cll --dump-bir --target aarch64-linux-gnu --mir-focus-function lsli1 tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function lsli1 tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function opi --mir-focus-value t72 tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function opi --mir-focus-value t73 tests/c/external/c-testsuite/src/00204.c`
- `nl -ba build/c_testsuite_aarch64_backend/src/00204.c.s | sed -n '14318,14335p;14680,14698p'`
- `nl -ba build/c_testsuite_aarch64_backend/src/00204.c.s | sed -n '14422,14432p'`

No broad test or build was run, and `test_after.log` was not overwritten for
this todo-only packet. `test_before.log` remains the baseline showing
`c_testsuite_aarch64_backend_src_00204_c` failing at `lsli1`, expected `7d0`,
actual `3e8`. `git diff --check` passed.
