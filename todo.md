Status: Active
Source Idea Path: ideas/open/110_call_planning_frame_address_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Contracts And Run Acceptance Proof

# Current Packet

## Just Finished

Completed Step 4 contract tightening for explicit prepared frame-slot
authority on local aggregate call arguments.

Coverage changes:

- `backend_prepare_frame_stack_call_contract_test.cpp` now requires the direct
  local aggregate call-argument route to have a concrete
  `FrameSlot` address materialization at the producing instruction and requires
  the call argument source selection to carry the matching materialization
  block, instruction, frame-slot, and byte-offset facts.
- The same contract now requires the same-block derived local aggregate route
  (`local.aggregate.0 + 4`) to publish and consume the derived frame-slot
  materialization offset, including computed-address base and immediate-delta
  authority when the source encoding is `ComputedAddress`.
- `backend_prepared_lookup_helper_test.cpp` now verifies frame-address lookup
  fails closed when value-id lookup authority is absent and when explicit
  value-id materialization authority is conflicting.
- No `backend_prepare_stack_layout_test.cpp` edit was needed for this packet:
  its existing rooted pointer-binary contract already proves the stack-layout
  producer publishes frame-address materialization facts for the local
  frame-address route.

## Suggested Next

Recommend lifecycle review/closure consideration for Step 4. The focused
backend contracts and delegated acceptance proof are complete; any next packet
should be supervisor or plan-owner driven rather than executor-selected.

## Watchouts

- These contracts intentionally assert prepared data structures directly and do
  not rely on broad dump string matching.
- The immediate computed-address route is explicitly checked when the planner
  chooses `ComputedAddress`; the direct route is allowed to remain register
  sourced as long as it still carries the prepared local frame-address
  materialization selection facts.
- Residual risk: this packet proves supported local aggregate call argument
  routes and lookup fail-closed behavior in the backend subset, but does not add
  broader non-backend coverage.

## Proof

Delegated proof command:
`bash -lc 'git diff --check && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`.

Result: passed; `test_after.log` is the canonical proof log. The build
completed and the backend CTest subset passed 179 of 179 tests.
