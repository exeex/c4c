Status: Active
Source Idea Path: ideas/open/83_aarch64_local_helper_duplication_tail_cleanup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Fold Memory Helper Duplication

# Current Packet

## Just Finished

Step 5 memory owner-local helper cleanup completed. `memory.cpp` no longer
defines local `scalar_fp_register_view(bir::TypeKind)` or
`scalar_storage_register_view(bir::TypeKind)` helpers; memory lowering now uses
the existing AArch64-local type-to-view helpers declared by `float_ops.hpp` and
`alu.hpp`. The memory-local
`scalar_fp_register_view(abi::RegisterReference, bir::TypeKind)` overload
remains local and explicitly calls the shared FP type helper.

## Suggested Next

Next coherent packet: continue with the next supervisor-selected tail-cleanup
target or plan review only, keeping any slice limited to local helper
duplication removal and preserving existing lowering policy.

## Watchouts

The shared helpers preserve memory's prior integer-first storage view behavior
and F32/F64 FP view behavior. The register-reference FP overload must stay
memory-local because it returns a re-viewed `abi::RegisterReference`, not a
type-only `abi::RegisterView`. Existing untracked `review/` artifacts were
left untouched.

## Proof

Passed:
`bash -o pipefail -c "(cmake --build build --target c4c_backend c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract|target_instruction_records|machine_printer|instruction_dispatch)' --output-on-failure) 2>&1 | tee test_after.log"`

Proof log: `test_after.log`.
