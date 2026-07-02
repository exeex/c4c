Status: Active
Source Idea Path: ideas/open/541_rv64_object_call_variadic_return_fragment_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract Variadic Resource Helpers

# Current Packet

## Just Finished

Completed Step 3 by moving only the behavior-preserving variadic resource predicates from `object_emission.cpp` into `prepared_call_emit.*`: `rv64_variadic_va_list_overflow_arg_area_field` and `rv64_variadic_helper_free_entry_contract_is_complete`. Existing object-side admission, helper diagnostics, va_start/va_arg materialization diagnostics, fragment materialization, object encoder work, and call ABI side effects remain parked and unchanged.

## Suggested Next

Execute Step 4 by extracting call-frame/prologue/epilogue helpers into `prepared_frame_emit.*` only if the packet explicitly owns `prepared_frame_emit.*`; otherwise request a narrower review/mapping packet for the remaining call-family helper groups.

## Watchouts

- `prepared_call_emit.*` now owns simple call emission, prepared frame-slot address, prior-preserved GPR emission, callee-saved boundary-effect emission, byval stack-copy emission, simple result movement, the narrow object-route call argument source predicates moved in Step 2, and the two variadic resource predicates moved in Step 3. Do not turn it into a broad call-fragment owner.
- Keep `prepared_frame_slot_address_call_argument_offset`, `prepared_sret_memory_return_argument_address_offset`, `PreparedFrameSlotAddressArgumentPublication`, `find_matching_call_argument_value_publication_store_source`, `find_unique_prepared_call_argument_value_publication_fact`, `prepared_frame_slot_address_call_argument_publication`, and `fragment_for_prepared_call` parked object-side for now. They still mix address-materialization lookup, store-source publication facts, sret memory-return checks, payload movement, object fixups, byval stack adjustment, inline-asm routes, and final call/result publication.
- Keep variadic admission, diagnostics, and materialization parked object-side for now: `rv64_variadic_function_admission_diagnostic`, `rv64_variadic_incoming_gpr_publications_diagnostic`, `rv64_variadic_va_start_runtime_state_diagnostic`, `rv64_variadic_va_start_materialization_diagnostic`, `rv64_variadic_va_arg_aggregate_materialization_diagnostic`, `diagnose_unsupported_prepared_variadic_helper_fragment`, `fragment_for_rv64_variadic_incoming_gpr_publications`, `fragment_for_prepared_variadic_va_start`, `fragment_for_prepared_variadic_va_arg_aggregate`, `fragment_for_prepared_variadic_va_end`, and `is_rv64_variadic_va_end_call`. They still own user-facing diagnostic strings, helper admission decisions, operand-home lookup, object encoder emission, or fragment materialization.
- `prepared_frame_emit.*` is the right eventual owner for call-frame/prologue/epilogue helpers: `rv64_call_frame_size`, `rv64_call_frame_ra_offset`, `rv64_is_callee_saved_gpr_register_name`, `rv64_saved_callee_gpr_stack_offset`, `append_rv64_saved_callee_gpr_spills`, `append_rv64_saved_callee_gpr_restores`, `make_rv64_call_frame_prologue_fragment`, `make_rv64_stack_frame_prologue_fragment`, `append_rv64_call_frame_epilogue`, and `append_rv64_stack_frame_epilogue`. Park them until the call predicate extraction compiles cleanly; they depend only on frame-plan data and existing frame emit primitives, so they are a likely Step 4 target.
- Keep return helpers object-side for now: `make_rv64_return_immediate_fragment`, `make_rv64_return_zero_fragment`, `prepared_move_is_before_return_stack_to_register_abi_move`, and `fragment_for_prepared_before_return_stack_to_register_abi_move`. The before-return helper still depends on object traversal event phase, function/value type lookup, stack-slot absolute offsets, and return ABI move semantics.
- Any live split for variadic-only or return-only ownership needs a new compiled owner with CMake/build wiring; do not revive legacy `calls.cpp`, `variadic.cpp`, `prologue.cpp`, or `returns.cpp` as destinations without explicit build ownership. Keep final object module assembly, public ELF entrypoints, and `prepared_function_to_object_function` object-side.

## Proof

Ran the delegated Step 3 proof command into `test_after.log`; build passed and the selected backend/object call and variadic subset passed 12/12:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_byval|dump_riscv64_byval|obj_runtime_rv64_local_arg_call|obj_runtime_rv64_callee_saved_gpr_live_across_call|cli_riscv64_variadic_aggregate_overflow_helper_contract_obj)'
```
