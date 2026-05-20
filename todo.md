Status: Active
Source Idea Path: ideas/open/333_aarch64_opi_pointer_integer_operation_result_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Owner

# Current Packet

## Just Finished

Continued plan Step 2, "Repair The Classified Owner", by repairing the
AArch64 direct arithmetic-shift-right immediate lowering/publication owner
localized for `tests/c/external/c-testsuite/src/00204.c:508`,
`pll(asri1(x))`.

The repair extends the prepared scalar shift immediate path to admit
`bir.ashr` when the shift amount is immediate and within the operand width, and
prints a width-correct `asr` into the prepared result home before the existing
return-ABI publication. Register-amount shifts still fail closed until
variable shifts are modeled.

Repaired `asri1`/nearby arithmetic-shift-right generated sequence:

```asm
asri1:
    asr w13, w0, #1
    mov x0, x13
    mov w0, w13
    ret
asri31:
    asr w13, w0, #31
    mov x0, x13
    mov w0, w13
    ret
asrl1:
    asr x13, x0, #1
    mov x0, x13
    mov x0, x13
    ret
```

The `opi` caller-side publication remains coherent:

```asm
ldr w13, [sp]
mov w0, w13
bl asri1
mov x20, x0
sxtw x13, w20
mov x0, x13
bl pll
```

The representative `c_testsuite_aarch64_backend_src_00204_c` now passes in the
delegated focused subset.

## Suggested Next

Hand control back to the supervisor/plan-owner for lifecycle review of the
active OPI result-publication plan, because the delegated representative is now
green in the selected backend subset.

## Watchouts

Do not reopen the repaired MOVI BIR immediate cast fold unless new evidence
shows a remaining MOVI mismatch. Keep HFA/byval/stdarg/fixed-formal/local-
value guardrails and `review/326_stdarg_byval_route_review.md` untouched.

Do not undo the scalar ALU result-home fix, cast source publication fix, the
rematerializable-immediate clobber fix, or the scalar `Shl`/`LShr`/`AShr`
immediate fixes. They are separate from MOVI, HFA/floating, byval, stdarg
cursor, fixed-formal, local/value, frame/formal, return lowering, scalar ALU
result-home publication, caller call-result/cast publication, and
rematerializable-immediate operand publication/live-source clobbering.

## Proof

Ran the exact delegated proof command:

`cmake --build build --target c4cll backend_aarch64_scalar_alu_records_test backend_aarch64_prepared_scalar_alu_records_test backend_aarch64_scalar_record_contract_test backend_aarch64_return_lowering_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_(scalar_alu_records|prepared_scalar_alu_records|scalar_record_contract|return_lowering)|cli_aarch64_asm_external_return_add_smoke|cli_aarch64_asm_external_return_add_sub_chain_smoke|cli_dump_(bir|prepared_bir)_00204_stdarg)|c_testsuite_aarch64_backend_src_00204_c' > test_after.log 2>&1`

Result: 10/11 selected tests passed. The focused backend tests all pass,
including updated scalar shift record/print coverage for `Shl`, `LShr`, and
`AShr`. The representative `c_testsuite_aarch64_backend_src_00204_c` now
passes, so the selected proof is 11/11 green. `test_after.log` contains the
full proof output. `git diff --check` passed.
