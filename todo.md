Status: Active
Source Idea Path: ideas/open/83_aarch64_local_helper_duplication_tail_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fold Globals Helper Duplication

# Current Packet

## Just Finished

Step 2 globals owner-local helper cleanup completed. `globals.cpp` no longer
defines local `scalar_fp_register_view(bir::TypeKind)` or
`scalar_storage_register_view(bir::TypeKind)` helpers; its prepared global
register operand path now uses the existing AArch64-local
`scalar_storage_register_view` declared by `alu.hpp`. Global record semantics,
prepared register conversion, occupied-register publication, and diagnostics
were otherwise left unchanged.

## Suggested Next

Next coherent packet: continue with the next supervisor-selected helper cleanup
target only, keeping the slice limited to local duplication removal and
preserving existing lowering policy.

## Watchouts

The shared `scalar_storage_register_view` keeps the same integer-first and
F32/F64 fallback behavior the globals-local copy had. Existing untracked
`review/` artifacts and `test_baseline.new.log` were left untouched.

## Proof

Passed:
`bash -o pipefail -c "(cmake --build build --target c4c_backend c4cll -- -j2 && ctest --test-dir build -R 'backend_(aarch64_target_instruction_records|codegen_route_aarch64_got_load_global_prepared_memory|codegen_route_aarch64_store_global_stack_publication|codegen_route_aarch64_global_function_pointer_table_selected_indirect_call)' --output-on-failure) 2>&1 | tee test_after.log"`

Proof log: `test_after.log`.
