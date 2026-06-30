# RV64 Runtime External Abort Exit Support

Status: Open
Type: Runtime external support idea
Parent: `ideas/closed/435_rv64_coherent_aggregate_sret_call_storage_lowering.md`
Source Evidence: `build/agent_state/435_step3_sret_memory_return/summary.txt`
Owning Layer: RV64 runtime external call emission and support policy

## Goal

Resolve fail-closed RV64 codegen residuals for runtime external calls such as
`abort` and `exit` after unrelated aggregate `sret` call-storage blockers are
cleared.

## Why This Exists

Idea 435 confirmed that coherent same-module aggregate `sret` memory-return
call storage already has a prepared-fact-driven RV64 path and focused coverage.
Fresh probes for `src/20000917-1.c` and `src/20020206-1.c` then exposed the
first remaining non-sret residual as:

- `unsupported_external_call function='main' callee='abort' ... reason=unsupported runtime external symbol`
- `unsupported_external_call function='baz' callee='abort' ... reason=unsupported runtime external symbol`

The existing runtime mismatch triage idea excludes unsupported compile/codegen
failures, so this residual needs separate durable ownership.

## In Scope

- Classify RV64 runtime external call residuals for `abort`, `exit`, and
  closely related ordinary C runtime termination helpers.
- Decide whether each runtime external should lower through normal external
  call emission, a runtime support shim, or a fail-closed unsupported path.
- Add focused backend tests for semantic runtime external handling rather than
  representative testcase names.
- Preserve clear diagnostics for unsupported runtime externals outside the
  accepted policy.

## Out Of Scope

- Aggregate `sret`/`byval` lowering, including the coherent rows closed by
  idea 435.
- Runtime mismatch triage for programs that already compile, link, and run but
  produce wrong results.
- F128 helper policy, softfloat runtime helpers, inline asm, select lowering,
  local-publication, post-call aggregate copy, or pointer residuals.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Acceptance Criteria

- Runtime external `abort`/`exit` residuals are classified with a clear RV64
  lowering or fail-closed policy.
- Focused backend tests cover the accepted runtime external behavior and any
  rejected unsupported runtime external shape.
- Representative probes no longer misattribute these residuals to aggregate
  `sret` call-storage.
- Any remaining non-runtime residuals exposed afterward are routed separately.

## Reviewer Reject Signals

- Reject testcase-shaped handling keyed to `20000917-1`, `20020206-1`, `main`,
  `baz`, or one dump layout.
- Reject claiming aggregate `sret` progress through runtime external support
  changes.
- Reject expectation downgrades, unsupported-marker changes, allowlist edits,
  runtime-comparison weakening, or pass/fail accounting changes as progress.
- Reject broad runtime-helper rewrites that pull in F128, softfloat, inline
  asm, select, local-publication, pointer, or aggregate ABI work.
- Reject routes that silently treat unknown runtime externals as supported
  without explicit policy and focused tests.
