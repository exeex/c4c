Status: Active
Source Idea Path: ideas/open/541_rv64_object_call_variadic_return_fragment_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Extract Prologue And Epilogue Frame Helpers

# Current Packet

## Just Finished

Completed Step 4 by moving the safe call-frame/prologue/epilogue helper cluster from `object_emission.cpp` into `prepared_frame_emit.*` under the `rv64_prepared_*` API boundary: call-frame size/RA-offset helpers, callee-saved GPR name and stack-offset validation, saved-GPR spill/restore appenders, and call-frame/stack-frame prologue and epilogue fragment builders. Object-side final function assembly still decides whether a call frame is needed and still owns rejection diagnostics, traversal, public object entrypoints, and fragment publication order.

## Suggested Next

Execute Step 5 by reviewing return and before-return helpers for a narrow extraction only if their dependencies can be made explicit without changing sret/byval behavior, before-return moves, diagnostics, object traversal, or final object assembly. If the remaining return helpers still require traversal/event-phase authority, park them with rationale and move to review readiness.

## Watchouts

- `prepared_frame_emit.*` now owns the safe frame/prologue/epilogue helper cluster moved in Step 4. The public names intentionally use `rv64_prepared_*` to avoid colliding with private same-purpose helpers still present in `prepared_scalar_emit.cpp`.
- `object_emission.cpp` still owns `diagnose_unsupported_prepared_saved_register_bank`, `rv64_object_stack_frame_size`, call-frame admission checks inside `prepared_function_to_object_function`, final object module assembly, public ELF entrypoints, and object-route rejection strings.
- `prepared_call_emit.*` remains the owner for simple call emission, prepared frame-slot address helpers, prior-preserved GPR emission, callee-saved boundary-effect emission, byval stack-copy emission, simple result movement, the narrow object-route call argument source predicates moved in Step 2, and the two variadic resource predicates moved in Step 3. Do not turn it into a broad call-fragment owner.
- Keep `prepared_frame_slot_address_call_argument_offset`, `prepared_sret_memory_return_argument_address_offset`, `PreparedFrameSlotAddressArgumentPublication`, `find_matching_call_argument_value_publication_store_source`, `find_unique_prepared_call_argument_value_publication_fact`, `prepared_frame_slot_address_call_argument_publication`, and `fragment_for_prepared_call` parked object-side for now. They still mix address-materialization lookup, store-source publication facts, sret memory-return checks, payload movement, object fixups, byval stack adjustment, inline-asm routes, and final call/result publication.
- Keep variadic admission, diagnostics, and materialization parked object-side for now: `rv64_variadic_function_admission_diagnostic`, `rv64_variadic_incoming_gpr_publications_diagnostic`, `rv64_variadic_va_start_runtime_state_diagnostic`, `rv64_variadic_va_start_materialization_diagnostic`, `rv64_variadic_va_arg_aggregate_materialization_diagnostic`, `diagnose_unsupported_prepared_variadic_helper_fragment`, `fragment_for_rv64_variadic_incoming_gpr_publications`, `fragment_for_prepared_variadic_va_start`, `fragment_for_prepared_variadic_va_arg_aggregate`, `fragment_for_prepared_variadic_va_end`, and `is_rv64_variadic_va_end_call`. They still own user-facing diagnostic strings, helper admission decisions, operand-home lookup, object encoder emission, or fragment materialization.
- Keep return helpers object-side for now: `make_rv64_return_immediate_fragment`, `make_rv64_return_zero_fragment`, `prepared_move_is_before_return_stack_to_register_abi_move`, and `fragment_for_prepared_before_return_stack_to_register_abi_move`. The before-return helper still depends on object traversal event phase, function/value type lookup, stack-slot absolute offsets, and return ABI move semantics.
- Any live split for variadic-only or return-only ownership needs a new compiled owner with CMake/build wiring; do not revive legacy `calls.cpp`, `variadic.cpp`, `prologue.cpp`, or `returns.cpp` as destinations without explicit build ownership. Keep final object module assembly, public ELF entrypoints, and `prepared_function_to_object_function` object-side.

## Proof

Ran the delegated Step 4 proof command into `test_after.log`; build passed and the selected backend/object byval, call, callee-saved, and variadic subset passed 12/12:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_byval|dump_riscv64_byval|obj_runtime_rv64_local_arg_call|obj_runtime_rv64_callee_saved_gpr_live_across_call|cli_riscv64_variadic_aggregate_overflow_helper_contract_obj)'
```
