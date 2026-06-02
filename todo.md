Status: Active
Source Idea Path: ideas/open/93_aarch64_calls_stack_frame_slot_operand_owner.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route All Stack/Frame-Slot Call Sites Through The Owner

# Current Packet

## Just Finished

Completed plan Step 3: `Route All Stack/Frame-Slot Call Sites Through The Owner`.

Routed the remaining in-scope memory-return frame-slot storage call site through `StackFrameSlotCallOperandOwner::memory_return_storage`. The owner now covers memory-return storage, selected frame-slot sources, sret/local-frame address sources, stack call argument destinations, aggregate stack-copy sources, endpoint stack storage, and prior stack-preserved sources.

Reviewed remaining stack/frame-slot constructions in `calls.cpp` and left these outside the owner by boundary:

- `make_byval_register_lane_prepared_source`: aggregate byval lane publication owns transport chunk/lane validation and is explicitly outside this route.
- `lower_before_call_immediate_binding`: immediate scalar stack argument publication belongs to idea 95 and remains outside this owner.
- `lower_after_call_move` stack-result store: after-call result publication is a non-goal for this route.
- `make_value_stack_move_instruction`, `lower_value_moves`, and `lower_before_return_moves`: generic prepared value/return move helpers, not selected call operand rendering.
- `publish_stack_preserved_call_values`: after-call preserved-value publication/republication producer, explicitly outside the owner.

## Suggested Next

Supervisor can move to Step 4/5 acceptance review. No additional code route is suggested from this packet.

## Watchouts

- Keep the boundary AArch64-local and prepared-fact-consuming.
- Do not move prepared call plan, move-bundle, publication, result, preservation, aggregate transport, or scalar producer authority into the operand owner.
- Do not weaken diagnostics, selection status, assembly output, or test expectations.
- Keep ideas `94` and `95` out of this route unless a supervisor-approved lifecycle switch happens.
- Keep outside the owner: `make_selected_call_argument_source`, `make_call_boundary_move_instruction`, `make_call_boundary_machine_instruction`, `make_outgoing_stack_base_instruction`, `make_aggregate_stack_copy_instruction`, `make_byval_register_lane_stack_publication_instruction`, prior-preservation lookup, register-prior-preservation fallback, scalar immediate publication, f128 carrier handling, and all after-call republication producers.
- `make_sret_memory_return_address_source` and selected frame/local address wrappers now live on the local owner, but callers still decide when those selected facts are applicable.
- `make_stack_call_argument_destination` and `make_aggregate_call_argument_source` now live on the local owner as operand-record constructors; do not expand them into call-argument selection or aggregate transport ownership.
- `memory_return_storage` now lives on the owner as an operand renderer; call-plan presence and call-record construction still stay outside.

## Proof

Ran the exact delegated proof command; build succeeded and all 8 focused tests passed. Proof log: `test_after.log`.

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_memory_operand_contract|backend_aarch64_prepared_memory_operand_records|backend_prepare_frame_stack_call_contract|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor)$' --output-on-failure > test_after.log
```
