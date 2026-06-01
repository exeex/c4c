Status: Active
Source Idea Path: ideas/open/83_aarch64_local_helper_duplication_tail_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Fold Cast Helper Duplication

# Current Packet

## Just Finished

Step 4 cast owner-local helper cleanup completed. `cast_ops.cpp` no longer
carries manual X/W/S/D spelling arrays in its cast-local
`register_display_name`; it now derives stable local X/W/S/D names from
`abi::register_name` while preserving empty occupied-name behavior for V/Q/Sp
and out-of-range register indexes. Scalar and FP cast storage-plan conversion,
record emission, and unsupported-path behavior were otherwise left unchanged.

## Suggested Next

Next coherent packet: continue with the next supervisor-selected helper cleanup
target only, keeping the slice limited to local duplication removal and
preserving existing lowering policy.

## Watchouts

`RegisterOperand::occupied_registers` stores `std::string_view`, so the cast
cleanup keeps ABI-derived names in stable local static storage before returning
views. Cast intentionally still accepts only X/W/S/D occupied views here; V/Q/Sp
continue to return empty. Existing untracked `review/` artifacts and
`test_baseline.new.log` were left untouched.

## Proof

Passed:
`bash -o pipefail -c "(cmake --build build --target c4c_backend c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(scalar_cast_records|prepared_scalar_cast_records|scalar_record_contract|target_instruction_records|machine_printer|instruction_dispatch)|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result' --output-on-failure) 2>&1 | tee test_after.log"`

Proof log: `test_after.log`.
