Status: Active
Source Idea Path: ideas/open/83_aarch64_local_helper_duplication_tail_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Fold Cast Helper Duplication

# Current Packet

## Just Finished

Step 4 cast owner-local helper cleanup completed. `cast_ops.cpp` no longer
defines local `scalar_fp_register_view(bir::TypeKind)` or
`scalar_storage_register_view(bir::TypeKind)` helpers; cast lowering now uses
the existing AArch64-local `scalar_storage_register_view` declared by
`alu.hpp` through `cast_ops.hpp`.

## Suggested Next

Next coherent packet: supervisor should decide whether this completes Step 4
or whether another plan-review/lifecycle packet is needed before selecting the
next tail-cleanup target.

## Watchouts

The cast-local `floating_register_view(bir::TypeKind)` helper remains because
it is part of cast print policy and was outside this packet. Existing untracked
`review/` artifacts and `test_baseline.new.log` were left untouched.

## Proof

Passed:
`bash -o pipefail -c "(cmake --build build --target c4c_backend c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(scalar_cast_records|prepared_scalar_cast_records|scalar_record_contract|target_instruction_records|machine_printer|instruction_dispatch)|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result' --output-on-failure) 2>&1 | tee test_after.log"`

Proof log: `test_after.log`.
