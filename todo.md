Status: Active
Source Idea Path: ideas/open/628_fpr_abi_frame_policy_and_placement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace FPR Producer Authority

# Current Packet

## Just Finished

Step 2 traced producer authority and RV64 consumer checks for the idea 628 in-scope FPR ABI/frame bucket. AST-backed lookup was used for the relevant C++ translation units before reading focused regions.

Producer authority:

- Call-plan entry point: `prepare::populate_call_plans(PreparedBirModule&)` in `src/backend/prealloc/call_plans.cpp`.
- FPR call argument producers: `call_argument_value_bank(...)` classifies F32/F64 ABI values as `PreparedRegisterBank::Fpr`; `plan_call_argument_destination(...)` fills `PreparedCallArgumentPlan::destination_register_name`, `destination_register_bank`, `destination_contiguous_width`, `destination_occupied_register_names`, `destination_register_placement`, and `destination_target_register_identity`; `plan_call_argument_source(...)` fills `source_encoding`, `source_value_id`, `source_literal`, `source_register_name`, `source_register_bank`, `source_slot_id`, `source_stack_offset_bytes`, `source_register_placement`, and base/delta fields. Immediate FPR literals are already represented as `source_encoding=Immediate`, `source_literal=<F32/F64>`, `value_bank=Fpr`, destination `Fpr` ABI placement.
- FPR call result producer: `build_call_result_plan(...)` fills `PreparedCallResultPlan::value_bank`, `source_storage_kind`, `source_register_name`, `source_register_bank`, `source_register_placement`, `source_target_register_identity`, `destination_value_id`, `destination_storage_kind`, `destination_register_name`, `destination_register_bank`, `destination_register_placement`, `destination_slot_id`, `destination_stack_offset_bytes`, and `destination_spill_slot_placement`. For FPR register results, existing fields carry `value_bank=Fpr`, source ABI placement such as `fa0`, and destination FPR home such as `ft0`/`fs1`.
- FPR call-boundary preservation producer: `build_call_preserved_values(...)` records live-across-call FPRs in `PreparedCallPreservedValue` using `route=CalleeSavedRegister`, `register_name`, `register_bank=Fpr`, `occupied_register_names`, `register_placement`, `callee_saved_save_index`, and source/destination endpoints. `plan_prepared_call_boundary_effects(...)` publishes `PreparedCallBoundaryEffectPlan` entries for `PreservationHomePopulation` before the call and `PreservationRepublication` after the call.
- FPR frame producer: `populate_frame_plan(PreparedBirModule&)` in `src/backend/prealloc/frame_plan.cpp` builds `PreparedSavedRegister` rows from regalloc callee-saved FPR assignments, using `is_callee_saved_register_assignment(...)`, `callee_saved_span_save_index(...)`, `assignment_register_placement(...)`, and `make_saved_register_slot_placement(...)`. The carrier fields are `PreparedSavedRegister::bank`, `register_name`, `contiguous_width`, `occupied_register_names`, `save_index`, `placement`, and `slot_placement`. Fixed-frame FPR rows get complete slot placement; dynamic frames intentionally publish slot placement only for GPR rows via `should_publish_saved_register_slot_placement(...)`.

RV64 consumer checks:

