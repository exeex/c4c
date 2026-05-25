Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate Helper/API Surface

# Current Packet

## Just Finished

Step 3 - Consolidate Helper/API Surface: checked the Step 2 frame-slot
call-argument boundary narrowing for obsolete helper/API surface and found no
coherent code cleanup left for the selected boundary.

`materialize_missing_frame_slot_call_arguments` is still the dispatch bridge
entry point used by `dispatch.cpp`, its new prepared-move lookup helper is
private to `calls_dispatch_bridge.cpp`, and `find_bir_value_for_prepared_name`
is still live through `materialize_call_boundary_source_to_destination`.
`make_frame_slot_call_argument_source` remains a shared call-emission helper
needed by the bridge fallback, so removing or relocating it would broaden this
packet beyond the selected Step 2 boundary.

## Suggested Next

Proceed to Step 4 - Broader Backend Checkpoint with the supervisor-selected
backend validation scope.

## Watchouts

- AST-backed checks confirmed `find_bir_value_for_prepared_name` has a direct
  caller in `materialize_call_boundary_source_to_destination`.
- Text reference checks confirmed the two bridge entry points remain declared
  in `calls_dispatch_bridge.hpp` and are called only from `dispatch.cpp`.
- No implementation files, tests, or expectation contracts were changed.

## Proof

No build/test run; this packet changed only `todo.md` after verifying that no
coherent Step 3 implementation cleanup remained for the selected boundary.
No `test_after.log` was produced for this todo-only packet.
