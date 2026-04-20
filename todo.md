# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Pointer-Indirect And Residual Address Cleanup
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Completed a Step 3.3 packet in the residual direct local load/store fast paths
inside `render_prepared_local_slot_guard_chain_if_supported`,
`render_prepared_minimal_local_slot_return_if_supported`, and the bounded
compare-against-zero helper. Those x86 consumers now require canonical
prepared frame-slot accesses or prepared stack-object address facts instead of
falling back to `render_prepared_local_address_operand_if_supported` or raw
`layout->offsets.find(...)` slot reconstruction.

## Suggested Next

Continue Step 3.3 by auditing the remaining x86 helper paths that still depend
on `layout.offsets`, `render_prepared_local_slot_memory_operand_if_supported`,
or equivalent local-slot reconstruction outside the prepared-module consumer
families touched in this packet. Keep raw symbol-pointer call-lane setup out
of scope.

## Watchouts

- Do not reopen closed idea 60 value-home or move-bundle work while touching
  address consumers.
- Keep frame size, slot identity, and address provenance in shared prepare,
  not x86-local slot-name or suffix reconstruction.
- This packet only removed the residual direct local load/store fallback sites
  in three bounded x86 helpers; other local-slot helper routes still need a
  separate Step 3.3 audit.
- The bounded multi-defined call-lane pointer-arg consumer near the raw
  `@name` checks remains out of scope unless lifecycle work later adds a
  separate prepared producer contract for `CallInst` pointer arguments.
- Do not treat raw symbol-pointer call setup as a residual address consumer
  just to keep Step 3.3 busy; that would expand the idea instead of executing
  it.
- Do not silently activate idea 59 instruction-selection work from this plan.
- The delegated proof regex produced `test_after.log`, but in this repo it only
  matched `backend_x86_handoff_boundary`; supervisor-side acceptance should
  decide whether a tighter pointer-family subset is needed for broader proof.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(x86_handoff_boundary|codegen_route_x86_64_(local_pointer_deref|nested_member_pointer_array|local_dynamic_member_array|local_dynamic_member_array_store|local_direct_dynamic_member_array_store|local_direct_dynamic_member_array_load|local_direct_dynamic_struct_array_call)_observe_semantic_bir)$' > test_after.log 2>&1`.
Fresh before/after captures on that exact regex report the same two existing
`main`-branch failures in
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
and
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`;
`backend_x86_handoff_boundary` and the other covered pointer/address routes
remain green.
Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
returned `PASS`, then `test_after.log` was rolled forward into `test_before.log`
for the next slice.
