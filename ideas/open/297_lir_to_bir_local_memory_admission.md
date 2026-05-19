# LIR To BIR Local Memory Admission

Status: Open
Created: 2026-05-19
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Intent

Repair the semantic `lir_to_bir` admission path for local-memory address
formation and local-memory load/store boundaries reached by AArch64
c-testsuite backend cases.

## Why This Exists

The refreshed post-296 backend-regex inventory selected 352 tests, passed 290,
and failed 62. All 62 failures are AArch64 c-testsuite backend tests. The
largest coherent compile-stage owner is the 14-case `lir_to_bir` admission
bucket, and the tractable focused subset is local-memory admission: 9 GEP
local-memory cases with nearby store/load boundary checks.

## Current Evidence

Confirmed direct local-memory GEP cases:

- `00143`
- `00157`
- `00185`

Residual GEP admissions reclassified outside this local-memory owner after
Step 2 diagnosis:

- Global dynamic scalar-array GEPs: `00176`, `00181`
- Pointer-parameter or pointer-value GEPs: `00182`, `00209`
- Global dynamic aggregate member GEPs: `00195`, `00205`

These residuals should become a separate global/pointer GEP projection idea
when that route is selected; they are not direct local-memory GEP Step 3 work.

Store/load boundary checks:

- Store local-memory cases: `00046`, `00140`
- Load local-memory cases: `00216`, `00218`

Separate gate not owned here:

- `00204` is a bootstrap global aggregate/array semantics gate. Do not absorb
  it into this owner without new evidence that it shares the same local-memory
  admission rule.

## In Scope

- Identify the semantic `lir_to_bir` admission gap for direct local-memory GEP
  forms.
- Repair local-memory address admission so GEP-derived local addresses are
  represented through the existing lowering model instead of being rejected.
- Use store and load local-memory cases as boundary checks for the same
  admission capability.
- Preserve the existing backend route and prove progress through compile/test
  evidence for the focused cases.
- Keep the repair general to local-memory GEP/store/load semantics rather than
  any named c-testsuite file.

## Out Of Scope

- Filename matching, local test-name shortcuts, or narrow matching on emitted
  diagnostics.
- Expectation rewrites, unsupported-classification changes, allowlist changes,
  timeout policy changes, runner changes, or CTest registration changes.
- Absorbing `00204` global aggregate/array bootstrap semantics into this owner
  without new evidence.
- Repairing global scalar-array, pointer-value, or global aggregate projection
  GEP admissions now represented by `00176`, `00181`, `00182`, `00195`,
  `00205`, and `00209`.
- Repairing unrelated runtime mismatch, timeout/hang, or machine-printer
  failures from the umbrella inventory.
- Broad rewrites of unrelated frontend, HIR, LIR, BIR, runtime, or AArch64
  printer behavior.

## Completion Criteria

- The confirmed direct local-memory GEP cases no longer fail at `lir_to_bir`
  admission for the old local-memory rejection reason.
- Store/load boundary cases are checked and either pass or have clearly
  recorded residual failures outside the repaired admission rule.
- Any residual failures in the focused set are classified by their new failure
  source instead of being hidden behind the old admission failure.
- Proof uses the supervisor-selected focused c-testsuite backend subset and
  includes fresh build or compile evidence appropriate to the touched code.
- No progress is claimed through expectation, unsupported, runner, timeout,
  allowlist, CTest-registration, or filename-specific changes.

## Reviewer Reject Signals

Reject the route if it:

- adds named-case checks for `00143`, `00157`, `00176`, `00181`, `00182`,
  `00185`, `00195`, `00205`, `00209`, `00046`, `00140`, `00216`, or `00218`
  instead of a semantic local-memory admission rule;
- changes expectations, unsupported classifications, allowlists, timeout
  policy, runner behavior, or CTest registration to improve counts;
- claims capability progress while the same local-memory `lir_to_bir`
  rejection still exists behind a renamed helper or alternate diagnostic;
- folds `00204` global aggregate/array bootstrap semantics into this owner
  without evidence that it shares the local-memory GEP/store/load rule;
- claims global scalar-array, pointer-value, or global aggregate projection GEP
  work as idea 297 progress instead of splitting it into a focused owner;
- treats unrelated runtime, timeout, or machine-printer failures as part of
  this owner;
- proves only one target case while nearby local-memory GEP/store/load cases
  remain unexamined;
- performs broad rewrites outside the `lir_to_bir` local-memory admission path
  without a clear source-idea update and lifecycle switch.