- Object call consumer: `fragment_for_prepared_call(...)` in `src/backend/mir/riscv/codegen/object_emission.cpp`.
- Current FPR argument consumer admits only register-source FPR arguments: when `destination_register_bank=Fpr`, it requires `value_bank=Fpr`, `source_encoding=Register`, `source_register_bank=Fpr`, `source_value_id`, destination FPR placement with width 1, a prepared source FPR home from `prepared_value_home_for_id(...)`, an ABI destination from `fpr_register_number_for_abi_placement(...)`, and emits `append_rv64_fpr_move(...)`. This is the first Step 1 blocker for immediate/literal FPR arguments because `source_encoding=Immediate` is rejected before materialization.
- Current FPR result consumer already admits register-to-register FPR results: it requires source/destination storage width 1, `value_bank=Fpr`, source and destination banks `Fpr`, source FPR placement width 1, `destination_value_id`, matching prepared destination FPR home via `fpr_register_number_for_home(...)`, and emits `append_rv64_fpr_move(...)`. FPR stack-result policy is not part of this bucket.
- Current preservation consumers are GPR-only in `fragment_for_prepared_call(...)`: before-call and after-call effect loops try `append_rv64_callee_saved_gpr_preservation_effect(...)`, `append_rv64_register_source_stack_slot_preservation_effect(...)`, `append_rv64_stack_slot_source_register_preservation_effect(...)`, and `rv64_noop_stack_slot_preservation_effect(...)`. `append_rv64_callee_saved_gpr_preservation_effect(...)` hard-requires both endpoint banks to be GPR, so callee-saved FPR-to-FPR preservation effects fail closed. The stack-slot preservation helpers also require GPR target identities, so they are not an FPR substitute.
- Frame prologue/epilogue fixed-frame FPR consumers already exist in `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`: `rv64_prepared_saved_callee_fpr_stack_offset(...)`, `append_rv64_prepared_saved_callee_fpr_spills(...)`, and `append_rv64_prepared_saved_callee_fpr_restores(...)` validate bank `Fpr`, callee-saved `fs*` spelling, width 1, matching occupied names, callee-saved placement, complete matching `slot_placement`, 8-byte size/align, fixed location, and in-frame offset. `rv64_prepared_object_stack_frame_size(...)` still restricts dynamic-frame admission to saved GPR placement only.

Existing carriers are sufficient for the selected fixed-frame FPR bucket. No producer code is required before consumer work. The missing authority boundary is RV64 consumption: FPR immediate/literal call arguments and FPR callee-saved call-boundary preservation effects are rejected even though the prepared call/frame facts are explicit.

Fail-closed conditions for the next code packet:

- FPR immediate/literal arguments must fail closed unless `value_bank=Fpr`, `destination_register_bank=Fpr`, destination placement is explicit FPR ABI width 1, `source_encoding=Immediate`, `source_literal` exists and has an F32/F64 type matching the call argument type, and materialization uses an explicit scratch GPR/FPR path.
- FPR register-source arguments must keep the existing prepared-home checks; do not infer from `fa*` spelling alone.
- FPR results must keep the existing source/destination placement and prepared-home checks; do not extend to FPR stack results in this idea.
- FPR preservation effects must fail closed unless effect kind/phase/classification match, route is `CalleeSavedRegister`, both endpoints are `Register`, both banks are `Fpr`, names/target identities map to RV64 FPRs, width is 1, occupied names are coherent, and source/destination value ids/names match.
- Frame save/restore must continue requiring complete `PreparedSavedRegisterSlotPlacement`; dynamic-frame FPR saved rows remain out of scope unless a separate plan changes dynamic FPR slot publication and admission.

## Suggested Next

Execute the smallest code-changing packet as Step 3/4 combined or Step 3 if the supervisor wants coverage first: add focused backend coverage proving the existing prepared FPR facts, then add narrow RV64 consumer admission in `fragment_for_prepared_call(...)` for (1) F32/F64 immediate/literal call arguments into prepared FPR ABI destinations and (2) FPR callee-saved preservation effects before/after calls. Reuse existing `append_rv64_gpr_to_fpr_move(...)`, `append_rv64_fpr_move(...)`, and FPR register-number helpers; add fail-closed tests for missing/mismatched bank, missing placement, wrong width, absent source literal/home, and GPR/FPR bank confusion.

## Watchouts

Do not collapse this into scalar GPR call lowering with `fa` spellings. Keep dynamic FPR frame-slot publication, FPR stack results, FPR stack-slot arguments, `unsafe-fp-assoc-1.c` fan-in, floating comparisons/min/max/casts, local/global memory repair, runtime mismatch triage, variadic/library policy, unsupported markers, allowlists, timeouts, and `f128` outside idea 628.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: passed, 347/347 backend tests. Proof log: `test_after.log`.
