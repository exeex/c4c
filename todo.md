Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Publication Authority Leak

# Current Packet

## Just Finished

Step 1 selected one remaining publication authority leak:
`call_argument_allows_local_aggregate_address_publication` in
`src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`.

- Retained read and decision: the helper reads
  `CallInst::arg_abi[argument_index].byval_copy` to reject byval arguments,
  then reads `CallInst::arg_types[argument_index]` or
  `CallInst::args[argument_index].type` to decide whether local aggregate
  address publication is allowed for the scalar call-argument producer path.
- Prepared-fact mapping result: `PreparedCallPlan::arguments[arg_index]`
  already publishes source/destination placement facts such as
  `source_encoding`, `source_value_id`, `source_base_value_name`,
  `source_pointer_byte_delta`, `destination_register_*`, and
  `destination_stack_*`, but it does not publish the semantic eligibility fact
  this helper owns: non-LLVM call argument accepts a pointer value and is not a
  byval copy.
- Selected Step 2 route: replace this local `CallInst` eligibility decision
  with a prepared argument fact for aggregate-address publication eligibility,
  or stop if the supervisor wants no prepared-plan shape change in this
  checkpoint.

## Suggested Next

Delegate Step 2 to add or consume the missing prepared argument authority for
the selected aggregate-address publication gate, then delete the
`CallInst::arg_abi`/`CallInst::arg_types` decision from
`call_argument_allows_local_aggregate_address_publication`.

## Watchouts

- The precise missing prepared fact is a per-argument publication eligibility
  fact equivalent to: this call is not an `llvm.*` helper call, the selected
  argument is pointer-typed for call publication, and the selected ABI argument
  is not a byval copy.
- Existing `PreparedCallArgumentPlan` placement/source fields are not enough by
  themselves because using them to infer pointer/non-byval eligibility would
  move the duplicate decision instead of consuming a prepared authority.
- The selected implementation path will likely need
  `lower_scalar_call_argument_producers` to receive the matching
  `PreparedCallPlan` or a prepared per-argument eligibility view.

## Proof

Selection-only packet; no build was required and no `test_after.log` was
created.

Evidence commands used:

- `rg -n "arg_abi|arg_types|CallInst" src/backend/mir/aarch64/codegen/calls*.cpp src/backend/mir/aarch64/codegen/calls*.hpp src/backend/mir/aarch64/codegen/calls.hpp`
- `rg -n "publication|byval|aggregate|frame address|local frame|CallInst" src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp src/backend/mir/aarch64/codegen/calls_argument_sources.cpp src/backend/mir/aarch64/codegen/calls_dispatch_bridge.hpp`
- `sed -n '60,270p' src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `sed -n '1,220p' src/backend/prealloc/calls.hpp`
- `sed -n '1240,1505p' src/backend/prealloc/call_plans.cpp`
- `rg -n "lower_scalar_call_argument_producers\\(|call_plan.arguments|source_base_value_name|source_pointer_byte_delta|PreparedStorageEncodingKind::ComputedAddress|pointer_base_value_name" src/backend/mir/aarch64/codegen src/backend/prealloc -g '*.cpp' -g '*.hpp'`

Next Step 2 proof command:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
