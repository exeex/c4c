# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.3.2
Current Step Title: Stack-Home Consumer Audit Boundary
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed Step 3.3.1 by removing the residual frame-size
reconstruction in `prepared_module_emit.cpp` for the covered prepared
return/move-bundle consumers. Stack-slot-backed return helpers now size the
wrapper frame from canonical prepared function frame data
(`PreparedAddressingFunction::frame_size_bytes` with the prepared
`PreparedStackLayout::frame_size_bytes` handoff as fallback) instead of
inflating it from stack-home byte offsets.

## Suggested Next

Continue Step 3.3.2 by auditing the remaining
`PreparedValueHomeKind::StackSlot` consumers in
`prepared_module_emit.cpp` and `prepared_local_slot_render.cpp` to separate
acceptable scalar value-home moves from any leftover stack-relative address
recovery. If another consumer still rebuilds frame/address meaning locally,
move it onto prepared frame/address facts without widening into raw symbol
call-lane setup or idea 60 move-bundle work.

## Watchouts

- Do not reopen closed idea 60 value-home or move-bundle work while touching
  address consumers.
- Keep frame size, slot identity, and address provenance in shared prepare,
  not x86-local slot-name/object-name or suffix reconstruction.
- `prepared_module_emit.cpp` still needs the prepared stack-layout fallback
  when boundary tests mutate stack-home/value-location state without updating
  prepared addressing in lockstep; do not delete that fallback unless the
  handoff contract itself is tightened.
- Treat the remaining `PreparedValueHomeKind::StackSlot` sites as an audit
  boundary, not automatic follow-on implementation; some are acceptable scalar
  value-home moves rather than residual address provenance recovery.
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
