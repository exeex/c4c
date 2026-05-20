Status: Active
Source Idea Path: ideas/open/333_aarch64_opi_pointer_integer_operation_result_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Owner

# Current Packet

## Just Finished

Completed plan Step 2, "Repair The Classified Owner". The AArch64 scalar ALU
prepared-record path in `src/backend/mir/aarch64/codegen/alu.cpp` no longer
retargets a prepared binary result home to the return ABI register. Prepared
scalar ALU records now keep the prepared result home, and the existing
before-return move remains responsible for publishing the return ABI value.

The repaired `addip0` callee now matches the prepared contract: source value
`%p.x` enters in `w0`, prepared `%t0`/value id `3634` is computed into the
prepared result home `w13`, the prepared return move publishes `x13 -> x0`, and
the fallback return-ABI shortcut remains available only when no prepared scalar
record was produced.

```asm
addip0:
    add w13, w0, #0
    mov x0, x13
    mov w0, w13
    ret
```

Focused coverage was updated so the AArch64 external return-add smoke exercises
a helper return instead of only a constant-folded `main`:

```asm
add_three:
    add w13, w0, #3
    mov x0, x13
    ret
```

`backend_aarch64_return_lowering_test` now asserts the same contract for an
`i32` register-backed scalar result: the scalar record computes into `w19`, and
the return record reads the prepared `w19` source instead of expecting the ALU
node itself to own ABI `w0`.

The representative still fails at `tests/c/external/c-testsuite/src/00204.c:476`
(`pll(addip0(x))`), but the first generated-code owner has advanced from the
callee ALU destination to caller-side call-result/cast publication. The repaired
callee returns through `x0`, then `opi` captures that result into `%t1`/`x20`,
but the zext result `%t2` is still published from stale `x19` instead of the
call result:

```asm
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
`x0`. Runtime output now prints `c61bed1a` for that first OPI line, expected
`3e8`, because `ubfx x13, x19, #0, #32` reads stale `x19` after the repaired
callee return.

## Suggested Next

Continue with the caller-side `%t1 -> %t2 -> pll` publication owner exposed by
the repaired callee. This is not a MOVI, HFA/floating, byval, stdarg cursor,
fixed-formal, local/value, or frame/formal issue. It is a call-result/cast
handoff problem in generated `opi`: the call result is captured in `x20`, but
the following zext/publication reads `x19`.

## Watchouts

Do not reopen the repaired MOVI BIR immediate cast fold unless new evidence
shows a remaining MOVI mismatch. Keep HFA/byval/stdarg/fixed-formal/local-
value guardrails and `review/326_stdarg_byval_route_review.md` untouched.

Do not undo the scalar ALU prepared-home repair to make the representative
appear to move differently. The current callee sequence is coherent with the
prepared result home and return move. The remaining owner is the caller-side
call-result/cast publication sequence after `bl addip0`.

## Proof

Ran the exact delegated proof command:

`cmake --build build --target c4cll backend_aarch64_scalar_alu_records_test backend_aarch64_prepared_scalar_alu_records_test backend_aarch64_scalar_record_contract_test backend_aarch64_return_lowering_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_(scalar_alu_records|prepared_scalar_alu_records|scalar_record_contract|return_lowering)|cli_aarch64_asm_external_return_add_smoke|cli_aarch64_asm_external_return_add_sub_chain_smoke|cli_dump_(bir|prepared_bir)_00204_stdarg)|c_testsuite_aarch64_backend_src_00204_c' > test_after.log 2>&1`

Result: 10/11 selected tests passed. The only remaining failure is
`c_testsuite_aarch64_backend_src_00204_c`, with the first current mismatch at
`00204.c:476` from caller-side `ubfx x13, x19, #0, #32` after `bl addip0`.
`test_after.log` contains the full proof output.
