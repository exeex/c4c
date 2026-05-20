Status: Active
Source Idea Path: ideas/open/333_aarch64_opi_pointer_integer_operation_result_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The OPI Result Publication Owner

# Current Packet

## Just Finished

Completed plan Step 1, "Localize The OPI Result Publication Owner". The first
generated-code divergence is inside callee `addip0`, before the caller-side
`pll` publication observes the bad value.

Semantic BIR is correct:

`bir.func @addip0(i32 %p.x) -> i32 { %t0 = bir.add i32 %p.x, 0; bir.ret i32 %t0 }`

Prepared AArch64 state is also internally consistent: `%p.x` is in `x0`,
`%t0` has value id `3634`, type `i32`, and prepared result home
`register:x13`; the prepared return move says to publish that same value id
from `x13` to the function return ABI register `x0`
(`placement=gpr:call_result#0/w1 reg=x0`, reason
`return_register_to_register`).

The emitted assembly violates the prepared result-home contract at the scalar
ALU instruction:

```asm
addip0:
    add w0, w0, #0
    mov x0, x13
    ret
```

The `add` computes the source value `1000` in `w0`, but the prepared home for
`%t0` is `w13`. The following `mov x0, x13` matches the prepared return move
and therefore returns stale `x13` instead of the computed result. This
classifies the first owner as AArch64 scalar ALU result-home publication /
return-ABI override in `src/backend/mir/aarch64/codegen/alu.cpp`: the binary
lowering path lets a return-ABI register override the prepared scalar result
home, while the separate prepared before-return move still reads the original
prepared home.

Caller `opi` confirms this downstream:

```asm
movz w9, #1000
str w9, [sp]
ldr w13, [sp]
mov w0, w13
bl addip0
mov x20, x0
ubfx x13, x19, #0, #32
mov x0, x13
bl pll
```

Prepared caller state expects `%t1 = call i32 addip0(...)` in `x20`, `%t2 =
zext i32 %t1 to i64` in `x13`, and the `pll` argument from `x13` to ABI
`x0`. The caller-side `ubfx x13, x19, #0, #32` is a later publication problem
for the same scalar result/cast handoff family, but the first divergence for
the observed `pll(addip0(x))` failure is already in `addip0`'s emitted ALU
destination versus prepared result home.

## Suggested Next

Execute plan Step 2 by repairing AArch64 scalar ALU result publication so a
binary result is emitted into its prepared result home, or so any return-ABI
shortcut replaces the paired prepared return move coherently. Start in
`src/backend/mir/aarch64/codegen/alu.cpp` around the path that builds the
prepared scalar ALU record and overrides `result_register` with
`find_return_abi_register`.

## Watchouts

Do not reopen the repaired MOVI BIR immediate cast fold unless new evidence
shows a remaining MOVI mismatch. Keep HFA/byval/stdarg/fixed-formal/local-
value guardrails and `review/326_stdarg_byval_route_review.md` untouched.

Do not classify this as a standalone return-lowering, call-result capture, or
`pll` call-argument issue. Return/call metadata is present and coherent in the
prepared dumps; the first bad fact is that the scalar ALU instruction fails to
publish `%t0` into its prepared home before the existing return move consumes
that home. Also preserve the caller-side `%t1 -> %t2 -> pll` evidence: after
the callee is fixed, the `opi` cast/call-output sequence may expose a second
same-family stale-register use (`x19` instead of `%t1`/`x20`).

## Proof

Todo-only localization packet. Read-only inspection commands run:

- `build/c4cll --dump-bir --target aarch64-linux-gnu --mir-focus-function addip0 tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --dump-bir --target aarch64-linux-gnu --mir-focus-function opi tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function addip0 --mir-focus-value t0 tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function opi --mir-focus-value t0 tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function opi --mir-focus-value t1 tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function opi --mir-focus-value t2 tests/c/external/c-testsuite/src/00204.c`
- `nl -ba build/c_testsuite_aarch64_backend/src/00204.c.s | sed -n '14208,14216p;14490,14502p'`

No build or CTest was run, and `test_after.log` was not overwritten. Ran
`git diff --check`: passed.
