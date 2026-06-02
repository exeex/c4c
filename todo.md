Status: Active
Source Idea Path: ideas/open/93_aarch64_calls_stack_frame_slot_operand_owner.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce The Operand Owner

# Current Packet

## Just Finished

Completed plan Step 2: `Introduce The Operand Owner`.

Introduced the AArch64-local `StackFrameSlotCallOperandOwner` boundary inside `src/backend/mir/aarch64/codegen/calls.cpp` and routed the stack/frame-slot operand rendering helpers through it without changing behavior. The owner now contains the frame-slot offset/address rendering helpers, stack-copy address rendering, selected frame-slot/local-frame/sret memory operand construction, stack call destination and aggregate source operand construction, endpoint-to-frame-slot memory construction, and prior stack-preserved memory source construction.

Selection and publication authority remains outside the owner. `make_selected_call_argument_source` still switches on prepared source-selection kind, `make_prior_preserved_call_argument_source` still owns register-vs-stack prior-preservation choice and register fallback, and call-boundary instruction construction/move ordering remain in the existing calls-side code.

## Suggested Next

Execute Step 3 by auditing the remaining stack/frame-slot call sites against `StackFrameSlotCallOperandOwner` and documenting any intentionally outside call sites. Keep the packet behavior-preserving and local to `calls.cpp` plus `todo.md` unless a missing route requires supervisor review.

## Watchouts

- Keep the boundary AArch64-local and prepared-fact-consuming.
- Do not move prepared call plan, move-bundle, publication, result, preservation, aggregate transport, or scalar producer authority into the operand owner.
- Do not weaken diagnostics, selection status, assembly output, or test expectations.
- Keep ideas `94` and `95` out of this route unless a supervisor-approved lifecycle switch happens.
- Keep outside the owner: `make_selected_call_argument_source`, `make_call_boundary_move_instruction`, `make_call_boundary_machine_instruction`, `make_outgoing_stack_base_instruction`, `make_aggregate_stack_copy_instruction`, `make_byval_register_lane_stack_publication_instruction`, prior-preservation lookup, register-prior-preservation fallback, scalar immediate publication, f128 carrier handling, and all after-call republication producers.
- `make_sret_memory_return_address_source` and selected frame/local address wrappers now live on the local owner, but callers still decide when those selected facts are applicable.
- `make_stack_call_argument_destination` and `make_aggregate_call_argument_source` now live on the local owner as operand-record constructors; do not expand them into call-argument selection or aggregate transport ownership.

## Proof

Ran the exact delegated proof command; build succeeded and all 8 focused tests passed. Proof log: `test_after.log`.

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_memory_operand_contract|backend_aarch64_prepared_memory_operand_records|backend_prepare_frame_stack_call_contract|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor)$' --output-on-failure > test_after.log
```
