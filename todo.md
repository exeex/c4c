Status: Active
Source Idea Path: ideas/open/83_aarch64_local_helper_duplication_tail_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fold Globals Helper Duplication

# Current Packet

## Just Finished

Step 2 globals owner-local cleanup completed. `globals.cpp` no longer carries
the local `register_display_name` spelling table for occupied-register
metadata; `occupied_register_views` now derives stable occupied names from
`abi::register_name` while preserving the empty list for out-of-range register
indexes. Address materialization record construction and prepared register
operand population were otherwise left unchanged.

## Suggested Next

Next coherent packet: repeat the same owner-local occupied-register spelling
cleanup in `atomics.cpp` only, replacing its local `register_display_name`
table with ABI-backed occupied names without changing atomic scratch-selection
or loop-record policy.

## Watchouts

`RegisterOperand::occupied_registers` stores `std::string_view`, so the globals
cleanup keeps ABI-derived names in stable storage before returning views.
`cast_ops.cpp` remains intentionally narrower and should not be folded with the
globals/atomics/memory occupied-name path without a separate cast-specific
review.

## Proof

Passed:
`bash -o pipefail -c "(cmake --build build --target c4c_backend c4cll -- -j2 && ctest --test-dir build -R 'backend_(aarch64_target_instruction_records|codegen_route_aarch64_got_load_global_prepared_memory|codegen_route_aarch64_store_global_stack_publication|codegen_route_aarch64_global_function_pointer_table_selected_indirect_call)' --output-on-failure) 2>&1 | tee test_after.log"`

Proof log: `test_after.log`.
