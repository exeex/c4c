# RV64 Variadic Target LLVM-Route Facts

## Goal

Teach the target-aware LLVM-route/LIR path to produce RV64-correct variadic ABI
facts for the two minimal evidence cases from
`build/variadic_abi_triage/`: direct external variadic integer calls and scalar
`stdarg` pointer-cursor lowering.

The first repair boundary is before semantic BIR, prepared ABI publication, and
RV64 final emission. Later stages should consume these facts; they should not
invent or guess them after LLVM-route output has already flattened the target
ABI.

## Why This Exists

`ideas/open/325_variadic_target_ir_abi_boundary_triage.md` found that clang's
RV64 LLVM IR diverges from AArch64 before prepared BIR:

- Direct external variadic calls need RV64 integer extension facts, including
  `signext` on the `i32` return and integer variadic operand.
- Scalar `stdarg` uses an RV64 pointer-cursor `va_list`: load the cursor,
  advance by an 8-byte slot, store the advanced cursor, then load the scalar
  value from the original cursor.
- c4c currently emits target-flattened direct-call LLVM IR and AArch64-shaped
  structured `va_list` lowering for RV64.

Prepared BIR already records target register placement for the direct-call
case, but it does not carry the missing RV64 extension facts. The scalar
`stdarg` case also fails before semantic/prepared BIR can publish useful
target-specific callee-entry facts. That makes target-aware LLVM-route/LIR fact
production the narrow first implementation boundary.

## In Scope

- Preserve or emit RV64 `signext` facts for direct external variadic integer
  calls in LLVM-route output, including the `printf` declaration/call return
  and the integer variadic operand.
- Model scalar RV64 `va_list` as the clang-observed pointer-cursor layout in
  LLVM-route output.
- Lower scalar `va_arg(ap, int)` for RV64 as cursor load, 8-byte cursor
  advance, cursor store, and `i32` load from the original cursor.
- Keep the existing AArch64 structured `%struct.__va_list` route intact.
- Use the tiny triage fixtures as the first proof surface before deciding
  whether semantic BIR admission or prepared ABI records need a separate idea.
- Record any unavailable LIR dump surface honestly and prove through the
  current `--codegen llvm` route when no lower-level dump exists.

## Out Of Scope

- Full RV64 variadic runtime support.
- RV64 prepared ABI records for variadic callee entry, `va_arg`, or direct
  external variadic calls beyond consuming already-correct target facts.
- RV64 final target emitter support for variadic calls or `stdarg`.
- Semantic BIR local-memory admission fixes for the scalar `stdarg` fixture.
- Broad c-testsuite registration for variadic cases.
- Fake libc stubs, special `printf` bodies, or c-testsuite symbol shortcuts.
- Copying AArch64 `va_list`, register-save-area, offset, or HFA constants into
  RV64.
- Weakening existing unsupported diagnostics for backend routes that still lack
  target-correct facts.

## Implementation Boundary

The owned boundary is target-aware LLVM-route/LIR production for RV64 variadic
ABI facts. The implementation should make the RV64 target route choose and
preserve the right facts before semantic BIR or prepared BIR runs:

- Direct external variadic call ABI: RV64 integer return and variadic integer
  operands carry extension attributes where clang requires them.
- Variadic callee entry setup: RV64 `va_start` initializes a pointer cursor
  representation instead of the AArch64 structured `va_list`.
- `va_list` layout: RV64 uses a single pointer cursor for this scalar case;
  AArch64 remains a structured record with target-specific fields.
- `va_arg` extraction: RV64 scalar integer extraction advances by the ABI slot
  and loads the scalar from the original cursor.

If the current code has no explicit reusable LIR fact object for one of these
items, introduce the smallest target-owned fact needed by the LLVM-route output.
Do not route the repair through prepared ABI or final emission as a substitute
for missing target LLVM-route facts.

## Proof Strategy

Use fixture-level checks first:

```sh
./build/c4cll --codegen llvm --target riscv64-linux-gnu build/variadic_abi_triage/direct_variadic_printf.c -o build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll
./build/c4cll --codegen llvm --target riscv64-linux-gnu build/variadic_abi_triage/stdarg_scalar_int.c -o build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll
rg "declare signext i32 @printf|call signext i32 .*@printf|i32 noundef signext 7" build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll
rg "llvm.va_start|load ptr|store ptr|getelementptr inbounds i8, ptr .*, i64 8|load i32" build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll
```

Also run a matching AArch64 fixture check that proves the structured
`%struct.__va_list` path remains AArch64-only and has not been generalized into
RV64. If the spelling of generated IR differs from clang while preserving the
same required ABI facts, document the equivalence in the proof log rather than
weakening the contract.

## Acceptance Criteria

- RV64 direct external variadic integer-call LLVM-route output carries the
  extension facts clang shows for the minimal `printf` case.
- RV64 scalar `stdarg` LLVM-route output no longer uses the AArch64 structured
  `%struct.__va_list` lowering and instead exposes pointer-cursor behavior.
- AArch64 still uses its structured `va_list` route for the same fixture.
- The implementation boundary stays before semantic BIR, prepared ABI, and
  final target emission unless the proof shows the target LLVM-route facts are
  already correct and only need a named carrier.
- Existing RV64 unsupported backend diagnostics are not weakened until a later
  implementation idea proves real backend support.

## Reviewer Reject Signals

- The slice copies AArch64 `va_list`, register-save-area, offset, slot, HFA, or
  save-area constants into RV64 instead of modeling the RV64 pointer-cursor
  facts from target evidence.
- The direct-call repair is a testcase-shaped `printf` shortcut, named-symbol
  special case, fake libc body, or c-testsuite-only path rather than a general
  direct external variadic integer ABI rule.
- The route weakens unsupported diagnostics, reclassifies tests as unsupported,
  or loosens expected contracts without explicit user approval.
- The implementation claims prepared ABI, prealloc, or RV64 emitter progress
  before target-correct RV64 LLVM-route facts exist for both direct-call
  `signext` and scalar pointer-cursor `stdarg`.
- The change only renames helpers, rewrites expectations, or changes
  classification text while retaining target-flattened RV64 LLVM-route output.
- The slice makes AArch64 and RV64 share one variadic IR shape after the triage
  proved the target shapes diverge.
- The proof only demonstrates the exact `printf("%d\n", 7)` spelling while
  nearby direct external variadic integer calls would still bypass the new
  target-aware rule.

## Closure Note

Closed on 2026-06-23 after the active runbook completed the owned
LLVM-route/LIR boundary:

- RV64 direct external variadic integer-call LLVM output carries the required
  integer extension facts, including signed and unsigned return attributes and
  `signext` on variadic integer operands.
- RV64 scalar `stdarg` LLVM output now models `va_list` as pointer storage and
  lowers scalar `va_arg` through cursor load, 8-byte cursor advance, cursor
  store, and scalar load from the original cursor.
- AArch64 still uses the structured `%struct.__va_list_tag_` route for the
  same scalar fixture.
- Focused regression coverage lives in
  `tests/frontend/frontend_lir_call_type_ref_test.cpp`.

Remaining semantic BIR, prepared ABI, final RV64 emission, broad c-testsuite
registration, aggregate `va_arg`, and wider-than-8-byte RV64 `va_arg` payload
support stay intentionally out of scope for this idea and require separate
source ideas if pursued.
