# AArch64 Backend Local Operand Materialization Runtime Nonzero

Status: Open
Created: 2026-05-18
Discovered From: ideas/open/277_aarch64_backend_result_register_runtime_nonzero.md

## Intent

Repair the AArch64 backend lowering path that leaves an operand for a local
integer value backed by the incoming `w0` register instead of materializing or
loading the source-local value before an arithmetic return expression.

The motivating runtime evidence is `tests/c/external/c-testsuite/src/00003.c`
after idea 277 repaired return-result register placement. Generated assembly
now returns through `w0`:

```asm
sub w0, w0, #4
ret
```

The result register is correct, but the left operand is still the incoming
`w0` value rather than the local `x = 4` value, so the route fails as
`[RUNTIME_NONZERO] exit=253`.

## Why This Exists

Idea 277 isolated and repaired the AArch64 return-result owner layer. The
remaining failure is truthful backend operand/local-value materialization, not
return register placement. Keeping it separate prevents the result-register
plan from becoming a broad backend repair bucket while preserving the same
runtime route evidence.

## In Scope

- Identify where local integer storage, assignment, or value references become
  AArch64 backend operands for arithmetic expressions.
- Repair the semantic lowering/codegen rule so local integer values used as
  operands are materialized from their actual source value or storage, not from
  unrelated incoming ABI registers.
- Preserve the result-register repair from idea 277.
- Prove `src/00003.c` through the AArch64 backend `.s -> clang -x assembler
  -> executable -> runtime -> expected-output` route.
- Include at least one nearby same-feature local-value arithmetic proof that
  rejects a named-case-only repair.

## Out of Scope

- Reworking AArch64 return-result register placement already handled by idea
  277.
- Changing c-testsuite `.expected` sidecars, allowlists, unsupported
  classifications, or CTest expectations.
- Adding filename-specific handling for `00003.c` or c-testsuite-shaped
  shortcuts.
- Routing the proof through LLVM IR fallback while claiming AArch64 backend
  progress.
- Broad register-allocation or ABI rewrites unrelated to local operand
  materialization for integer expressions.
- Runtime-runner, native-host detection, or route-diagnostic work.

## Completion Criteria

- `src/00003.c` passes the AArch64 backend runtime route without expectation
  weakening.
- Generated AArch64 assembly for the motivating case materializes the local
  value used by the subtraction rather than using incoming `w0` as that
  operand.
- Existing passing runtime smoke cases such as `src/00001.c` and `src/00002.c`
  still pass through the same backend assembly runtime route.
- A focused nearby local-value arithmetic proof shows the repair is semantic
  rather than named-case matching.
- The idea 277 result-register behavior remains intact: returned integer
  expression values are still written to `w0`/`x0` before `ret`.

## Reviewer Reject Signals

Reject the slice if it:

- matches the filename `00003.c`, a c-testsuite path, or an exact emitted
  instruction string instead of repairing local operand materialization;
- changes `.expected` files, allowlists, unsupported classifications, or CTest
  expectations to make the route pass;
- routes proof through LLVM IR fallback while claiming AArch64 backend runtime
  progress;
- reintroduces the idea 277 failure by computing the final return value outside
  `w0`/`x0` before `ret`;
- only renames helpers or changes diagnostics while the local value operand can
  still read from an unrelated incoming ABI register;
- proves only `00003.c` while leaving an equivalent nearby local integer
  arithmetic case unexamined or still broken;
- expands into broad runtime-runner, c-testsuite harness, register allocator,
  or unrelated ABI rewrites instead of the local operand materialization issue.
