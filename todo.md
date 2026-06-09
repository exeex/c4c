Status: Active
Source Idea Path: ideas/open/138_call_plan_lookup_ownership_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect current call lookup ownership and dependencies

# Current Packet

## Just Finished

Step 1 inspection mapped the current call-plan lookup ownership and dependency
constraints.

- `PreparedCallPlanLookups` is declared only in
  `src/backend/prealloc/prepared_lookups.hpp:41`. It owns
  `calls_by_position`, `outgoing_stack_argument_areas_by_position`,
  `immediate_arguments_by_position_and_abi`, `prior_preserved_by_value`,
  `first_stack_preserved_by_call_index`, and
  `block_entry_republication_effects_by_block`.
- The call lookup helper/result declarations adjacent to that type are
  `PreparedPriorPreservedValueEntry`,
  `PreparedPriorPreservedValueLookupStatus`,
  `PreparedPriorPreservedValueLookupResult`,
  `prepared_call_position_key`,
  `prepared_call_argument_position_key`,
  `prepared_prior_preserved_value_entry_position_less`,
  `make_prepared_call_plan_lookups`,
  `find_indexed_prepared_call_plan`,
  `find_indexed_prepared_immediate_call_argument`,
  `find_latest_indexed_prior_preserved_value`,
  `find_dominating_indexed_prior_preserved_value`,
  `find_unique_indexed_prior_preserved_value_source`,
  `find_latest_indexed_prior_stack_preserved_value_before_instruction`,
  `first_indexed_stack_preserved_values_for_call`,
  `find_indexed_prepared_outgoing_stack_argument_area`, and
  `indexed_block_entry_republication_effects_for_block`.
- `make_prepared_call_plan_lookups` is defined in
  `src/backend/prealloc/prepared_lookups.cpp:1244`; AST callers show it is
  constructed only by `make_prepared_function_lookups` at
  `src/backend/prealloc/prepared_lookups.cpp:1770`, which stores it in
  `PreparedFunctionLookups::call_plans`.
- Constructor/index population also uses local helpers in
  `prepared_lookups.cpp`: `collect_block_entry_republication_effects` at line
  615, `prepared_call_position_key` at line 1107,
  `prepared_call_argument_position_key` at line 1120, and
  `prepared_prior_preserved_value_entry_position_less` at line 1195.
- Query definitions are in `prepared_lookups.cpp`: indexed call plan at line
  2061, indexed immediate call argument at line 2087, latest prior preserved at
  line 3602, dominating prior preserved at line 3630, unique prior-preserved
  source at line 3669, latest prior stack-preserved before instruction at line
  3728, first stack-preserved values for a call at line 3763, outgoing stack
  argument area at line 3788, and block-entry republication effects at line
  3804.
- `src/backend/prealloc/call_plans.cpp` includes `prepared_lookups.hpp` and
  consumes `PreparedCallPlanLookups` in
  `append_incremental_prior_preserved_values` at line 2108,
  `select_prepared_call_argument_source` at line 2130, and the incremental
  `prior_preserved_lookups` construction at line 2591. It calls
  `find_unique_indexed_prior_preserved_value_source` at line 2346.
- AArch64 call lowering uses call lookup facts through
  `FunctionLoweringContext::call_plan_lookups` in
  `src/backend/mir/aarch64/module/module.hpp:84`. Known call consumers in
  `src/backend/mir/aarch64/codegen/calls.cpp` call
  `find_indexed_prepared_call_plan` at line 73,
  `find_latest_indexed_prior_stack_preserved_value_before_instruction` at line
  1927, `find_indexed_prepared_immediate_call_argument` at line 4162,
  `indexed_block_entry_republication_effects_for_block` at line 5622, and
  `first_indexed_stack_preserved_values_for_call` at line 8624.
- Include findings: `prepared_lookups.hpp` already includes `calls.hpp`, and
  `calls.hpp` already owns `PreparedCallPlan`,
  `PreparedOutgoingStackArgumentArea`, `PreparedCallArgumentPlan`,
  `PreparedCallPreservedValue`, and `PreparedCallBoundaryEffectPlan`.
  `calls.hpp` does not include `module.hpp` or `control_flow.hpp`; the call
  lookup declarations can avoid cycles by forward-declaring
  `PreparedBirModule` and `PreparedControlFlowFunction` and adding only the
  standard containers needed by the moved structs.
- Consumer/include scan did not find a target file that directly includes
  `prepared_lookups.hpp` only for the named call lookup helpers. `call_plans.cpp`
  also uses broader prepared lookup helpers; AArch64 call lowering reaches the
  declarations indirectly through `module.hpp`; `globals.cpp` uses non-call
  prepared lookup helpers; prepared printer files include the broader prepared
  module/private boundary rather than call lookup APIs directly.
- Smallest intended call-owned declaration boundary: move
  `PreparedPriorPreservedValueEntry`,
  `PreparedPriorPreservedValueLookupStatus`,
  `PreparedPriorPreservedValueLookupResult`,
  `PreparedCallPlanLookups`, the call-position/call-argument key helpers, the
  prior-preserved ordering helper, `make_prepared_call_plan_lookups`, and all
  `PreparedCallPlanLookups` query declarations listed above into
  `src/backend/prealloc/calls.hpp`. Leave `PreparedFunctionLookups` and broad
  aggregate construction in `prepared_lookups.hpp/.cpp`.
- Dependency-cycle blocker: none found for declaration movement into
  `calls.hpp` if `PreparedBirModule` and `PreparedControlFlowFunction` remain
  forward declarations there. Moving the definitions out of
  `prepared_lookups.cpp` would be a larger Step 3 decision because the builder
  currently depends on prepared module/control-flow helpers and local dominance
  helpers.

## Suggested Next

Execute Step 2 from `plan.md`: move the call lookup declarations into
`src/backend/prealloc/calls.hpp` while keeping definitions and
`PreparedFunctionLookups` wiring in `prepared_lookups.*`.

## Watchouts

- Include `<unordered_map>` in `calls.hpp` when moving
  `PreparedCallPlanLookups`.
- Add forward declarations for `PreparedBirModule` and
  `PreparedControlFlowFunction` in `calls.hpp`; do not include `module.hpp` or
  `control_flow.hpp` from `calls.hpp`.
- Keep `prepared_lookups.hpp` including `calls.hpp` and avoid moving
  `PreparedFunctionLookups`, non-call lookup structs, or broad prepared lookup
  helper declarations in Step 2.
- Do not change ABI classification, call lowering semantics, preservation
  behavior, or AArch64 register handling.
- Do not replace prepared call lookups with local scans in AArch64 consumers.

## Proof

Inspection-only packet. Used `c4c-clang-tool`/`c4c-clang-tool-ccdb` for
function signatures, direct callers/callees, and type refs, supplemented by
`rg` include and consumer scans. No build required by packet; no test logs
created.
