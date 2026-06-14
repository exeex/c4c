# 257 Phase F3 x86 Route 6 call argument source identity adapter

## Idea Type

x86-local implementation packet.

## Goal

Implement one x86-local adapter for scalar named-`i32` call argument source
identity after Route 6 and prepared `source_value_id` agreement.

## Why This Exists

Idea 249 proved that broad x86/riscv call identity parity is not ready, but it
did identify one bounded x86 fact that can safely move toward implementation:
scalar `i32` call argument source identity, only after Route 6 and prepared
metadata agree on the same source value.

## In Scope

- Add an x86-local adapter for scalar named-`i32` call argument source identity.
- Require agreement between Route 6 call-use/source evidence and prepared
  `source_value_id` before the adapter claims semantic authority.
- Preserve public prepared call-plan APIs and all compatibility output.
- Fail closed for missing, invalid, duplicate/conflict, mismatch,
  unsupported, prepared-only, Route 6-only, fallback, and policy-sensitive
  rows.
- Add or preserve focused proof that the adapter does not weaken existing
  prepared call-plan, route-debug, wrapper, fallback, helper/oracle, or
  `ConsumedPlans` surfaces.

## Out Of Scope

- Riscv implementation or riscv parity claims.
- Deleting, privatizing, renaming, or bypassing
  `PreparedFunctionLookups::call_plans`, `PreparedBirModule::call_plans`, or
  `find_prepared_call_plans`.
- Moving ABI call sequence, argument/result layout, register/stack policy,
  helper selection, wrapper instruction text, fallback names, route-debug
  strings, or public expected strings into Route 6 or BIR.
- Broad Route 6 migration beyond the selected scalar call argument
  source-identity fact.

## Acceptance Criteria

- The adapter is x86-local and limited to scalar named-`i32` call argument
  source identity.
- The adapter reads the semantic fact only when Route 6 and prepared
  `source_value_id` agreement gates pass.
- Prepared-only, Route 6-only, mismatch, unsupported, duplicate/conflict, and
  fallback rows fail closed without weakening expected output.
- Public prepared call-plan APIs and compatibility rows remain observable and
  stable.
- Proof covers the focused x86 adapter path plus unchanged prepared lookup,
  route-debug, wrapper/fallback, helper/oracle, and `ConsumedPlans` behavior.
- Riscv remains explicitly non-applicable unless a separate source idea adds a
  real riscv Route 6 call-plan consumer.

## Reviewer Reject Signals

- Reject any implementation that treats prepared-only call-plan evidence as
  Route 6 semantic agreement.
- Reject named-case shortcuts that only satisfy one prior accepted scalar row
  while weakening nearby missing, mismatch, unsupported, duplicate/conflict, or
  fallback behavior.
- Reject unsupported expectation downgrades, weaker `ConsumedPlans` contracts,
  wrapper-baseline rewrites, route-debug text rewrites, or helper/oracle status
  renames.
- Reject deleting, privatizing, renaming, or bypassing the public prepared
  call-plan APIs as part of this adapter.
- Reject riscv parity claims, ABI/register/stack/result-policy movement, or
  broad Route 6 call-lowering rewrites under this x86-local idea.
- Reject an abstraction that preserves the old prepared semantic source while
  only renaming it as Route 6 or BIR authority.

## Completion Note

Closed after confirming the selected x86 scalar named-`i32` call argument
source identity path was already routed through
`find_consumed_scalar_i32_call_argument_source_authority(...)`. The retained
adapter boundary requires Route 6 call-use/source evidence and prepared
`source_value_id` agreement, preserves public prepared call-plan surfaces, and
fails closed for one-sided, mismatch, unsupported, fallback, and
policy-sensitive rows.

Default-build compatibility proof passed for the prepared/stdarg handoff
subset, and the direct x86 proof was run in `build-x86` with
`C4C_ENABLE_X86_BACKEND_TESTS:BOOL=ON`. The x86-enabled close proof covered
`backend_x86_route_debug` and `backend_x86_handoff_boundary`, both passing
2/2, with the canonical regression guard accepting matching before/after logs.
