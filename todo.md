Status: Active
Source Idea Path: ideas/open/629_prepared_return_destination_home_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace Return Producer Authority

# Current Packet

## Just Finished

Step 2 traced the prepared return producer/carrier boundary for the in-scope `return_stack_to_register` before-return move bundles.

Producer boundary:

- `src/backend/prealloc/regalloc/call_moves.cpp:append_return_move_resolution(...)` creates the assigned return move records for named return terminator values. For stack-slot sources returning through a register ABI destination, it calls `append_move_resolution_record(...)` with `destination_kind=FunctionReturnAbi`, `destination_storage_kind=Register`, `destination_abi_index` from the explicit return lane when present, `destination_register_name`, `destination_contiguous_width`, `destination_occupied_register_names`, `destination_register_placement`, `destination_target_register_identity`, `block_index`, `instruction_index=block.insts.size()`, `from_value_id=to_value_id=source->value_id`, `op_kind=Move`, `reason=storage_transfer_reason("return", StackSlot, Register)` (`return_stack_to_register`), and `authority_kind=None`.
- `append_unassigned_return_move_resolution_record(...)` is the sibling producer for unassigned return values; it also targets `FunctionReturnAbi` register destinations and currently publishes `authority_kind=None`, but the Step 1 rows are assigned stack-slot sources, not this unassigned path.

Carrier fields already present:

- `PreparedMoveResolution` carries the function-return ABI destination shape: `destination_kind`, `destination_storage_kind`, `destination_abi_index`, `destination_register_name`, `destination_contiguous_width`, `destination_occupied_register_names`, `destination_register_placement`, `destination_target_register_identity`, `block_index`, `instruction_index`, `from_value_id`, `to_value_id`, `reason`, and `authority_kind`.
- `PreparedMoveBundle` carries `phase=BeforeReturn`, the bundle block/instruction position, the single move, and the bundle-level `authority_kind`.
- `PreparedValueHome` carries the source value identity and source home (`value_id`, `value_name`, `kind=StackSlot`, slot/offset data). The object-route diagnostic's `destination_home_kind=stack_slot` is only the `to_value_id` value home, not an explicit return-ABI destination home.
- `PreparedMoveBundleLookups::before_return_abi_moves_by_source_and_bank`, built by `make_prepared_move_bundle_lookups(...)`, indexes before-return `FunctionReturnAbi` register moves by `(block_index, source_value_id, destination_register_placement.bank)`. `find_prepared_before_return_abi_move_by_source_and_destination_bank(...)` is the consumer lookup for that carrier.

Current consumer checks:

- `prepared_move_is_before_return_stack_to_register_abi_move(...)` accepts only `phase=BeforeReturn`, bundle `authority_kind=None`, `destination_kind=FunctionReturnAbi`, `destination_storage_kind=Register`, `op_kind=Move`, `reason=return_stack_to_register`, no immediate/parallel-copy/temp source, no destination stack offset, width `1`, at most one occupied destination register, and a concrete destination register name/placement/target identity.
- `fragment_for_prepared_before_return_stack_to_register_abi_move(...)` then requires a source `PreparedValueHome` with `kind=StackSlot`, rejects pointer-typed sources, derives the load size from the BIR source type, and emits a load from the source stack slot to the return register.
- The pre-traversal guard in `build_rv64_prepared_text_object_module_with_diagnostics(...)` rejects any `return_stack_to_register` move that is not the accepted stack-slot shape or the separate direct-global pointer return shape, producing the Step 1 `unsupported_move_bundle_target_shape` diagnostic.

First missing authority boundary:

- The producer publishes destination register ABI facts but does not publish a return-specific authority that proves this stack-slot source home is the selected source for the function's return value and that the destination register placement is the prepared function-return ABI destination. `authority_kind=None` leaves RV64 with only raw shape checks plus ABI lookup, so pointer stack-slot returns remain fail-closed instead of being admitted by explicit prepared return destination-home authority.

Required fail-closed conditions for the next code-changing packet:

- Missing or non-stack `PreparedValueHome` for `from_value_id`.
- Missing or mismatched `to_value_id`, `value_name`, BIR return terminator value, or function return association.
- Missing, non-register, wrong-bank, duplicate, or mismatched `FunctionReturnAbi` destination placement/register/target identity.
- Width mismatch between source value type, source stack slot size, destination width, and return ABI storage.
- Missing or non-return move phase, destination kind, destination storage, reason, op kind, block index, instruction index, or return ABI storage.
- Ambiguous duplicate lookup entries in `before_return_abi_moves_by_source_and_bank`.

## Suggested Next

Execute Step 3 as a narrow prepared-layer coverage packet first: add focused tests proving the current `append_return_move_resolution(...)` output and lookup carriers for one stack-slot-to-register function return, including source home, destination register placement/target identity, width, return ABI storage, function-return association, and the currently missing `authority_kind=None` boundary. If the coverage confirms no existing explicit authority carrier beyond the raw move record, the next packet should be a narrow producer publication path that introduces return-specific prepared authority for this family before RV64 consumer admission.

## Watchouts

The existing `plan_prepared_direct_global_return_authority(...)` helper is only for named global pointer constants already homed in a register; it is not the owner for these pointer stack-slot return rows. Do not infer destination homes from the RV64 return convention or final assembly. Keep scalar call/result transport, pointer stack-results, FPR policy, generic move-bundle authority, runtime triage, local/global repair, variadic/library policy, expectation changes, unsupported marker changes, allowlists, timeouts, and accounting outside this idea.

## Proof

Evidence-only trace. No build was required by the delegated packet, and no root proof log was created or modified. Read-only diagnostics used AST-backed symbol inventory plus focused source reads of `src/backend/prealloc/regalloc/call_moves.cpp`, `src/backend/prealloc/regalloc.hpp`, `src/backend/prealloc/value_locations.hpp`, `src/backend/prealloc/prepared_lookups.cpp`, `src/backend/prealloc/publication_plans.cpp`, `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`, `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`, and `src/backend/mir/riscv/codegen/object_emission.cpp`.
