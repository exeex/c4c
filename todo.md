# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.3.3
Current Step Title: Pointer-Indirect Base-Plus-Offset Cleanup
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed Step 3.3.3 by tightening the residual pointer-indirect stack-address
helper in `prepared_local_slot_render.cpp` so pointer-derived stack addresses
now require canonical prepared frame-slot identity through
`PreparedModuleLocalSlotLayout::frame_slot_offsets` instead of falling back to
recorded `PreparedValueHome::offset_bytes`.

## Suggested Next

Audit whether any other Step 3.3 pointer-indirect or base-plus-offset lanes in
`prepared_local_slot_render.cpp` still recover address meaning locally. If no
such lane remains, treat Step 3.3 as exhausted and let the supervisor decide
between broader Step 4 validation and lifecycle closure work rather than
widening into raw symbol pointer call-lane setup or idea 60 move-bundle work.

## Watchouts

- Do not reopen closed idea 60 value-home or move-bundle work while touching
  pointer-indirect cleanup.
- Keep frame size, slot identity, and address provenance in shared prepare,
  not x86-local slot-name/object-name or suffix reconstruction.
- The remaining explicit `PreparedValueHomeKind::StackSlot` sites in
  `prepared_module_emit.cpp` and `prepared_local_slot_render.cpp` stay bounded
  to scalar value-home loads/stores and wrapper-frame sizing from the Step
  3.3.2 audit boundary; do not churn them unless a site starts reconstructing
  provenance again.
- The raw `PreparedValueHome::offset_bytes` fallback for pointer-derived stack
  addresses is now intentionally closed off in `prepared_local_slot_render.cpp`;
  any remaining failures in this family should be fixed by extending prepared
  frame/address facts, not by restoring local offset recovery.
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
returned `PASS`. Proof log path: `test_after.log`.
