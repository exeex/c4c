# AArch64 Short-Circuit Control Publication Regression

Status: Closed
Created: 2026-05-21
Discovered After: ideas/closed/366_aarch64_string_literal_pointer_null_comparison.md
Closed: 2026-05-21

## Closure Summary

Completed by implementation `ea9d30f56` and proof updates `a5333beb4` /
`c37b51e85`. `c_testsuite_aarch64_backend_src_00196_c` passes again, and
`c_testsuite_aarch64_backend_src_00112_c` remains passing. The full-suite
regression guard passed against the accepted baseline with no new failing
tests, improving the suite from `3346` passed / `29` failed to `3347` passed /
`28` failed. After baseline roll-forward, the current canonical full-suite
logs still compare cleanly with no new failures.

## Goal

Repair the newly introduced AArch64 regression where short-circuit logical
`&&`/`||` lowering can evaluate the right-hand operand after the left-hand
side already determines the result.

## Why This Exists

Close-time full-suite baseline review after idea 366 reported one resolved
test and one new failure:

- resolved: `c_testsuite_aarch64_backend_src_00112_c`
- new failing: `c_testsuite_aarch64_backend_src_00196_c`

The full-suite regression guard compared `test_baseline.log` against the fresh
full-suite `test_after.log` and reported unchanged aggregate counts
(`3346` passed, `29` failed, `3375` total) but a test swap. A focused rerun
reproduces `c_testsuite_aarch64_backend_src_00196_c`.

Source `tests/c/external/c-testsuite/src/00196.c` exercises short-circuit
logical expressions using `fred()` returning `0` and `joe()` returning `1`.
The fifth expression, `fred() && (1 + joe())`, should stop after `fred()` and
print `0`; the regressed AArch64 path calls `joe()` and prints `1`. Because
the failure appeared after the string-pointer comparison repair, the leading
hypothesis is a control-value publication, compare publication, or prepared
address/materialization interaction that now corrupts short-circuit lowering.

## In Scope

- Localize the first changed boundary for `00196` between the last accepted
  full-suite baseline and the current tree.
- Inspect AArch64 lowering for short-circuit logical `&&`/`||`, control-value
  publication, compare publication, and any interaction with recent pointer
  comparison materialization changes.
- Repair the general short-circuit/control publication rule so the right-hand
  operand is not evaluated when the left-hand side determines the result.
- Add focused backend coverage for function-call short-circuit behavior that
  would catch the `fred() && (1 + joe())` regression without depending only on
  external `00196`.
- Prove `c_testsuite_aarch64_backend_src_00196_c` returns to the baseline
  result without losing the `00112` fix.

## Out Of Scope

- Reopening string-literal pointer/null comparison work except where its
  committed changes are proven to be the regression source.
- Broad backend inventory cleanup, unrelated C testsuite failures, timeout
  policy, runner behavior, CTest registration, unsupported classifications, or
  expectation rewrites.
- Reclassifying old known failures that existed in the accepted full-suite
  baseline.
- Fixing only `00196`, one print sequence, one function name, or one exact
  emitted assembly shape.

## Acceptance Criteria

- The first bad fact for `00196` is localized to a concrete AArch64
  short-circuit/control publication boundary.
- Focused coverage proves short-circuit function-call operands preserve the
  required evaluation order and skip behavior.
- `c_testsuite_aarch64_backend_src_00196_c` passes again.
- `c_testsuite_aarch64_backend_src_00112_c` remains passing.
- A matching regression guard for the supervisor-selected scope shows no new
  failures relative to the accepted baseline.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00196`, `fred`, `joe`, the fifth expression, one literal
  arithmetic wrapper such as `1 + joe()`, or a single emitted label/register
  sequence instead of repairing short-circuit/control publication generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through output-text reshuffling while short-circuit
  evaluation can still call a skipped right-hand operand;
- reopens unrelated backend inventory buckets or old known failures without
  proving they are required to resolve the new full-suite regression;
- drops or regresses the `00112` string-pointer comparison fix while repairing
  `00196`;
- treats aggregate pass/fail counts as sufficient even though the full-suite
  guard found a resolved/new-failure test swap.
