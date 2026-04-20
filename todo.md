# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Pointer-Indirect And Residual Address Cleanup
Plan Review Counter: 8 / 10
# Current Packet

## Just Finished

Completed a Step 3.3 packet that moved the remaining pointer `lea`
materialization paths in `prepared_local_slot_render.cpp` off local
`PreparedStackLayout::objects` / `frame_slots` scans and onto canonical
prepared value-home data. The guard-chain, minimal-local-slot-return, and
constant-folded pointer fallback paths now resolve stack-backed pointer values
through `PreparedValueLocationFunction` stack-slot homes, and
`prepared_module_emit.cpp` now passes function-location data into the guard
consumer.

## Suggested Next

Continue Step 3.3 by auditing the remaining x86 consumers of
`PreparedValueHomeKind::StackSlot`, especially in
`prepared_module_emit.cpp`, and separate acceptable value-home consumption from
any lingering stack-relative address recovery that should instead consume
prepared frame/address facts. Keep raw symbol-pointer call-lane setup and
idea 60 move-bundle work out of scope.

## Watchouts

- Do not reopen closed idea 60 value-home or move-bundle work while touching
  address consumers.
- Keep frame size, slot identity, and address provenance in shared prepare,
  not x86-local slot-name/object-name or suffix reconstruction.
- This packet replaced the local stack-object/frame-slot scan with prepared
  value-home lookup, which is more authoritative but still distinct from
  direct prepared-memory-access consumption; treat the remaining
  `PreparedValueHomeKind::StackSlot` sites as an audit boundary, not automatic
  follow-on implementation.
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
returned `PASS`, and the accepted proof baseline was rolled forward into
`test_before.log`.
