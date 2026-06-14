Status: Active
Source Idea Path: ideas/open/257_phase_f3_x86_route6_call_argument_source_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Agreement Adapter Design

# Current Packet

## Just Finished

Step 2 - Agreement Adapter Design accepted the existing x86-local helper as
the adapter boundary and recorded the precise agreement and rejection contract.

Adapter boundary decision:

- Use `c4c::backend::x86::find_consumed_scalar_i32_call_argument_source_authority(...)`
  in `src/backend/mir/x86/x86.hpp` as the Step 3 implementation boundary.
- Do not introduce a smaller parallel query for this packet: the existing
  helper is already x86-local, not a public prepared API, and already receives
  the selected `ConsumedPlans`, block, call, block/instruction/arg indexes, and
  BIR argument needed for the one scalar named-`i32` fact.
- Keep `find_consumed_scalar_i32_call_argument_source(...)` as the compatibility
  convenience wrapper that returns only the Route 6 record after the authority
  helper succeeds.

Accepted agreement row:

- Argument must be a named `bir::TypeKind::I32` BIR call argument.
- Prepared evidence must include a `PreparedCallArgumentPlan` found through
  `find_consumed_call_argument_plan(...)` for the same
  `(block_index, instruction_index, arg_index)`.
- Route 6 evidence must come from
  `ConsumedPlans::shared_route6_call_use_source_index()` through
  `route6_find_call_argument_source(...)` for the same block, call instruction,
  callee, and argument index.
- Route 6 status must be available through an `ArgumentValue` row accepted by
  `route6_call_argument_source_matches_argument_value_record(...)`, including a
  matching argument value/name.
- Route 6 `source_value_id` and prepared `PreparedCallArgumentPlan::source_value_id`
  must both be present and equal.
- Route 6 `source_value_name` must be present and is the returned
  `ConsumedScalarI32CallArgumentSourceAuthority::source_name`.
- Public prepared call-plan compatibility remains observable through
  `PreparedBirModule::call_plans`, `PreparedFunctionLookups::call_plans`,
  `find_prepared_call_plans(...)`, `find_indexed_prepared_call_plan(...)`, and
  x86 `find_consumed_call_argument_plan(...)`.

Rejection matrix:

- Missing prepared call-plan row: return `std::nullopt`; prepared lookup APIs
  remain callable and unchanged.
- Missing Route 6 index or empty Route 6 index: return `std::nullopt`;
  fallback behavior continues through prepared call-plan selection.
- Invalid or unsupported argument shape, including non-named, non-`i32`,
  immediate, aggregate, ABI-bound, computed/symbol-address, direct-global,
  source-selection, register/stack policy, helper-selection, or target-policy
  rows: return `std::nullopt`; do not infer authority from prepared metadata.
- Route 6 record absent, missing-source-relationship, invalid, or any other
  non-available status: return `std::nullopt`; route-debug may report the
  existing blocked status but strings stay unchanged.
- Route 6 `ArgumentValue` mismatch against the BIR argument, including source
  value/name mismatch: return `std::nullopt`.
- Missing Route 6 `source_value_id`: return `std::nullopt`.
- Missing prepared `source_value_id`: return `std::nullopt`.
- Route 6/prepared `source_value_id` mismatch: return `std::nullopt`.
- Missing Route 6 `source_value_name`: return `std::nullopt` even when ids
  agree.
- Duplicate or conflicting Route 6 relationships/source evidence: return
  `std::nullopt` through the Route 6 record status/matching gate.
- Prepared-only evidence: return `std::nullopt`; do not promote prepared
  `source_value_id` alone into Route 6 semantic authority.
- Route 6-only evidence: return `std::nullopt`; do not bypass missing prepared
  agreement.
- Fallback rows: preserve prepared call-plan selection and emitted x86 asm when
  agreement is absent or non-authoritative.
- Policy-sensitive rows: leave ABI call sequence, argument/result layout,
  register/stack policy, helper/oracle status, wrapper instruction text,
  fallback names, route-debug strings, expected output, and `ConsumedPlans`
  contracts unchanged.

Compatibility surfaces confirmed unchanged:

- Public prepared call-plan APIs stay visible and stable:
  `PreparedBirModule::call_plans`, `PreparedFunctionLookups::call_plans`,
  `find_prepared_call_plans(...)`, `find_indexed_prepared_call_plan(...)`, and
  x86 `find_consumed_call_argument_plan(...)`.
- Existing route-debug gate names remain unchanged:
  `agreed`, `blocked`, `missing_source_value`, `missing_source_name`,
  `missing_prepared_source`, `prepared_source_mismatch`, and
  `source_value_mismatch`.
- Wrapper/fallback names, helper/oracle status, expected output strings,
  fallback assembly, and `ConsumedPlans` threading remain compatibility
  surfaces, not implementation targets for this design-only step.

## Suggested Next

Execute Step 3 by routing the selected x86 scalar named-`i32` call argument
source identity through the accepted
`find_consumed_scalar_i32_call_argument_source_authority(...)` boundary, only if
the code still has a caller that does not use this authority helper directly.

## Watchouts

- The authority helper already implements the Step 2 agreement predicates; Step
  3 should first check whether implementation work is already complete and only
  make a code change if a selected caller bypasses the authority boundary.
- Keep the `source_value_name` requirement as part of named authority; missing
  names are fallback rows, not partial successes.
- Route-debug status rows already name `gate=agreed`,
  `gate=missing_source_value`, `gate=missing_source_name`,
  `gate=prepared_source_mismatch`, `gate=source_value_mismatch`, and
  `gate=blocked`; do not rewrite these strings as an implementation shortcut.
- Riscv, ABI/register/stack/result policy, helper selection, wrapper
  instruction text, and fallback names remain out of scope.

## Proof

Command: `git diff --check -- todo.md`

Result: passed.

The delegated proof command is todo-only and does not produce `test_after.log`;
no replacement root log was created.
