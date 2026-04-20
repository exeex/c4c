# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Pointer-Indirect And Residual Address Cleanup
Plan Review Counter: 6 / 10
# Current Packet

## Just Finished

Completed a Step 3.3 packet that removed the residual raw stack-object-name
fallback from
`render_prepared_constant_folded_single_block_return_if_supported`. Its
`resolve_ptr_value` helper now resolves pointer-valued stack objects through
prepared stack-object lookup keyed by canonical prepared value ids, instead of
reopening stack slots by raw value-name string matching when the folded return
path needs a concrete pointer base.

## Suggested Next

Continue Step 3.3 by auditing the remaining helper surfaces in
`prepared_local_slot_render.cpp` that still expose
`find_prepared_named_stack_object_frame_offset` or
`render_prepared_named_stack_object_address_if_supported`, and determine
whether any remaining x86 callers are residual string-backed address recovery
rather than legitimate prepared-frame consumers. Keep raw symbol-pointer
call-lane setup out of scope.

## Watchouts

- Do not reopen closed idea 60 value-home or move-bundle work while touching
  address consumers.
- Keep frame size, slot identity, and address provenance in shared prepare,
  not x86-local slot-name or suffix reconstruction.
- This packet only replaced the residual raw-name fallback inside the
  constant-folded single-block return helper; remaining helper definitions may
  still be shared with legitimate prepared-frame consumers or may need a later
  Step 3.3 cleanup packet after audit.
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
The exact subset still reports the same two existing `main`-branch failures in
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
and
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`;
`backend_x86_handoff_boundary`,
`backend_codegen_route_x86_64_local_pointer_deref_observe_semantic_bir`,
`backend_codegen_route_x86_64_nested_member_pointer_array_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_local_direct_dynamic_struct_array_call_observe_semantic_bir`
remain green after this packet. Regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
returned `PASS`. `test_after.log` is preserved as the current proof artifact.
