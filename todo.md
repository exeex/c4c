Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Value Materialization Ownership

# Current Packet

## Just Finished

Step 2 - Extract Value Materialization Ownership moved
`make_load_global_got_materialization_instruction` out of `dispatch.cpp` and
into `dispatch_value_materialization.cpp`, with the narrow owner surface
declared in `dispatch_value_materialization.hpp`.

`dispatch_prepared_block` still recognizes `LoadGlobalInst` and routes GOT
loads through the value-materialization owner. The moved implementation
preserves the register-recording path for register-homed results and the
stack-home select-chain materialization path.

## Suggested Next

Supervisor review/commit this Step 2 extraction slice, then select the next
dispatch responsibility-reduction packet.

## Watchouts

No call-source logic changed. The helper still depends on value-home lookup,
prepared result lookup, global-load labels, select-chain materialization, and
scalar-register recording through the existing value-materialization includes.

## Proof

Passed. Proof log: `test_after.log`.

```bash
cmake --build --preset default && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call' --output-on-failure > test_after.log 2>&1
```
