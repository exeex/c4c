# AArch64 Scalar Machine Node Operand Forms

Status: Closed
Created: 2026-05-19
Split From: ideas/open/295_backend_regex_failure_family_inventory.md
Closed: 2026-05-19

## Goal

Repair the AArch64 backend path where scalar arithmetic and reduction machine
nodes reach assembly printing without structured operands the printer accepts.

## Why This Exists

The committed post-301 backend-regex residual inventory found a focused
compile-stage machine-node operand-form bucket after memory-store owner 301
closed. The affected cases are:

```text
00064, 00139, 00205
```

These failures are direct frontend/backend diagnostics rather than runtime
result triage. Current evidence shows selected scalar nodes reaching the
AArch64 printer with unsupported operand shapes:

- scalar `div` operand forms in `00064`;
- scalar `mul` operand forms in `00139`;
- scalar `logical_shift_right` unsigned-reduction operand forms in `00205`.

The shared owner is scalar machine-node operand publication, selection, or
printer admission for arithmetic/reduction forms. Progress means the backend
exposes valid AArch64 scalar operands before instruction emission, not that it
matches individual c-testsuite filenames or rewrites expectations.

## In Scope

- AArch64 scalar integer arithmetic and reduction machine-node operand
  publication, selection, materialization, or structured printer admission.
- The focused residual cases `00064`, `00139`, and `00205`.
- Focused diagnostic or generated-assembly inspection that proves which
  `div`, `mul`, or logical-shift/reduction operand reaches the printer in an
  unsupported form.
- Fresh build proof plus the supervisor-selected focused backend c-testsuite
  subset before broader backend-regex acceptance proof.

## Out Of Scope

- Fused compare-branch operand forms already covered by closed owner 296.
- Scalar add/sub/bitwise immediate materialization or encoding fallback already
  covered by closed owner 299.
- Scalar cast machine-printer forms already covered by closed owner 300.
- Memory-store operand materialization already covered by closed owner 301.
- Assembly legality/materialization singletons `00104` and `00182`, the
  call-boundary move printer gap `00140`, and `lir_to_bir` residuals `00204`
  and `00216`.
- Runtime nonzero, runtime mismatch/crash, timeout, output-storm, libc,
  floating, ABI, aggregate, pointer, string, and control-flow residual buckets
  from the post-301 inventory.
- Expectation rewrites, allowlist changes, unsupported classification changes,
  timeout policy changes, runner behavior changes, or CTest registration
  changes.
- Filename-specific shortcuts or matching only the exact current diagnostic
  strings.

## Acceptance Criteria

- The focused cases no longer fail from the old scalar `div`, scalar `mul`, or
  scalar `logical_shift_right` unsigned-reduction machine-node operand-form
  diagnostics.
- Generated AArch64 assembly or printer diagnostics show scalar arithmetic and
  reduction nodes are lowered, selected, materialized, or published as valid
  structured operands before instruction emission.
- The repair is semantic across the focused scalar operand-form family and does
  not special-case individual c-testsuite filenames.
- Fresh build proof and the focused c-testsuite backend subset are recorded.
- Any broader backend-regex proof reports remaining residual buckets
  separately from this owner and does not claim testcase-count progress without
  explaining the semantic bucket movement.

## Parked Lifecycle Note

Parked on 2026-05-19 when the active `00205` route moved past the old scalar
`logical_shift_right` unsigned-reduction operand-form diagnostic and exposed a
later assembler-legality failure on generated `sxtw w9, w13`. That sign-extension
spelling is outside this owner's scalar arithmetic/reduction machine-node
operand-form scope and is split to
`ideas/open/303_aarch64_sign_extension_assembler_legality.md`.

Do not close this idea or claim focused pass-count progress from the dirty
Step 3 implementation alone. Reactivation should first review the uncommitted
Step 3 slice, its focused proof, and whether the old `00205` unsigned-reduction
diagnostic is absent without absorbing the split sign-extension owner.

Closed after Step 1 close-candidate review recorded in commit `c59937bed`.
Fresh focused proof passed `00064`, `00139`, and `00205`; the old scalar
`div`, scalar `mul`, and scalar `logical_shift_right` unsigned-reduction
operand-form diagnostics were absent. Generated AArch64 evidence showed
structured scalar arithmetic forms including `sdiv w0, w9, w0` for `00064`
and `mul w0, w9, w13` for `00139`. Later `00205` residuals remain outside
this owner and are closed under split owners 303, 304, and 305.

Close-gate validation used matching canonical backend logs and passed with
`--allow-non-decreasing-passed`: before 139/139, after 139/139, no new
failures, and no slow tests.

## Reviewer Reject Signals

Reject the route if it:

- special-cases one of `00064`, `00139`, or `00205` instead of repairing
  scalar arithmetic/reduction operand publication, selection, materialization,
  or structured machine-printer admission;
- only rewrites expected output, allowlists, unsupported classifications,
  timeout policy, runner behavior, or CTest registration while claiming
  backend capability progress;
- fixes only the exact current diagnostic text while leaving scalar `div`,
  scalar `mul`, or logical-shift/reduction operands unstructured behind new
  helper names;
- folds `00104`, `00182`, `00140`, `00204`, `00216`, runtime residuals, or
  timeout/output-storm cases into this owner without generated-code or
  diagnostic evidence that they share the scalar machine-node operand-form
  failure mode;
- reopens closed owners 285 through 301 from residual failure counts alone
  without proof that their closure boundary was contradicted;
- performs a broad AArch64 backend rewrite whose main accepted evidence is one
  named scalar machine-node case rather than the full focused family
  `00064`, `00139`, and `00205`;
- claims progress through helper renames, expectation rewrites, or
  classification-only changes while the same unprintable scalar node operands
  still reach assembly emission.
