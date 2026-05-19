# AArch64 Scalar Immediate Materialize Or Encoding Fallback

Status: Closed
Created: 2026-05-19
Closed: 2026-05-19
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair the AArch64 backend path where scalar add, subtract, and bitwise
machine instructions reach assembly printing with immediate operands that are
outside the plain printable encoding range.

## Why This Exists

The post-298 backend-regex inventory found a crisp machine-printer residual
bucket after `lir_to_bir` global/pointer/aggregate projection work closed.
The affected cases are:

```text
00031, 00104, 00143, 00207, 00213, 00214, 00215, 00218
```

These failures share the same semantic signal: selected scalar add/xor/and or
nearby add/sub/bitwise forms carry constants to the AArch64 printer as plain
immediates even when those constants are not encodable as that instruction's
immediate operand. The backend should materialize the constant into a register
or lower through a structured printable fallback rather than failing in the
machine printer.

## In Scope

- AArch64 scalar integer add/subtract and bitwise immediate operand selection,
  normalization, materialization, or encoding fallback.
- The machine-printer residual cases `00031`, `00104`, `00143`, `00207`,
  `00213`, `00214`, `00215`, and `00218`.
- Focused generated-assembly or diagnostic inspection that proves whether the
  failing operands are non-encodable instruction immediates.
- Narrow backend proof for the focused c-testsuite cases, followed by the
  supervisor-selected broader backend-regex proof when the slice is ready.

## Out Of Scope

- Compare-branch operand-form work already closed by idea 296.
- `lir_to_bir` admission or aggregate projection work already covered by ideas
  297 and 298.
- Scalar cast, mul/div/rem, call-boundary move, memory store source/symbol,
  unsigned reduction, runtime nonzero, runtime mismatch, or timeout buckets
  from the post-298 inventory.
- Expectation rewrites, allowlist changes, unsupported classification changes,
  timeout policy changes, runner behavior changes, or CTest registration
  changes.
- Filename-specific or instruction-string-specific shortcuts.

## Acceptance Criteria

- The focused cases no longer fail from the old scalar immediate
  machine-printer diagnostic.
- Generated AArch64 assembly shows non-encodable scalar add/sub/bitwise
  constants are materialized into registers or otherwise lowered through a
  structured printable fallback before printing.
- Existing compare-branch, `lir_to_bir`, memory, call-boundary, runtime, and
  timeout residual buckets are not folded into this owner without new evidence.
- Fresh build proof and the focused c-testsuite backend subset are recorded.
- Any broader backend-regex proof is reported with the residual bucket changes
  separated from unrelated runtime or timeout behavior.

## Closure Note

Closed after the scalar ALU immediate materialization route removed the old
focused machine-printer diagnostic:

```text
scalar add/sub/bitwise immediate operand is outside the plain #imm encoding
range 0..4095
```

Fresh close proof used matching backend-regex reruns in `test_before.log` and
`test_after.log`: both selected 352 tests, stayed at 294 passed and 58 failed,
and introduced no new failures. This was non-regressive for lifecycle close,
not a strict pass-count regression-guard improvement. The old scalar immediate
diagnostic is absent from both focused and broad backend-regex proof.

The focused target set is now classified outside this owner: `00031` passes;
`00104` fails on invalid `sxtw w20, w13` cast spelling; `00213` and `00214`
fail on symbol-store value printing; `00207` and `00215` segfault; `00143`
times out; and `00218` reaches a runtime mismatch. Those residuals remain
parked for later focused owners under the umbrella inventory.

No expectation, unsupported, runner, timeout, allowlist, or CTest-registration
changes were part of the closure.

## Reviewer Reject Signals

Reject the route if it:

- special-cases one of `00031`, `00104`, `00143`, `00207`, `00213`, `00214`,
  `00215`, or `00218` instead of repairing scalar add/sub/bitwise immediate
  lowering or fallback semantics;
- only rewrites expected output, allowlists, unsupported classifications,
  timeout policy, runner behavior, or CTest registration while claiming
  backend capability progress;
- accepts an instruction string by widening the printer to emit non-encodable
  immediates instead of materializing or selecting a valid AArch64 form;
- folds scalar casts, mul/div/rem, memory store source/symbol, runtime
  nonzero/mismatch, or timeout residuals into this owner without generated-code
  or diagnostic evidence that they share the same immediate encoding failure;
- reopens closed owners 285 through 298 from residual failure counts alone
  without proof that their closure boundary was contradicted;
- introduces a broad AArch64 backend rewrite whose main effect is retaining the
  old non-encodable-immediate printer failure behind new helper names.
