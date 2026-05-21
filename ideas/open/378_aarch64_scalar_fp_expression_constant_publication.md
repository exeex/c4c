# AArch64 Scalar FP Expression Constant Publication

Status: Open
Created: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair the AArch64 scalar floating-point expression and constant publication
residual currently represented by `c_testsuite_aarch64_backend_src_00174_c`.

## Why This Exists

After the external/libc call-result owner closed, the refreshed backend
inventory leaves four residuals: `00174`, `00216`, `00200`, and `00207`.
`00174` is the strongest next non-timeout owner. The source is a focused
float/double arithmetic, comparison, assignment, unary, coercion, and `sin(2)`
test. Current runtime output still reaches the later comparison rows and final
`2.000000` / `0.909297` lines, while the expected early arithmetic constants
print as zero.

Generated `build/c_testsuite_aarch64_backend/src/00174.c.s` prints from FP
registers such as `d13` without clear preceding materialization of the first
constant or expression values. The first bad fact is missing or lost scalar FP
constant/expression publication before ordinary scalar FP consumers, not broad
control flow, aggregate initialization, external call-result handling, or a
timeout.

## In Scope

- Localize the first scalar FP value that is not materialized or preserved
  before its consumer in generated AArch64 for `00174`.
- Determine whether the owner is FP literal materialization, prepared scalar
  FP expression lowering, assignment/unary/coercion value publication,
  FP-register consumer handoff, or a nearby scalar FP constant-expression path.
- Repair the classified AArch64 scalar FP publication owner generally.
- Add focused backend coverage for scalar float and/or double constants or
  expression values feeding ordinary FP consumers before relying on the
  external `00174` representative.
- Preserve the closed external/libc call-result, aggregate local-address,
  static/global selected-value, scalar integer immediate, and HFA/variadic
  closure boundaries unless fresh generated-code evidence contradicts them.

## Out Of Scope

- Aggregate initializer, compound literal, relocation, or function-pointer
  table behavior currently parked under `00216`.
- Dynamic stack/VLA/goto timeout behavior in `00207`.
- Shift/type-promotion timeout or output-storm behavior in `00200`.
- External/libc call-result publication for `00187` or reopened call-result
  handling without fresh first-bad-fact evidence.
- HFA/variadic floating argument or `va_arg` handling from idea 326 unless
  generated-code evidence reaches that exact boundary.
- Expectation changes, unsupported-classification changes, allowlist changes,
  runner behavior, timeout policy, CTest registration, or proof-log behavior.
- Fixing only `00174`, one literal, one output row, one FP register, one stack
  slot, or one emitted instruction sequence.

## Acceptance Criteria

- The first bad fact is localized to a concrete scalar FP constant/expression
  materialization or publication boundary with generated-code or focused dump
  evidence.
- Focused backend coverage fails without the repair and passes with it for a
  scalar float and/or double constant or expression feeding an ordinary
  consumer.
- `c_testsuite_aarch64_backend_src_00174_c` advances past the zero-valued
  early FP arithmetic output or passes.
- `00216`, `00200`, and `00207` remain classified as separate parked buckets
  unless fresh evidence proves a shared semantic owner.
- Supervisor-selected backend guardrails remain stable.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00174`, one printed row, one literal, one temporary, one FP
  register such as `d13`, one stack offset, or one emitted instruction
  sequence instead of repairing scalar FP constant/expression publication;
- weakens expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, proof-log policy, CTest registration, or external
  test contracts;
- claims progress through helper renames, diagnostics, classification-only
  notes, or output-count changes without making generated FP consumers observe
  the intended scalar FP value;
- broadens into aggregate initialization/relocation, VLA/dynamic stack,
  shift/type-promotion timeout, HFA/variadic floating, or external call-result
  work without fresh first-bad-fact evidence and lifecycle handoff;
- leaves generated AArch64 able to print or consume an unmaterialized stale FP
  register where a scalar float or double constant/expression value should be
  observed.
