# AArch64 Backend Result Register Runtime Nonzero

Status: Open, inactive after result-register owner repair
Created: 2026-05-18
Discovered From: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md

## Lifecycle Handoff

The focused result-register owner layer was repaired by commit `d298e1945`.
Focused backend coverage now proves a non-rematerializable scalar return value
whose storage home is `x19` is retargeted into the prepared AArch64 ABI return
register, and `00003.c` assembly advanced from `sub w19, w0, #4; ret` to
`sub w0, w0, #4; ret`.

The original `[RUNTIME_NONZERO] exit=1` result-register failure is gone. The
same runtime route still fails as `[RUNTIME_NONZERO] exit=253` because the left
operand is still the incoming `w0` value instead of the source-local value
`4`. That is a separate local/operand materialization issue, now tracked by
`ideas/open/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md`.
Do not expand this idea into that owner layer.

## Intent

Repair the AArch64 backend codegen path that can compute a return expression
into a non-result register and then return without moving the value into `w0`.

The motivating runtime evidence is `tests/c/external/c-testsuite/src/00003.c`
on the AArch64 backend c-testsuite route. The route now reaches real runtime
execution and fails as `[RUNTIME_NONZERO] exit=1`; generated assembly currently
has the shape:

```asm
sub w19, w0, #4
ret
```

The route failure is truthful: the result is computed into `w19`, but the
AArch64 ABI return register `w0` is not updated before `ret`.

## Why This Exists

Idea 276 repaired the runtime execution route and proved simple passing cases
can compile to backend AArch64 assembly, assemble/link with clang, run, and
compare expected output. `00003.c` no longer exposes route infrastructure
failure; it exposes a backend result-register/codegen capability gap.

Keeping this as a separate idea prevents the runtime-route plan from becoming
a general backend repair bucket and gives reviewers a focused semantic
contract for the result-register fix.

## In Scope

- Identify where the AArch64 backend chooses or preserves the destination for
  expression results that feed C `return`.
- Repair the semantic lowering/codegen rule so integer return values are
  materialized in the ABI result register (`w0`/`x0` as appropriate) before
  function return.
- Preserve normal register allocation or move insertion behavior for nearby
  arithmetic-return cases, not only `00003.c`.
- Add focused proof that includes the failing runtime case and at least one
  nearby same-feature case.
- Preserve the backend `.s -> clang -x assembler -> executable -> runtime ->
  expected-output` proof route for c-testsuite evidence.

## Out of Scope

- Changing c-testsuite `.expected` sidecars, allowlists, or CTest expectations.
- Marking `00003.c` unsupported or weakening the route contract.
- Adding filename-specific handling for `00003.c` or c-testsuite-shaped
  shortcuts.
- Reworking runtime-runner availability, native-host detection, or the route
  diagnostics already handled by idea 276.
- Broad AArch64 ABI rewrites unrelated to returning integer expression values.

## Completion Criteria

- `src/00003.c` passes the AArch64 backend runtime route without expectation
  weakening.
- The generated AArch64 assembly returns the computed value through the ABI
  result register before `ret`.
- A focused nearby proof shows the repair is semantic rather than named-case
  matching.
- Existing passing runtime smoke cases such as `src/00001.c` and `src/00002.c`
  still pass through the same backend assembly runtime route.
- Any remaining non-result-register backend failures discovered during proof
  are recorded as separate follow-up ideas rather than folded into this one.

## Reviewer Reject Signals

Reject the slice if it:

- matches the filename `00003.c`, a c-testsuite path, or an exact emitted
  instruction string instead of repairing return-value lowering/register
  placement;
- changes `.expected` files, allowlists, unsupported classifications, or CTest
  expectations to make the route pass;
- routes the proof through LLVM IR fallback while claiming AArch64 backend
  runtime progress;
- only renames helpers or changes diagnostics while the emitted function can
  still compute into a non-result register and return without updating `w0`;
- fixes `00003.c` while leaving an equivalent nearby arithmetic-return case
  unexamined or still broken;
- expands into broad runtime-runner, c-testsuite harness, or unrelated ABI
  rewrites instead of the result-register/codegen issue.
