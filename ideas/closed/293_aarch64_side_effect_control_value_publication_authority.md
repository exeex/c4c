# AArch64 Side-Effect Control-Value Publication Authority

Status: Closed
Created: 2026-05-18
Closed: 2026-05-18
Split From: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Intent

Repair the AArch64 backend path where side-effecting expression results,
conditional-expression selected values, logical/comparison values, and
compound-assignment results must publish the authoritative semantic value to
later consumers.

## Why This Exists

The refreshed inventory after AArch64 scalar expression/control-value authority
closed still has a coherent runtime-mismatch family where the backend computes
or selects a value but the later observable consumer reads a stale register,
unrelated call result, old stack slot, or clobbered argument value.

Representative evidence from inventory Step 2:

- `src/00164.c` has side-effecting expressions such as `(y = c + d)`,
  logical `&&`/`||`, comparison values, and arithmetic expression values whose
  later `printf` consumers print stale or unrelated values.
- `src/00183.c` loses the selected result of a conditional expression, so the
  loop counter is printed instead of the selected `Count * Count` or
  `Count * 3` value.
- `src/00202.c` loses compound-assignment publication: `bob = jim *= 2`
  stores or reloads from an unrelated location, leaving both the expression
  result and assigned object stale.
- `src/00169.c` is a nearby boundary case where a nested-loop middle value is
  loaded and then clobbered by format-pointer setup before the variadic call
  consumes it. It should be used as support proof after the cleaner starters.

This owner is about the semantic publication chain for expression results with
side effects or control-selected values. It is not permission to reopen all
remaining runtime mismatches or pointer/address failures.

## In Scope

- AArch64 publication of scalar values produced by assignment expressions,
  compound assignments, logical/comparison expressions, and conditional
  expressions when the expression result is consumed later.
- Keeping the assigned object and the expression result in sync for
  side-effecting scalar expressions.
- Control-selected scalar value materialization when a branch or conditional
  expression chooses the value consumed by a later store, print, return, or
  call argument.
- Backend/prealloc authority plumbing that chooses the prepared semantic value
  over stale scratch, callee-saved, or clobbered argument registers.
- Focused proof beginning with `src/00164.c`, `src/00183.c`, and `src/00202.c`,
  then sampling `src/00169.c` as a boundary case.

## Out of Scope

- Pointer/aggregate/address-heavy cases such as `src/00172.c` and
  `src/00217.c` except as explicitly deferred follow-on evidence.
- Closed-owner overlap in scalar parameter/call-return or switch-dispatch
  cases such as `src/00159.c`, `src/00168.c`, and `src/00193.c` unless fresh
  generated-code evidence contradicts the closed owner.
- Timeout or hang repair for `src/00132.c`, `src/00220.c`, or other
  environment-sensitive loop/string/wide-character cases.
- Compile-stage printer/admission failures, floating/conversion/string-only
  failures, broad aggregate ABI work, parser/sema rewrites, libc behavior, or
  test harness changes.
- Runner behavior, expected outputs, allowlists, unsupported classifications,
  timeout policy, or CTest registration.

## Acceptance Criteria

- Starter representatives `src/00164.c`, `src/00183.c`, and `src/00202.c` no
  longer fail from stale or missing side-effect/control-value publication.
- Generated AArch64 assembly shows later consumers reading the authoritative
  selected or side-effected scalar value, not stale scratch, unrelated
  call-return, clobbered argument, or wrong stack-slot values.
- Compound assignment repairs update both the assigned object and expression
  result when both are semantically observable.
- Boundary sampling includes `src/00169.c` or an equivalent nearby case to
  show the repair survives call-argument setup without reopening closed
  call-argument authority.
- Pointer/aggregate, timeout, printer/admission, floating/conversion/string,
  and closed-owner overlap buckets remain explicitly separated unless fresh
  generated-code proof shows they share this exact publication primitive.
- No progress is claimed through expectation, runner, allowlist, timeout, or
  unsupported-classification changes.

## Closure Notes

Closed after Step 4 boundary validation. The focused boundary subset
`00164|00169|00183|00202` passed 4/4 in `test_after.log`, with the regression
guard showing no new failures against the matching Step 4 baseline. The
broader nearby sample passed the in-scope representatives `00164`, `00169`,
`00183`, and `00202`; the remaining failures `00159`, `00168`, `00193`, and
`00217` were explicitly separated as closed-owner overlap or
pointer/address/string-heavy work outside this source idea.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `src/00164.c`, `src/00183.c`, `src/00202.c`, `src/00169.c`,
  one local variable name, one loop, or one emitted instruction sequence
  instead of repairing side-effect/control-value publication authority;
- changes expected outputs, runner behavior, allowlists, unsupported
  classifications, timeout policy, or CTest registration;
- claims capability progress through helper renames, classification rewrites,
  or expectation churn while side-effecting expression consumers still read
  stale scratch registers, unrelated return values, clobbered argument
  registers, or wrong stack slots;
- broadens into pointer/aggregate address repair, timeout-loop investigation,
  compile-stage printer gaps, parser/sema, libc, floating/conversion/string
  behavior, or aggregate ABI work before proving the side-effect/control
  publication owner;
- reopens a closed AArch64 owner from residual failing counts alone without
  generated-code evidence contradicting that owner's closure;
- preserves the old failure mode behind a new abstraction name where prepared
  side-effect/control facts exist but emission still consumes fallback
  registers or stale memory.
