Status: Active
Source Idea Path: ideas/open/83_aarch64_local_helper_duplication_tail_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fold Atomics Helper Duplication

# Current Packet

## Just Finished

Step 3 atomics owner-local cleanup completed. `atomics.cpp` no longer carries
the local `register_display_name` spelling table for occupied-register
metadata; its local `occupied_register_views` now derives stable occupied names
from `abi::register_name` while preserving the empty list for out-of-range
register indexes. Atomic scratch-selection, ordering checks, and loop-record
policy were otherwise left unchanged.

## Suggested Next

Next coherent packet: continue the owner-local helper cleanup in the next
supervisor-selected file only, keeping the slice limited to local occupied-name
spelling replacement and preserving existing lowering policy.

## Watchouts

`RegisterOperand::occupied_registers` stores `std::string_view`, so the atomics
cleanup keeps ABI-derived names in stable local static storage before returning
views. Existing untracked `review/` artifacts were left untouched.

## Proof

Passed:
`bash -o pipefail -c "(cmake --build build --target c4c_backend c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)' --output-on-failure) 2>&1 | tee test_after.log"`

Proof log: `test_after.log`.
