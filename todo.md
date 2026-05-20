Status: Active
Source Idea Path: ideas/open/333_aarch64_opi_pointer_integer_operation_result_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Owner

# Current Packet

## Just Finished

Completed a todo-only localization packet for the new Step 2 residual at
`tests/c/external/c-testsuite/src/00204.c:481`,
`pll(subim503808(x))`. The representative first bad fact is now inside callee
`subim503808`, not in caller-side result publication.

Semantic BIR is correct:

```text
bir.func @subim503808(i32 %p.x) -> i32 {
entry:
  %t0 = bir.sub i32 0, 503808
  %t1 = bir.sub i32 %p.x, %t0
  bir.ret i32 %t1
}
```

For `x = 1000`, `%t0` should be `-503808` and `%t1` should be
`1000 - (-503808) = 504808`, printed as `7b3e8`.

Prepared callee facts are coherent:

- `%p.x` value id `3645`, type `i32`, home `register:x0`
- `%t0` value id `3646`, type `i32`, home `rematerializable_immediate
  imm_i32=-503808`
- `%t1` value id `3647`, type `i32`, result home `register:x21`
- before-instruction moves for instruction index 1 publish both `%p.x` and
  `%t0` as consumer inputs for `%t1`
- before-return move publishes `%t1` from `x21` to return register `x0`

The emitted callee violates that contract before the caller observes the value:

```asm
subim503808:
    mov w0, #0
    movz w9, #45056
    movk w9, #7, lsl #16
    sub w0, w0, w9
    sub w20, w0, w0
    mov x0, x21
    mov w0, w20
    ret
```

The immediate-materialization sequence computes `%t0 = -503808` into `w0`,
which is also the live `%p.x`/formal input register. The following `%t1`
subtract then reads `w0` for both operands (`sub w20, w0, w0`), producing zero.
This classifies the owner as AArch64 scalar ALU rematerializable-immediate
operand publication / live-source clobbering in the callee. It is distinct from
the already repaired scalar ALU result-home/return-ABI override and the
caller-side cast source publication repair.

Caller `opi` is now coherent for this path: `%t15` is loaded into `x13`, passed
to `subim503808` via `x0`, `%t16` is captured from return `x0` into `x20`,
`%t17 = zext i32 %t16 to i64` reads `x20` into `x13`, and the `pll` argument
is published from `x13` to `x0`:

```asm
ldr w13, [sp]
mov w0, w13
bl subim503808
mov x20, x0
ubfx x13, x20, #0, #32
mov x0, x13
bl pll
```

## Suggested Next

Repair the scalar ALU/immediate owner so rematerializing a prepared immediate
operand for a binary instruction does not clobber a live source register needed
by the same instruction. The fix should make `%t1 = %p.x - %t0` use `%p.x`
from the original `1000` source and `%t0` from a separate immediate/materialized
source, then publish the result into prepared home `x21` before the existing
return move.

## Watchouts

Do not reopen the repaired MOVI BIR immediate cast fold unless new evidence
shows a remaining MOVI mismatch. Keep HFA/byval/stdarg/fixed-formal/local-
value guardrails and `review/326_stdarg_byval_route_review.md` untouched.

Do not classify this as MOVI, HFA/floating, byval, stdarg cursor,
fixed-formal, local/value, frame/formal, return lowering, or caller
call-result/cast publication. The caller-side path is coherent after the prior
repair. The first bad fact is the callee scalar ALU materialization of `%t0`
into `w0`, which clobbers `%p.x` before `%t1` is computed.

## Proof

Todo-only localization packet. Read-only inspection commands run:

- `nl -ba tests/c/external/c-testsuite/src/00204.c | sed -n '430,485p'`
- `nl -ba build/c_testsuite_aarch64_backend/src/00204.c.s | sed -n '14248,14262p;14545,14555p'`
- `build/c4cll --dump-bir --target aarch64-linux-gnu --mir-focus-function subim503808 tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function subim503808 --mir-focus-value t0 tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function subim503808 --mir-focus-value t1 tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function opi --mir-focus-value t15 tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function opi --mir-focus-value t16 tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function opi --mir-focus-value t17 tests/c/external/c-testsuite/src/00204.c`
- `sed -n '300,320p;650,665p' test_before.log`

No build, broad test, or `test_after.log` overwrite was performed. Baseline
`test_before.log` records the current representative mismatch: expected
`7b3e8`, actual `0`.
