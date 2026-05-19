# LIR To BIR Global Pointer Aggregate Projection

Status: Open
Created: 2026-05-19
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Intent

Repair the semantic `lir_to_bir` admission path for global, pointer-derived,
and aggregate projection address formation reached by the AArch64 c-testsuite
backend route.

## Why This Exists

Umbrella inventory idea 295 classified the accepted backend-regex baseline in
`test_before.log` as 352 selected tests, 291 passed tests, and 61 failed
tests. Closed idea 297 completed the direct local-memory admission owner and
explicitly left residual global, pointer, and aggregate projection admissions
outside its closure boundary.

The clearest remaining compile-stage semantic owner is the residual
`lir_to_bir` projection family:

- global scalar-array GEPs: `00176`, `00181`
- pointer-value or pointer-parameter GEPs: `00182`, `00209`
- global dynamic aggregate member GEPs: `00195`, `00205`
- bootstrap/global aggregate semantics: `00204`
- boundary case: `00216` pointer-parameter/flexible-array aggregate
  projection

This owner is separate from the AArch64 machine-printer residuals, runtime
nonzero or mismatch buckets, and the standalone `00220` timeout.

## In Scope

- Identify the semantic projection categories that `lir_to_bir` must admit for
  global scalar arrays, pointer-derived values or parameters, global aggregate
  members, and bootstrap/global aggregate semantics.
- Repair projection admission so these addresses are represented through the
  existing lowering model instead of being rejected by coarse GEP diagnostics.
- Inspect `00216` as a boundary case for pointer-parameter/flexible-array
  aggregate projection and either include it when it shares the same admission
  rule or classify it as an adjacent focused owner.
- Prove progress with the supervisor-selected focused backend subset covering
  the full residual projection family, not only one starter testcase.
- Record any residual failures by their new failure source after the old
  projection admission failure is removed.

## Out Of Scope

- Filename matching, test-number shortcuts, or matching on exact diagnostic
  strings.
- Expectation rewrites, unsupported-classification changes, allowlist changes,
  timeout policy changes, runner changes, or CTest registration changes.
- Reopening closed local-memory admission idea 297 unless generated diagnostic
  evidence contradicts its closure boundary.
- Repairing AArch64 machine-printer residuals, including scalar opcode,
  integer cast, non-encodable immediate, stack-slot store source, or
  call-boundary move printer failures.
- Repairing runtime nonzero or runtime mismatch buckets without generated-code
  evidence that assigns them to this projection owner.
- Folding the standalone `00220` timeout/hang case into this owner.
- Broad rewrites of unrelated frontend, HIR, LIR, BIR, runtime, or AArch64
  printer behavior.

## Completion Criteria

- The focused global scalar-array, pointer-value or pointer-parameter, global
  aggregate member, and bootstrap/global aggregate cases no longer fail at the
  old residual `lir_to_bir` projection admission point.
- `00216` is either repaired as part of the same projection capability or
  explicitly classified as a separate pointer-parameter/flexible-array
  aggregate owner with evidence.
- Any remaining focused-scope failures are classified by their new failure
  source instead of being hidden behind the old GEP admission diagnostic.
- Proof uses the supervisor-selected focused c-testsuite backend subset and
  includes fresh build or compile evidence appropriate to the touched code.
- No progress is claimed through expectation, unsupported, runner, timeout,
  allowlist, CTest-registration, filename-specific, or diagnostic-string-only
  changes.

## Reviewer Reject Signals

Reject the route if it:

- adds named-case checks for `00176`, `00181`, `00182`, `00195`, `00204`,
  `00205`, `00209`, or `00216` instead of a semantic projection admission
  rule;
- changes expectations, unsupported classifications, allowlists, timeout
  policy, runner behavior, or CTest registration to improve counts;
- claims capability progress while the same global, pointer, or aggregate
  projection admission failure still exists behind a renamed helper or
  alternate diagnostic;
- reopens idea 297 from failing counts alone instead of generated diagnostic
  evidence that contradicts its local-memory closure boundary;
- folds machine-printer residuals, runtime nonzero or mismatch cases, or the
  `00220` timeout into this owner without evidence that they share the same
  projection admission rule;
- proves only one target case while nearby residual projection cases remain
  unexamined;
- performs broad rewrites outside the `lir_to_bir` projection admission path
  without a clear source-idea update and lifecycle switch.
