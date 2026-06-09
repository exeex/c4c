Status: Active
Source Idea Path: ideas/open/138_call_plan_lookup_ownership_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move or re-export call lookup declarations through call ownership

# Current Packet

## Just Finished

Step 2 moved the call-plan lookup declaration surface into
`src/backend/prealloc/calls.hpp` while preserving existing data shape and
definitions.

- `calls.hpp` now owns `PreparedPriorPreservedValueEntry`,
  `PreparedPriorPreservedValueLookupStatus`,
  `PreparedPriorPreservedValueLookupResult`, `PreparedCallPlanLookups`,
  `prepared_call_position_key`, `prepared_call_argument_position_key`,
  `prepared_prior_preserved_value_entry_position_less`,
  `make_prepared_call_plan_lookups`, and the call lookup query declarations.
- `prepared_lookups.hpp` now keeps `PreparedFunctionLookups` and non-call broad
  lookup declarations, with `PreparedFunctionLookups::call_plans` still using
  `PreparedCallPlanLookups` through the existing `calls.hpp` include.
- No behavior, field layout, definition placement, call semantics, ABI
  classification, preservation logic, or AArch64 codegen paths were changed.

## Suggested Next

Execute Step 3 from `plan.md`: review whether the existing definitions and
`PreparedFunctionLookups::call_plans` aggregate construction should remain in
`prepared_lookups.cpp` or be rewired to a smaller compilation-unit boundary,
without changing lookup indexing behavior.

## Watchouts

- `calls.hpp` forward-declares `PreparedBirModule` and
  `PreparedControlFlowFunction`; avoid adding `module.hpp` or
  `control_flow.hpp` includes there unless Step 3 proves a real blocker.
- The builder/query definitions still live in `prepared_lookups.cpp`; moving
  them may require accounting for prepared module/control-flow helpers and
  local dominance helpers currently used in that file.
- Keep `PreparedFunctionLookups`, non-call lookup structs, and broad prepared
  lookup helper declarations in `prepared_lookups.hpp` unless Step 3 exposes a
  concrete ownership reason to change them.
- Do not change ABI classification, call lowering semantics, preservation
  behavior, or AArch64 register handling.
- Do not replace prepared call lookups with local scans in AArch64 consumers.

## Proof

`cmake --build --preset default` succeeded. No root-level proof log was created
because the delegated packet explicitly listed root-level logs under Do Not
Touch.
