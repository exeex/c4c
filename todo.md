Status: Active
Source Idea Path: ideas/open/541_rv64_object_call_variadic_return_fragment_cleanup.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Review And Close Readiness

# Current Packet

## Just Finished

Completed Step 6 review and close-readiness evaluation for the active RV64 call-family cleanup runbook. The accumulated Step 2-5 work matches the source idea's cleanup scope: narrow call argument source predicates and prior-preserved GPR helper selection moved to `prepared_call_emit.*`; two variadic resource predicates moved to `prepared_call_emit.*`; safe prologue/epilogue frame helpers moved to `prepared_frame_emit.*`; and the narrow before-return stack-to-register ABI-shape predicate moved to `prepared_call_emit.*`.

The runbook is ready for plan-owner closure evaluation. No legacy RV64 owner file was revived as a live destination: `src/backend/CMakeLists.txt` still builds the RV64 prepared helper owners, and the existing RV64 `calls.cpp`, `variadic.cpp`, `prologue.cpp`, and `returns.cpp` files remain reference files outside the build. No tests, expectations, unsupported markers, diagnostics, variadic admission, ABI behavior, runtime behavior, or object byte contract were changed by this review packet or recorded as changed by the completed proof packets.

## Suggested Next

Ask the plan owner to evaluate closure for `ideas/open/541_rv64_object_call_variadic_return_fragment_cleanup.md` using this `todo.md` review record and the Step 2-5 proof records.

## Watchouts

- Residual parked helpers are intentionally out of this runbook's final movement set because they still require object traversal authority, diagnostics, object encoder utilities, fixups, or public object-route ownership.
- `object_emission.cpp` still owns `prepared_function_to_object_function`, final object module assembly, public ELF entrypoints, object-route rejection strings, call-frame admission checks, traversal-time call/result publication, variadic admission and materialization diagnostics, and before-return fragment emission.
- Keep `prepared_frame_slot_address_call_argument_offset`, `prepared_sret_memory_return_argument_address_offset`, `PreparedFrameSlotAddressArgumentPublication`, `find_matching_call_argument_value_publication_store_source`, `find_unique_prepared_call_argument_value_publication_fact`, `prepared_frame_slot_address_call_argument_publication`, and `fragment_for_prepared_call` parked object-side until a future idea can separate address-materialization lookup, publication facts, sret checks, payload movement, object fixups, byval stack adjustment, inline-asm routes, and final call/result publication.
- Keep variadic admission, diagnostics, and materialization parked object-side: `rv64_variadic_function_admission_diagnostic`, `rv64_variadic_incoming_gpr_publications_diagnostic`, `rv64_variadic_va_start_runtime_state_diagnostic`, `rv64_variadic_va_start_materialization_diagnostic`, `rv64_variadic_va_arg_aggregate_materialization_diagnostic`, `diagnose_unsupported_prepared_variadic_helper_fragment`, `fragment_for_rv64_variadic_incoming_gpr_publications`, `fragment_for_prepared_variadic_va_start`, `fragment_for_prepared_variadic_va_arg_aggregate`, `fragment_for_prepared_variadic_va_end`, and `is_rv64_variadic_va_end_call`.
- Keep remaining return helpers object-side: `make_rv64_return_immediate_fragment`, `make_rv64_return_zero_fragment`, and `fragment_for_prepared_before_return_stack_to_register_abi_move`.
- Any future variadic-only, call-publication, or return-only extraction needs a separate source idea or a new compiled owner with explicit CMake/build ownership. Do not use legacy `calls.cpp`, `variadic.cpp`, `prologue.cpp`, or `returns.cpp` as live destinations without that ownership.

## Proof

No new build was run and no `test_after.log` was written for Step 6, per the delegated review-only proof contract. Proof basis is the committed Step 2-5 records in this file:

- Step 2: exact delegated build plus selected backend/object call subset passed 12/12.
- Step 3: exact delegated build plus selected backend/object call and variadic subset passed 12/12.
- Step 4: exact delegated build plus selected backend/object byval, call, callee-saved, and variadic subset passed 12/12.
- Step 5: exact delegated build plus selected backend/object byval, call, callee-saved, variadic, and return-tagged subset passed 12/12.

The Step 2-5 proof command was:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_byval|dump_riscv64_byval|obj_runtime_rv64_local_arg_call|obj_runtime_rv64_callee_saved_gpr_live_across_call|cli_riscv64_variadic_aggregate_overflow_helper_contract_obj)'
```
