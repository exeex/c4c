Status: Active
Source Idea Path: ideas/open/333_aarch64_opi_pointer_integer_operation_result_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Owner

# Current Packet

## Just Finished

Continued plan Step 2, "Repair The Classified Owner", by repairing the
AArch64 scalar shift-left immediate lowering/publication owner localized for
`tests/c/external/c-testsuite/src/00204.c:500`, `pll(lsli1(x))`.

The repair admits immediate `bir.shl` operations into the prepared scalar ALU
record path only when the shift amount is immediate and within the operand
width, and prints a width-correct `lsl` into the prepared result home before
the existing return-ABI publication. Register-amount shifts still fail closed
until `lslv` is modeled.

Repaired `lsli1`/nearby shift-left generated sequence:

```asm
lsli1:
    lsl w13, w0, #1
    mov x0, x13
    mov w0, w13
    ret
lsli31:
    lsl w13, w0, #31
    mov x0, x13
    mov w0, w13
    ret
lsll1:
    lsl x13, x0, #1
    mov x0, x13
    mov x0, x13
    ret
```

The `opi` caller-side publication remains coherent:

```asm
ldr w13, [sp]
mov w0, w13
bl lsli1
mov x20, x0
ubfx x13, x20, #0, #32
mov x0, x13
bl pll
```

The representative now advances to
`tests/c/external/c-testsuite/src/00204.c:504`, `pll(lsri1(x))`: expected
`1f4`, actual `3e8`. BIR/prepared BIR are correct for
`%t0 = bir.lshr i32 %p.x, 1`, but final assembly still returns the unshifted
input:

```asm
lsri1:
    mov x0, x13
    mov w0, w13
    ret
```

## Suggested Next

Localize and repair the direct logical-shift-right immediate path for
`lsri1`/`LShr`, keeping it separate from the completed `Shl` immediate repair.
The likely owner is the same scalar shift immediate publication family, but the
current packet repaired only left shifts.

## Watchouts

Do not reopen the repaired MOVI BIR immediate cast fold unless new evidence
shows a remaining MOVI mismatch. Keep HFA/byval/stdarg/fixed-formal/local-
value guardrails and `review/326_stdarg_byval_route_review.md` untouched.

Do not undo the scalar ALU result-home fix, cast source publication fix, the
rematerializable-immediate clobber fix, or this scalar `Shl` immediate fix to
chase `LShr`. This new first bad fact is separate from MOVI, HFA/floating,
byval, stdarg cursor, fixed-formal, local/value, frame/formal, return lowering,
scalar ALU result-home publication, caller call-result/cast publication,
rematerializable-immediate operand publication/live-source clobbering, and
shift-left immediate publication.

## Proof

Ran the exact delegated proof command:

`cmake --build build --target c4cll backend_aarch64_scalar_alu_records_test backend_aarch64_prepared_scalar_alu_records_test backend_aarch64_scalar_record_contract_test backend_aarch64_return_lowering_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_(scalar_alu_records|prepared_scalar_alu_records|scalar_record_contract|return_lowering)|cli_aarch64_asm_external_return_add_smoke|cli_aarch64_asm_external_return_add_sub_chain_smoke|cli_dump_(bir|prepared_bir)_00204_stdarg)|c_testsuite_aarch64_backend_src_00204_c' > test_after.log 2>&1`

Result: 10/11 selected tests passed. The focused backend tests all pass,
including added scalar `Shl` record/print coverage. The only remaining failure
is `c_testsuite_aarch64_backend_src_00204_c`, now advanced to `00204.c:504`
(`pll(lsri1(x))`), expected `1f4`, actual `3e8`. `test_after.log` contains the
full proof output. `git diff --check` passed.
