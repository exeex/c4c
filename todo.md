Status: Active
Source Idea Path: ideas/open/83_aarch64_local_helper_duplication_tail_cleanup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Fold Memory Helper Duplication

# Current Packet

## Just Finished

Step 5 memory owner-local cleanup completed. `memory.cpp` no longer carries
the local `register_display_name` spelling table for occupied-register
metadata; its local `occupied_register_views` now derives stable occupied names
from `abi::register_name` while preserving empty occupied-name behavior for
out-of-range register indexes. Storage-plan interpretation, frame-slot and
addressing legality, and memory record emission policy were otherwise left
unchanged.

## Suggested Next

Next coherent packet: continue with the next supervisor-selected Step 5 helper
cleanup target only, keeping the slice limited to local duplication removal and
preserving existing lowering policy.

## Watchouts

`RegisterOperand::occupied_registers` stores `std::string_view`, so the memory
cleanup keeps ABI-derived names in stable local static storage before returning
views. Existing untracked `review/` artifacts and `test_baseline.new.log` were
left untouched.

## Proof

Passed:
`bash -o pipefail -c "(cmake --build build --target c4c_backend c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract|target_instruction_records|machine_printer|instruction_dispatch)' --output-on-failure) 2>&1 | tee test_after.log"`

Proof log: `test_after.log`.
