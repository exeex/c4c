Status: Active
Source Idea Path: ideas/open/569_prepared_move_bundle_ambiguous_multi_source_stack_destination.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Prepared Contract Coverage

# Current Packet

## Just Finished

Step 2 added focused prepared contract coverage for the pinned
non-parallel multi-source stack-destination boundary in
`tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp`.

New focused test:

- `verify_move_bundle_consumer_rejects_select_materialization_stack_destination_sources()`

Coverage encoded:

- The fixture creates a general select-materialization-style
  `BeforeInstruction` move bundle with `authority_kind=None`, attached to a
  select instruction in the prepared consumer block.
- The move bundle has two register-source moves and one stack-to-stack source
  move into the same stack-slot destination value.
- The current contract remains fail-closed:
  `AmbiguousNonParallelMultiSourceStackDestination`, `move_count == 3`, and
  the matching diagnostic category are asserted.
- A nearby case with one register source plus one stack source into the same
  stack destination is asserted `Available`, documenting that the current
  rejection is driven by multiple register-source stack-destination
  authorities.
- The test and fixture names describe the prepared move-bundle shape, not
  `src/20001026-1.c`, and no behavior repair was implemented in this packet.

Evidence:

- `build/agent_state/569_step2_prepared_contract_coverage/coverage_summary.md`
- `build/agent_state/569_step2_prepared_contract_coverage/commands.sh.txt`
- `build/agent_state/569_step2_prepared_contract_coverage/backend_prepared_object_consumer_contract.test_after.log`

## Suggested Next

Execute Step 3 in `plan.md`: repair prepared move-bundle classification so the
new focused select-materialization stack-destination contract can move from
documented fail-closed behavior to coherent prepared authority or an explicit
supported split.

## Watchouts

- Do not treat this as integer div/rem lowering until the prepared classifier boundary is crossed.
- This packet intentionally preserves current fail-closed behavior; it is
  coverage only and does not cross the prepared classifier boundary.
- The new focused coverage records a three-move shape: two register-source
  stack-destination moves plus one stack-to-stack source into the same
  destination value.
- The existing simpler two-register-source ambiguous test remains in place.
- Do not change runtime comparison, expected output, unsupported markers, or allowlists.
- Do not special-case `src/20001026-1.c` by name.
- Keep generic `unsupported_instruction_fragment` rows in idea 570, not this active plan.

## Proof

Step 2 proof:

- `cmake --build build --target c4cll`
- `cmake --build build --target backend_prepared_object_consumer_contract_test`
  was run to compile the changed focused test binary before CTest.
- `ctest --test-dir build -j --output-on-failure -R '^backend_prepared_object_consumer_contract$' > test_after.log`
- `git diff --check -- todo.md tests src`
