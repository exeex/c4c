Status: Active
Source Idea Path: ideas/open/93_aarch64_calls_stack_frame_slot_operand_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Operand Boundary And Proof Subset

# Current Packet

## Just Finished

Completed plan Step 1: `Confirm Operand Boundary And Proof Subset`.

In-scope helper map:

- Pure operand/address rendering helpers: `call_boundary_frame_slot_direct_offset_is_encodable`, `materialize_call_boundary_frame_slot_address_lines`, `stack_copy_address`, `endpoint_has_stack_storage`, `make_frame_slot_memory_from_endpoint`, and `make_prior_stack_preserved_value_source`.
- Prepared selection-to-memory adapters that may be routed through the owner but should keep selection authority outside it: `make_selected_frame_slot_source`, `make_sret_memory_return_address_source`, and `make_selected_local_frame_address_source`.
- Stack destination/aggregate source record constructors that may consume owner-rendered operands but must not become selector/publication owners: `make_stack_call_argument_destination` and `make_aggregate_call_argument_source`.
- Preservation authority helpers that must stay outside the pure owner: `find_prior_stack_preserved_value_before_instruction` performs prepared lookup and route filtering; `make_prior_preserved_call_argument_source` owns register-vs-stack prior-preservation selection and register fallback, with only its stack-slot memory construction eligible for owner routing.

Smallest proposed AArch64-local owner boundary: keep a local `calls.cpp` owner first, such as an anonymous-namespace helper group/type, that consumes already-prepared frame-slot endpoints, source selections, memory-return facts, stack destination offsets, source homes, frame-plan/layout facts, and diagnostics sinks only where existing rejection paths require them. The owner may emit `MemoryOperand` records, stack-copy address strings, and frame-slot address materialization lines. It must not choose call arguments/results, prepare move bundles, decide publication/preservation existence, select aggregate transport, build full call-boundary instructions, or mutate diagnostics text.

## Suggested Next

Execute Step 2 by introducing the local stack/frame-slot operand owner inside `src/backend/mir/aarch64/codegen/calls.cpp` without changing behavior. Start by grouping/routing the pure helpers, then adapt the prepared selection-to-memory wrappers to call that owner while leaving selection, publication, preservation lookup, aggregate transport, and full record construction in existing calls-side owners.

## Watchouts

- Keep the boundary AArch64-local and prepared-fact-consuming.
- Do not move prepared call plan, move-bundle, publication, result, preservation, aggregate transport, or scalar producer authority into the operand owner.
- Do not weaken diagnostics, selection status, assembly output, or test expectations.
- Keep ideas `94` and `95` out of this route unless a supervisor-approved lifecycle switch happens.
- Keep outside the owner: `make_selected_call_argument_source`, `make_call_boundary_move_instruction`, `make_call_boundary_machine_instruction`, `make_outgoing_stack_base_instruction`, `make_aggregate_stack_copy_instruction`, `make_byval_register_lane_stack_publication_instruction`, prior-preservation lookup, register-prior-preservation fallback, scalar immediate publication, f128 carrier handling, and all after-call republication producers.
- `make_sret_memory_return_address_source` and selected frame/local address wrappers straddle the boundary: they validate prepared facts and should delegate only the final frame-slot `MemoryOperand` construction.
- `make_stack_call_argument_destination` is stack destination record construction, not source selection; keep destination stack-offset choice in calls-side code and delegate only memory operand assembly if that keeps the API narrower.

## Proof

Audit-only packet; no build/tests required because implementation files were not touched.

AST/tooling used:

- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees ...` for the Step 1 helper set.
- `ctest --test-dir build -N -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_memory_operand_contract|backend_aarch64_prepared_memory_operand_records|backend_prepare_frame_stack_call_contract|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor)$'`

Exact focused proof command for the first implementation packet:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_memory_operand_contract|backend_aarch64_prepared_memory_operand_records|backend_prepare_frame_stack_call_contract|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor)$' --output-on-failure
```

Coverage intent: stack call arguments, frame-slot selected sources, sret/result stack homes, aggregate/local-frame stack sources, prior stack-preserved values, prepared frame/stack contracts, dynamic-stack fixed-slot anchoring, and high-offset AArch64 prepared publication coverage for non-direct frame-slot address materialization.
