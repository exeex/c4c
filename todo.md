Status: Active
Source Idea Path: ideas/open/83_aarch64_local_helper_duplication_tail_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fold Atomics Helper Duplication

# Current Packet

## Just Finished

Step 3 atomics owner-local helper cleanup completed. `atomics.cpp` no longer
defines local `scalar_fp_register_view(bir::TypeKind)` or
`scalar_storage_register_view(bir::TypeKind)` helpers; its prepared atomic
register operand path now uses the existing AArch64-local
`scalar_storage_register_view` declared by `alu.hpp`. Atomic record semantics,
ordering checks, scratch policy, diagnostics, and loop records were otherwise
left unchanged.

## Suggested Next

Next coherent packet: continue with the next supervisor-selected tail-cleanup
target only, keeping the slice limited to local helper duplication removal and
preserving existing lowering policy.

## Watchouts

The shared `scalar_storage_register_view` keeps the same integer-first and
F32/F64 fallback behavior the atomics-local copy had. Existing untracked
`review/` artifacts and `test_baseline.new.log` were left untouched.

## Proof

Passed:
`bash -o pipefail -c "(cmake --build build --target c4c_backend c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)' --output-on-failure) 2>&1 | tee test_after.log"`

Proof log: `test_after.log`.
