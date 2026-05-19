# AArch64 Memory Store Operand Materialization

Status: Closed
Created: 2026-05-19
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair the AArch64 backend path where memory-store machine instructions reach
the machine printer with source or address/value operands that are not
materialized into printable register, immediate, or symbolic forms.

## Why This Exists

The committed post-300 backend-regex residual inventory found a focused
machine-printer memory-store bucket after scalar-cast owner 300 closed. The
affected cases are:

```text
00173, 00176, 00181, 00182, 00187, 00194, 00213, 00214
```

These failures are compile-blocking printer diagnostics, not runtime-result
triage. The inventory separates them into stack/local store source scratch
forms and symbol/global store value forms:

- memory store source scratch not printable: `00173`, `00187`, `00194`;
- memory store symbol value not register/immediate: `00176`, `00181`,
  `00182`, `00213`, `00214`.

The shared owner is store operand materialization or publication before
machine printing. Progress means the backend exposes valid AArch64 store
operands, not that it changes c-testsuite expectations or hides the cases.

## In Scope

- AArch64 memory-store lowering, operand publication, materialization, and
  machine-printer admission for store source, address, and symbolic value
  operands.
- The focused residual cases `00173`, `00176`, `00181`, `00182`, `00187`,
  `00194`, `00213`, and `00214`.
- Focused diagnostic or generated-assembly inspection that proves which store
  operand reaches the printer unmaterialized.
- Fresh build proof plus the supervisor-selected focused backend c-testsuite
  subset before broader backend-regex acceptance proof.

## Out Of Scope

- Scalar immediate fallback, scalar casts, fused compare-branch forms,
  `lir_to_bir` local-memory admission, and aggregate projection work already
  covered by closed owners 296 through 300.
- Scalar mul/div/rem operand forms, call-boundary move forms, unsigned
  reduction/logical-shift-right gaps, invalid scalar cast spelling, runtime
  nonzero, runtime mismatch/crash, and timeout buckets from the post-300
  inventory.
- Expectation rewrites, allowlist changes, unsupported classification changes,
  timeout policy changes, runner behavior changes, or CTest registration
  changes.
- Filename-specific shortcuts or matching only the exact current diagnostic
  strings.

## Acceptance Criteria

- The focused cases no longer fail from the old memory-store machine-printer
  diagnostics for unprintable store source scratch operands or symbol values
  that are not register/immediate operands.
- Generated AArch64 assembly or printer diagnostics show store operands are
  materialized or published as valid printable forms before instruction
  emission.
- The repair is semantic across the memory-store operand family and does not
  special-case individual c-testsuite filenames.
- Fresh build proof and the focused c-testsuite backend subset are recorded.
- Any broader backend-regex proof reports remaining residual buckets separately
  from this owner.

## Closure Note

Closed 2026-05-19 as complete for the AArch64 memory-store operand
materialization owner.

The accepted closure proof rebuilt the default preset and reran the backend
regex scope with:

```bash
timeout 900s ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1
```

The broad proof reports 352 selected tests, 300 passed, and 52 failed. Matching
regression guard comparison against `test_before.log` passed in documented
non-decreasing mode because the available baseline was already rolled forward
to the same 300/52 state; no new failing tests were introduced.

The focused memory-store cases no longer fail from the old memory-store
operand printer diagnostics. Current focused residuals are outside this owner
by present evidence: `00173`, `00181`, and `00214` are runtime
nonzero/segfault residuals; `00176` is a runtime mismatch; `00182` is a
backend assembler immediate-encoding residual; and `00187` times out. `00194`
and `00213` pass.

The broad backend log contains no `memory store`, `store source`, `store value
is not`, `symbol store value is not`, or `not a register or immediate operand`
diagnostic. Remaining broad residuals are runtime nonzero, runtime mismatch,
frontend/backend residuals outside the old store-printer mode, and timeouts.
Do not reopen this owner without generated-code or diagnostic evidence that a
residual failure again reaches AArch64 machine printing with an unmaterialized
memory-store source, address, or symbol value operand.

## Reviewer Reject Signals

Reject the route if it:

- special-cases one of `00173`, `00176`, `00181`, `00182`, `00187`, `00194`,
  `00213`, or `00214` instead of repairing memory-store operand
  materialization, publication, or structured machine-printer admission;
- only rewrites expected output, allowlists, unsupported classifications,
  timeout policy, runner behavior, or CTest registration while claiming
  backend capability progress;
- fixes only the exact current diagnostic text while leaving the same
  unmaterialized store source, address, or symbol-value operand failure behind
  a new helper name;
- folds scalar ALU, cast, compare-branch, `lir_to_bir`, runtime, or timeout
  residuals into this owner without generated-code or diagnostic evidence that
  they share the memory-store operand failure mode;
- reopens closed owners 285 through 300 from residual failure counts alone
  without proof that their closure boundary was contradicted;
- performs a broad AArch64 backend rewrite whose main accepted evidence is one
  or two named memory-store cases rather than the focused store operand family.
