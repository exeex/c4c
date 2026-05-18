# AArch64 Scalar Parameter ALU Authority Todo

Status: Active
Source Idea Path: ideas/open/290_aarch64_scalar_parameter_alu_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Scalar Parameter ALU Failure

# Current Packet

## Just Finished

Executed Step 1, "Establish Scalar Parameter ALU Failure", for
`tests/c/external/c-testsuite/src/00124.c` without implementation changes.

The failure is localized to scalar parameter/ALU operand authority in the
called function `f2`, not to function-pointer value/callee handling.

Semantic BIR for `f2` is correct:

```text
bir.func @f2(i32 %p.c, i32 %p.b) -> i32 {
entry:
  %t0 = bir.sub i32 %p.c, %p.b
  bir.ret i32 %t0
}
```

Prepared homes and storage for `f2` also name the incoming ABI parameter
registers and the scalar result register:

```text
home %p.c value_id=0 kind=register reg=x0
home %p.b value_id=1 kind=register reg=x1
home %t0 value_id=2 kind=register reg=x13
storage %p.c value_id=0 encoding=register bank=gpr reg=x0 width=1 units=x0
storage %p.b value_id=1 encoding=register bank=gpr reg=x1 width=1 units=x1
storage %t0 value_id=2 encoding=register bank=gpr placement=gpr:caller_saved#0/w1 reg=x13 width=1 units=x13
```

Generated AArch64 for `f2` ignores those prepared parameter homes and emits the
subtract from stale callee-saved registers:

```asm
f2:
    sub w0, w19, w20
    ret
```

The function-pointer return and indirect-call path remains valid:

```asm
f1:
    adrp x0, f2
    add x0, x0, :lo12:f2
    ret
main:
    adrp x20, f1
    add x20, x20, :lo12:f1
    mov w0, #0
    mov w1, #2
    blr x20
    mov x13, x0
    mov w0, #2
    mov w1, #2
    blr x13
```

Likely backend surfaces for the repair are:

- `src/backend/prealloc/regalloc/value_homes.cpp` for parameter home authority
  from ABI registers.
- `src/backend/prealloc/regalloc/storage.cpp` and
  `src/backend/prealloc/storage_plans.cpp` for parameter storage records.
- `src/backend/prealloc/regalloc/consumer_moves.cpp` for the
  `before_instruction` consumer moves that currently mention `%p.c` and `%p.b`
  feeding `%t0`.
- `src/backend/mir/aarch64/codegen/alu.cpp` for prepared scalar ALU operand
  selection.
- `src/backend/mir/aarch64/codegen/operands.cpp` and
  `src/backend/mir/aarch64/codegen/instruction.cpp` for typed prepared
  register operands and fallback behavior.

## Suggested Next

Implement a focused AArch64 scalar ALU authority repair so prepared parameter
homes/storage for `%p.c` and `%p.b` drive the emitted `sub` operands. The
expected `f2` shape is `sub w0, w0, w1` or an equivalent sequence that consumes
incoming `w0`/`w1` before returning the scalar result.

## Watchouts

- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Do not reopen the closed function-pointer materialization owner unless new
  proof contradicts the closure review.
- Preserve the existing valid function-pointer return and indirect `blr`
  shape.
- Reject named-case shortcuts for `src/00124.c`, one function name, one
  parameter name, or one arithmetic expression.
- The emitted `sub w0, w19, w20` conflicts with prepared facts that name
  `%p.c` as `x0` and `%p.b` as `x1`.
- Keep the `00210` call-argument/register-authority blocker separate.

## Proof

Baseline proof was already captured in `test_before.log` with:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00124_c$'; } > test_before.log 2>&1
```

Result: `c_testsuite_aarch64_backend_src_00124_c` failed
`[RUNTIME_NONZERO] exit=216`.

No new proof rerun was needed for this localization packet, so `test_after.log`
was not rewritten.

Inspection commands used:

```sh
build-aarch64-scan/c4cll --target aarch64-linux-gnu --dump-bir tests/c/external/c-testsuite/src/00124.c > /tmp/00124.scalar.bir
build-aarch64-scan/c4cll --target aarch64-linux-gnu --dump-prepared-bir tests/c/external/c-testsuite/src/00124.c > /tmp/00124.scalar.prepared
build-aarch64-scan/c4cll --target aarch64-linux-gnu --dump-mir --mir-focus-function f2 tests/c/external/c-testsuite/src/00124.c > /tmp/00124.scalar.f2.mir
build-aarch64-scan/c4cll --target aarch64-linux-gnu --trace-mir --mir-focus-function f2 tests/c/external/c-testsuite/src/00124.c > /tmp/00124.scalar.f2.trace
```
