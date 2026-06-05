Status: Active
Source Idea Path: ideas/open/110_call_planning_frame_address_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Prove Current Consumer And Producer Reachability

# Current Packet

## Just Finished

Completed the read-only Step 1 reachability audit for
`find_local_frame_address_source()` in `plan.md`.

AST-backed checks:

- `find_local_frame_address_source()` is defined in
  `src/backend/prealloc/call_plans.cpp:1919` and has one direct caller:
  `select_prepared_call_argument_source()` at
  `src/backend/prealloc/call_plans.cpp:2107`.
- `find_latest_frame_slot_materialization()` is defined in
  `src/backend/prealloc/call_plans.cpp:1859` and has the same one direct
  caller, `select_prepared_call_argument_source()`.
- `find_same_block_local_frame_address_derived_source()` is defined in
  `src/backend/prealloc/call_plans.cpp:468`, has one direct caller,
  `select_prepared_call_argument_source()`, and only recognizes same-block
  pointer `BinaryInst` add/sub with an immediate integer delta.

Consumer branch reachability:

- Direct `FrameSlotAddress` fallback:
  `select_prepared_call_argument_source()` first handles
  `PreparedStorageEncodingKind::FrameSlot`. Sret memory-return is separate and
  does not use the fallback. For ordinary stack-homed pointer sources, it first
  asks `find_latest_frame_slot_materialization(..., source_home->value_name,
  false)` and copies the prepared frame-slot materialization into the source
  selection. Only if that explicit fact is absent and
  `call_argument_uses_local_aggregate_frame_address()` is true does it call
  `find_local_frame_address_source()` and recover the slot from stack-object
  names.
- Register-source `LocalFrameAddressMaterialization` fallback:
  when `allows_local_aggregate_address_publication` is true, source encoding is
  `Register`, and the source home is a register, call planning optionally maps
  an immediate same-block derived pointer back to its base via
  `find_same_block_local_frame_address_derived_source()`. It then prefers
  `find_latest_frame_slot_materialization(..., selected_source_value_name,
  true)` and applies the selected materialization byte offset plus the derived
  delta. Only if no prepared fact is found does it call
  `find_local_frame_address_source()` on the selected base name and derive the
  same `LocalFrameAddressMaterialization` fields from the stack slot.
- Computed-address `LocalFrameAddressMaterialization` fallback:
  when local aggregate publication is allowed, source encoding is
  `ComputedAddress`, and the source home is `PointerBasePlusOffset`, call
  planning uses `source_home->pointer_base_value_name` plus
  `pointer_byte_delta`. It first asks
  `find_latest_frame_slot_materialization(..., selected_source_value_name,
  true)` and applies the computed delta to the materialization byte offset.
  Only if that explicit fact is absent does it call
  `find_local_frame_address_source()` on the pointer base name.

Producer evidence for explicit
`PreparedAddressMaterializationKind::FrameSlot` coverage:

- `build_frame_slot_publication_facts()` in
  `src/backend/prealloc/stack_layout/coordinator.cpp:288` indexes every
  function frame slot by the owning stack object's
  `frame_address_value_name`.
- `append_frame_slot_address_materialization()` in
  `src/backend/prealloc/stack_layout/coordinator.cpp:1072` emits a
  `PreparedAddressMaterialization` with kind `FrameSlot`, result value name,
  frame slot id, and concrete byte offset when a named pointer value matches
  that publication map.
- `append_pointer_value_address_materialization()` in
  `src/backend/prealloc/stack_layout/coordinator.cpp:1254` feeds direct named
  pointer values through the frame-slot materialization producer.
- `append_address_materializations()` in
  `src/backend/prealloc/stack_layout/coordinator.cpp:1378` reaches that
  producer for binary pointer operands and result-adjacent pointer values, call
  arguments, and store-local values. The binary path supplies the producer
  route used by same-block derived local frame addresses; the call-argument path
  supplies direct pointer argument materialization.
- `apply_frame_address_publication_hints()` in
  `src/backend/prealloc/stack_layout/analysis.cpp:213` marks named pointer
  values from binary/select/cast/phi/call/load/store shapes as frame-address
  publication candidates, which is the broader stack-layout input that makes
  `frame_address_value_name` facts available to the coordinator.

Existing coverage found:

- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
  checks direct local frame-address source selection at `check_local_frame_address_source_selection_contract()`
  and same-block derived local frame-address selection at
  `check_derived_local_frame_address_source_selection_contract()`.
- The same file has
  `check_missing_local_aggregate_frame_slot_address_source_selection_contract()`,
  which currently demonstrates the direct `FrameSlotAddress` name-derived
  fallback path when a manually assembled prepared module has no explicit
  address materialization.
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
  `check_rooted_pointer_binary_local_slot_activation()` verifies that a rooted
  pointer binary fixture publishes a `PreparedAddressMaterializationKind::FrameSlot`
  fact with the expected frame slot id and byte offset.
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
  also verifies that prepared frame-address materialization authority seeds
  `PointerBasePlusOffset` value homes for frame-address offset carriers.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  covers indexed frame-address lookup selection, latest-before-call behavior,
  future-materialization rejection, non-addressable spill-slot rejection,
  duplicate identical authority acceptance, and conflicting prepared authority
  fail-closed behavior.

Missing or ambiguous coverage:

- No focused dump fixture was found for the call-planning source-selection
  branch proving that direct `FrameSlotAddress` ordinary local aggregate call
  arguments use explicit `PreparedAddressMaterializationKind::FrameSlot`
  authority instead of the name fallback.
- The register-source and computed-address contract checks accept either
  `Register` or `PointerBasePlusOffset` homes, so the existing test names do
  not independently lock each branch to a distinct fixture.
- The current tests prove producer publication for rooted pointer binary
  materialization and lookup fail-closed behavior, but Step 1 found no narrow
  call-planning fixture that removes or withholds explicit materialization and
  proves the register/computed branches fail closed instead of relying on
  `find_local_frame_address_source()`.

## Suggested Next

Proceed to Step 2 by isolating the computed-address pointer-carrier caveat:
trace whether every supported `PointerBasePlusOffset` local aggregate call
argument necessarily has an earlier or same-instruction
`PreparedAddressMaterializationKind::FrameSlot` fact, or identify the missing
producer route.

## Watchouts

- The direct `FrameSlotAddress` fallback is still reachable for manually
  assembled prepared state that lacks address materializations.
- The register and computed branches already prefer explicit materialization,
  but the remaining fallback can still make supported shapes succeed from
  stack-object names if producer coverage is incomplete.
- Existing tests are useful evidence, but they do not yet cleanly separate
  direct, register-derived, and computed-address call-planning authority.

## Proof

Delegated proof command:
`bash -lc 'git diff --check > test_after.log 2>&1'`.

Result: passed; `test_after.log` contains the exact command output.
