Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate The Publication Helper Boundary

# Current Packet

## Just Finished

Step 3 consolidated the selected aggregate-address publication helper boundary.

- Tightened `lower_scalar_call_argument_producers` so callers pass the already
  selected `PreparedCallPlan` plus the BIR argument values needed for emission.
- Removed the helper's internal prepared-plan lookup and retired its full
  `CallInst` parameter; remaining `CallInst` parameters in this bridge are
  emission context for call lowering or indirect-callee materialization.
- Left `calls.hpp` untouched because this selected publication boundary is
  exposed through `calls_dispatch_bridge.hpp`, not the shared calls helper API.

## Suggested Next

Delegate Step 4 broader backend checkpoint validation if the supervisor wants a
milestone proof for the aggregate-address publication route.

## Watchouts

- `lower_scalar_call_argument_producers` still accepts the BIR argument vector
  because scalar producer emission needs source `bir::Value` kinds, names, and
  types; the selected publication decision itself now comes only from the
  prepared argument fact.
- `materialize_indirect_call_callee_to_prepared_register` still accepts
  `CallInst` because it uses the callee value for emission, not selected
  aggregate publication planning.

## Proof

Proof passed:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

`test_after.log` records `100% tests passed, 0 tests failed out of 162`.
