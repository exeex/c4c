# 246 Phase F1 prepared publication/status compatibility retention

## Goal

Keep prepared publication, source-memory, aggregate-stack-source,
current-block publication, route-debug, and helper-oracle status surfaces as
explicit compatibility contracts while route-native diagnostics are introduced
one fact family at a time.

## Why This Exists

Phase F0 found that prepared status strings and helper-oracle outputs still
carry public compatibility authority. They are not deletion candidates just
because Route 3, Route 4, Route 5, or Route 6 facts exist. A durable retention
contract is needed so future adapter work adds route-native diagnostics beside
prepared compatibility instead of weakening or renaming the existing surfaces.

## In Scope

- Compatibility coverage for prepared edge-publication source-memory statuses,
  typed-stack-source statuses, aggregate-stack-source statuses,
  current-block-entry publication statuses, helper-oracle statuses,
  route-debug strings, fallback strings, and baseline-visible wrapper output.
- A route-native diagnostic naming plan for Route 5 edge/source identity and
  Route 3 memory/source identity that coexists with prepared names.
- A route-native diagnostic naming plan for x86 Route 6 scalar call-use source
  identity that coexists with `ConsumedPlans` and route-debug compatibility.
- Tests or diagnostics that make compatibility fallback explicit rather than
  hidden behind renamed prepared helpers.

## Out Of Scope

- Implementing the x86 or riscv adapters themselves.
- Deleting, hiding, privatizing, or replacing whole prepared lookup groups.
- Renaming existing status strings or collapsing prepared and route-native
  statuses into one ambiguous enum.
- Opening draft 155 or claiming aggregate prepared retirement readiness.
- Broad baseline refreshes, expected-string rewrites, or unsupported
  downgrades.

## Acceptance Criteria

- Every retained prepared status family has an explicit compatibility owner and
  proof path.
- Route-native diagnostic rows are added only beside retained prepared rows
  until positive, missing, mismatch, duplicate, fallback, and wrapper-output
  behavior is proven independently.
- Prepared names such as `missing_prepared_memory_access`,
  `incomplete_prepared_memory_access`, `unsupported_source_home`,
  `missing_same_width_i32_type`, `missing_destination_gpr_bank`,
  `missing_aggregate_copy_authority`, and `publication_unavailable` remain
  stable where they are externally asserted.
- Compatibility fallback is visible in tests or diagnostics and is not hidden
  behind a renamed route-native facade.

## Proof Requirements

- Prepared lookup helper coverage for current status strings and helper-oracle
  statuses.
- Route-debug or diagnostic coverage showing new route-native rows, if added,
  are labeled separately from compatibility fallback rows.
- Targeted x86 and riscv wrapper-output checks proving retained compatibility
  does not alter emitted output.
- Matching before/after regression guard over prepared lookup helper,
  x86 route-debug, and riscv edge-publication tests.

## Reviewer Reject Signals

- Existing prepared status names, helper-oracle statuses, route-debug strings,
  fallback strings, or wrapper output are renamed or weakened without a
  separately approved compatibility-breaking idea.
- The implementation claims route-native ownership from string equality alone.
- Prepared fallback is hidden behind a facade rename while the old failure mode
  remains.
- Adapter work is smuggled into this compatibility-retention packet.
- The slice claims deletion, aggregate retirement, or draft 155 readiness.

## Closure Notes

Closed after matched before/after regression guard over
`backend_prepared_lookup_helper`, `backend_x86_route_debug`, and
`backend_riscv_prepared_edge_publication` passed 3/3 before and after.

Retained prepared compatibility owners remain explicit in the prepared lookup
helper and target edge-publication tests. x86 Route 6 and riscv Route 5/Route 3
diagnostic rows are separately named beside compatibility rows and do not
replace prepared fallback authority. This closure does not claim adapter
implementation, prepared status deletion, aggregate retirement, or draft 155
readiness.
