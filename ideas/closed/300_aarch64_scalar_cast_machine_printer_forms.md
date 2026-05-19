# AArch64 Scalar Cast Machine Printer Forms

Status: Closed
Created: 2026-05-19
Closed: 2026-05-19
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair the AArch64 backend path where scalar integer cast operations reach the
machine printer with operands or widths that are not represented as supported
printable extension forms.

## Why This Exists

The committed post-299 backend-regex residual inventory found a focused
machine-printer/frontend scalar-cast bucket after scalar immediate owner 299
closed. The affected cases are:

```text
00035, 00105, 00126, 00134, 00135, 00151, 00208
```

These failures share scalar cast diagnostics instead of runtime-output
behavior. The residual symptoms include `zero_extend` requiring supported
integer source/result widths and `sign_extend` requiring a structured register
source before assembly printing. This points to a cast-lowering or cast-form
publication capability, not to testcase expectation changes or a broad backend
rewrite.

## In Scope

- AArch64 scalar integer cast lowering, operand publication, machine-node
  selection, or printer admission for zero-extension and sign-extension forms.
- The focused residual cases `00035`, `00105`, `00126`, `00134`, `00135`,
  `00151`, and `00208`.
- Narrow diagnostic or generated-assembly inspection that proves which cast
  form, source width, result width, and operand shape reaches the printer.
- Fresh build proof plus the supervisor-selected focused backend c-testsuite
  subset before any broader backend-regex acceptance proof.

## Out Of Scope

- Scalar add/sub/bitwise immediate materialization or encoding fallback already
  covered by closed idea 299.
- Fused compare-branch operand forms, `lir_to_bir` admission, and aggregate
  projection work already covered by closed ideas 296, 297, and 298.
- Symbol-store value printing, mul/div/rem operand forms, stack-slot store
  source scratch handling, unsigned reduction, unselected machine-node
  printer failures, runtime nonzero, runtime mismatch, and timeout buckets from
  the post-299 residual inventory.
- Expectation rewrites, allowlist changes, unsupported classification changes,
  timeout policy changes, runner behavior changes, or CTest registration
  changes.
- Filename-specific shortcuts or matching only the exact old diagnostic text.

## Acceptance Criteria

- The focused cases no longer fail from the old scalar-cast machine-printer
  diagnostics around unsupported `zero_extend` widths or unstructured
  `sign_extend` sources.
- Generated AArch64 assembly or printer diagnostics show scalar integer casts
  are lowered or published as valid structured extension forms before printing.
- The fix is semantic across the scalar-cast family and does not special-case
  individual c-testsuite filenames.
- Fresh build proof and the focused c-testsuite backend subset are recorded.
- Any broader backend-regex proof reports remaining residual buckets separately
  from this owner.

## Closure Note

Closed 2026-05-19 after Step 4 scalar-cast printer proof.

- Focused proof covered `00035`, `00105`, `00126`, `00134`, `00135`,
  `00151`, and `00208`.
- The old scalar-cast printer diagnostics are absent from the focused proof and
  the broad backend-regex proof: unsupported `zero_extend` forms,
  `sign_extend`, `scalar cast node requires a structured register source
  operand`, and related structured-source admission failures no longer appear.
- Focused residuals are outside this owner by current evidence: `00035` and
  `00151` fail with `RUNTIME_NONZERO exit=1`, and `00208` fails with
  `RUNTIME_NONZERO exit=Segmentation fault`.
- Broader backend-regex proof is `ctest --test-dir build -j10 -R backend
  --output-on-failure`, selecting 352 tests with 298 passing and 54 failing.
  Current residual buckets are frontend failures, one backend failure, runtime
  nonzero/mismatch or crash residuals, and timeouts; none are claimed as
  scalar-cast printer blockers without fresh generated-code evidence.
- Close-time regression guard used matching `test_before.log` and
  `test_after.log` backend-regex logs. Because `test_before.log` had already
  been rolled to the current 298/54 state, closure used the non-decreasing
  guard mode and passed with no new failures.

## Reviewer Reject Signals

Reject the route if it:

- special-cases one of `00035`, `00105`, `00126`, `00134`, `00135`, `00151`,
  or `00208` instead of repairing scalar cast lowering, operand publication,
  or structured machine-printer admission;
- only rewrites expected output, allowlists, unsupported classifications,
  timeout policy, runner behavior, or CTest registration while claiming
  backend capability progress;
- fixes only the exact current diagnostic string while leaving the same
  unsupported cast width or unstructured sign-extension operand failure behind
  a new helper name;
- folds symbol-store value printing, scalar mul/div/rem forms, stack-slot store
  source handling, unsigned reduction, runtime nonzero/mismatch, or timeout
  residuals into this owner without generated-code or diagnostic evidence that
  they share the scalar-cast failure mode;
- reopens closed owners 285 through 299 from residual failure counts alone
  without proof that their closure boundary was contradicted;
- performs a broad AArch64 backend rewrite whose main accepted evidence is one
  or two named scalar-cast cases rather than the focused cast family.
