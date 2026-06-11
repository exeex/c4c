# 194 Prepared printer/debug/oracle replacement planning

## Goal

Define the route-native diagnostics and oracle strategy required before public
prepared printer, debug, route-debug, and oracle-test surfaces can be
contracted.

## Why This Exists

The Phase D selected-consumer migrations retained prepared diagnostics and
oracles as validation surfaces. The pre-Phase-E readiness audit concludes that
production-only green tests are not enough to delete or privatize those
surfaces. Replacement diagnostics and equivalent oracle coverage must be
planned before true Phase E retirement analysis can claim prepared API
contraction readiness.

## In Scope

- Inventory prepared printer, debug, route-debug, dump, and oracle-test
  consumers that still read prepared route facts or `PreparedFunctionLookups`.
- Define the route-native diagnostic output needed to replace each retained
  prepared diagnostic surface.
- Define positive, negative, ambiguous, and fallback oracle coverage expected
  for each affected route family.
- Identify compatibility adapters that may be needed while prepared and
  route-native diagnostics coexist.
- Produce a retirement-readiness checklist for diagnostic/oracle surfaces.

## Out Of Scope

- Removing existing prepared diagnostics before replacements exist.
- Replacing production lowering behavior as part of a diagnostics-only plan.
- Deleting oracle tests or weakening expected output to match current
  implementation gaps.
- Creating target-specific route adapters solely for diagnostic convenience.
- Opening draft 155 or demoting `PreparedBirModule` fields.

## Acceptance Criteria

- A durable inventory names each prepared printer/debug/oracle consumer that
  blocks API contraction.
- Every retained surface has either a route-native replacement plan or an
  explicit retained-oracle rationale.
- The plan states what equivalence coverage is required before a prepared
  diagnostic or oracle surface can contract.
- Compatibility needs between prepared and route-native diagnostics are named.
- Future implementation ideas can be split by diagnostic/oracle surface without
  collapsing into a broad retirement rewrite.

## Reviewer Reject Signals

- Removing or privatizing prepared diagnostics before route-native replacements
  and equivalent oracle coverage exist.
- Treating production lowering success or full-suite greenness as proof of
  diagnostic/oracle readiness.
- Weakening route-debug, wrapper, oracle, c_testsuite, or baseline expectations
  to claim contraction progress.
- Claiming capability progress through printer label renames or output-only
  rewrites.
- Introducing testcase-shaped diagnostic shortcuts that only satisfy a named
  expected-output file.
- Combining unrelated production lowering changes into the diagnostic planning
  slice.
