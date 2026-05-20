Status: Active
Source Idea Path: ideas/open/333_aarch64_opi_pointer_integer_operation_result_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Owner

# Current Packet

## Just Finished

Continued plan Step 2, "Repair The Classified Owner", by repairing the
caller-side call-result/cast publication residual exposed after the scalar ALU
result-home fix.

The prepared scalar cast lowering path in
`src/backend/mir/aarch64/codegen/cast_ops.cpp` now retargets a named cast
operand to an already-emitted scalar source before selecting the cast. For the
`opi` sequence, `%t1 = call i32 addip0(...)` is captured in `x20`; the following
`%t2 = zext i32 %t1 to i64` now reads that emitted call result instead of the
stale prepared source home `x19`.

Repaired generated sequence:

```asm
ldr w13, [sp]
mov w0, w13
bl addip0
mov x20, x0
ubfx x13, x20, #0, #32
mov x0, x13
bl pll
```

The representative now advances beyond `pll(addip0(x))`,
`pll(sublp0(x))`, `pll(addip123(x))`, `pll(addlm123(x))`, and
`pll(sublp4095(x))`. The new first bad fact is
`tests/c/external/c-testsuite/src/00204.c:481`, `pll(subim503808(x))`: expected
`7b3e8`, actual `0`.

## Suggested Next

Localize the new `subim503808` callee-side scalar ALU/immediate-materialization
residual. The current generated callee materializes `503808`, computes
`sub w20, w0, w0`, and returns zero:

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

This is not the caller-side `%t1 -> %t2 -> pll` publication owner repaired in
this packet.

## Watchouts

Do not reopen the repaired MOVI BIR immediate cast fold unless new evidence
shows a remaining MOVI mismatch. Keep HFA/byval/stdarg/fixed-formal/local-
value guardrails and `review/326_stdarg_byval_route_review.md` untouched.

Do not undo the scalar ALU prepared-home repair or this emitted-source cast
repair to chase the new `subim503808` failure. The repaired caller-side path
now uses the captured call result `x20` coherently. The new failure is a
different scalar ALU/immediate owner and is outside this packet's owned
implementation files.

## Proof

Ran the exact delegated proof command:

`cmake --build build --target c4cll backend_aarch64_scalar_alu_records_test backend_aarch64_prepared_scalar_alu_records_test backend_aarch64_scalar_record_contract_test backend_aarch64_return_lowering_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_(scalar_alu_records|prepared_scalar_alu_records|scalar_record_contract|return_lowering)|cli_aarch64_asm_external_return_add_smoke|cli_aarch64_asm_external_return_add_sub_chain_smoke|cli_dump_(bir|prepared_bir)_00204_stdarg)|c_testsuite_aarch64_backend_src_00204_c' > test_after.log 2>&1`

Result: 10/11 selected tests passed. The only remaining failure is
`c_testsuite_aarch64_backend_src_00204_c`, now advanced to `00204.c:481`
(`pll(subim503808(x))`), expected `7b3e8`, actual `0`. `test_after.log`
contains the full proof output.
