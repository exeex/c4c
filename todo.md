Status: Active
Source Idea Path: ideas/open/84_aarch64_prepared_consumer_wrapper_contraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Contract Memory-Access Wrappers

# Current Packet

## Just Finished

Step 3 - Contract Memory-Access Wrappers completed the redundant frame-slot
prepared load instruction wrapper contraction. Removed
`make_prepared_frame_slot_load_memory_instruction_record` from `memory.cpp`,
`memory.hpp`, and the aggregate declaration in `instruction.hpp`.

Direct prepared-memory tests now call the surviving
`make_prepared_load_memory_instruction_record(..., nullptr, ...)` helper for the
same no-edge-publication path, preserving the same assertions and diagnostics.
`make_prepared_load_memory_instruction_record` and
`make_prepared_memory_operand_record` semantics remain in place.

## Suggested Next

For the next bounded packet, continue with another supervisor-selected Step 3
memory wrapper from the Step 1 audit, or move to Step 5 consistency proof if the
supervisor considers the memory redundant-wrapper path sufficient.

## Watchouts

- Confirmed no remaining `make_prepared_frame_slot_load_memory_instruction_record`
  references outside this `todo.md` handoff.
- `make_prepared_load_memory_instruction_record` is now the public prepared
  local-load instruction helper where tests need the explicit null
  edge-publication path.
- This packet did not touch plan, source idea, ALU/call files, unrelated tests,
  or transient `review/` files.

## Proof

Supervisor-selected proof command ran successfully and wrote `test_after.log`:

```sh
bash -o pipefail -c '(cmake --build build --target c4c_backend c4cll -- -j2 && ctest --test-dir build -R "backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_got_load_global_prepared_memory|backend_codegen_route_aarch64_store_global_stack_publication|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor" --output-on-failure) 2>&1 | tee test_after.log'
```

Result: build passed; 8/8 selected tests passed.

Also ran:

```sh
git diff --check
```

Result: passed.
