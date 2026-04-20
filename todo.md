# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Pointer-Indirect And Residual Address Cleanup
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Completed a Step 3.3 packet that closed the residual named-stack-object
memory fallback in the x86 guard/helper families already consuming prepared
memory accesses. Shared prepare now publishes canonical frame-slot accesses
for local-memory instructions whose authoritative base comes from
`address.base_name` plus `address.byte_offset`, and
`render_prepared_local_slot_guard_chain_if_supported`,
`render_prepared_local_i32_arithmetic_guard_if_supported`, and
`render_prepared_local_i16_arithmetic_guard_if_supported` now require those
prepared accesses instead of reopening
`render_prepared_named_stack_object_memory_operand_if_supported`.

## Suggested Next

Continue Step 3.3 by auditing the remaining pointer/address helpers that still
recover stack-object addresses by name rather than from prepared addressing
data, especially the pointer-valued store/constant-folded routes in
`prepared_local_slot_render.cpp`. Keep raw symbol-pointer call-lane setup out
of scope.

## Watchouts

- Do not reopen closed idea 60 value-home or move-bundle work while touching
  address consumers.
- Keep frame size, slot identity, and address provenance in shared prepare,
  not x86-local slot-name or suffix reconstruction.
- This packet only removed the residual named-stack-object memory fallback in
  three bounded x86 helpers; pointer-valued address recovery by stack-object
  name still remains for a later Step 3.3 packet.
- The bounded multi-defined call-lane pointer-arg consumer near the raw
  `@name` checks remains out of scope unless lifecycle work later adds a
  separate prepared producer contract for `CallInst` pointer arguments.
- Do not treat raw symbol-pointer call setup as a residual address consumer
  just to keep Step 3.3 busy; that would expand the idea instead of executing
  it.
- Do not silently activate idea 59 instruction-selection work from this plan.
- The current proof subset still carries two existing main-branch failures in
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
  and
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`;
  treat those as baseline noise, not new fallout from this packet.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(x86_handoff_boundary|codegen_route_x86_64_(local_pointer_deref|nested_member_pointer_array|local_dynamic_member_array|local_dynamic_member_array_store|local_direct_dynamic_member_array_store|local_direct_dynamic_member_array_load|local_direct_dynamic_struct_array_call)_observe_semantic_bir)$' > test_after.log 2>&1`.
Fresh before/after captures on that exact regex report the same two existing
`main`-branch failures in
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
and
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`;
`backend_x86_handoff_boundary` and the other covered pointer/address routes
remain green after the producer/consumer changes in this packet.
Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
returned `PASS`, then `test_after.log` was rolled forward into `test_before.log`
for the next slice.
