Status: Active
Source Idea Path: ideas/open/333_aarch64_opi_pointer_integer_operation_result_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Owner

# Current Packet

## Just Finished

Continued plan Step 2, "Repair The Classified Owner", by repairing the
`subim503808` scalar ALU rematerializable-immediate operand publication /
live-source clobbering owner.

The bug was that `%t0 = bir.sub i32 0, 503808` had a rematerializable immediate
home, but fallback return-chain lowering still materialized it through `w0`,
which was also the live `%p.x` formal register for the next instruction. Then
`%t1 = %p.x - %t0` read `w0` for both operands and returned zero.

The repair keeps rematerializable-immediate homes authoritative when retargeting
prepared scalar ALU operands, and narrows fallback scratch use to the real
clobber case: when a rematerializable result would otherwise materialize into a
register needed by its immediate consumer. Harmless return-ABI accumulator
chains still use `w0`.

Repaired `subim503808` generated sequence:

```asm
subim503808:
    mov w9, #0
    movz w10, #45056
    movk w10, #7, lsl #16
    sub w9, w9, w10
    movz w9, #20480
    movk w9, #65528, lsl #16
    sub w20, w0, w9
    mov x0, x21
    mov w0, w20
    ret
```

This preserves `%p.x` in `w0`, materializes `%t0` separately, and computes
`1000 - (-503808) = 504808`, so `pll(subim503808(x))` now prints `7b3e8`.

The representative now advances to
`tests/c/external/c-testsuite/src/00204.c:500`, `pll(lsli1(x))`: expected
`7d0`, actual `3e8`. The generated callee currently returns the input without
emitting the left shift:

```asm
lsli1:
    mov x0, x13
    mov w0, w13
    ret
```

## Suggested Next

Localize the new shift residual for `lsli1`. Start from generated
BIR/prepared/assembly for `lsli1` and caller `opi`; determine why `x << 1`
returns the unshifted input even though caller-side call-result/cast
publication is coherent.

## Watchouts

Do not reopen the repaired MOVI BIR immediate cast fold unless new evidence
shows a remaining MOVI mismatch. Keep HFA/byval/stdarg/fixed-formal/local-
value guardrails and `review/326_stdarg_byval_route_review.md` untouched.

Do not undo the scalar ALU result-home fix, cast source publication fix, or this
rematerializable-immediate clobber fix to chase the new shift failure. The new
first bad fact is separate from MOVI, HFA/floating, byval, stdarg cursor,
fixed-formal, local/value, frame/formal, return lowering, immediate
rematerialization, and caller call-result/cast publication.

## Proof

Ran the exact delegated proof command:

`cmake --build build --target c4cll backend_aarch64_scalar_alu_records_test backend_aarch64_prepared_scalar_alu_records_test backend_aarch64_scalar_record_contract_test backend_aarch64_return_lowering_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_(scalar_alu_records|prepared_scalar_alu_records|scalar_record_contract|return_lowering)|cli_aarch64_asm_external_return_add_smoke|cli_aarch64_asm_external_return_add_sub_chain_smoke|cli_dump_(bir|prepared_bir)_00204_stdarg)|c_testsuite_aarch64_backend_src_00204_c' > test_after.log 2>&1`

Result: 10/11 selected tests passed. The focused backend tests all pass. The
only remaining failure is `c_testsuite_aarch64_backend_src_00204_c`, now
advanced to `00204.c:500` (`pll(lsli1(x))`), expected `7d0`, actual `3e8`.
`test_after.log` contains the full proof output.
