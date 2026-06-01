Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Align Memory-Backed f128 And Variadic Consumers

# Current Packet

## Just Finished

Step 5 first implementation packet completed. `memory.cpp` now exposes shared
prepared frame-slot helpers for constructing a prepared `MemoryOperand` from
carrier stack-slot facts and for materializing non-direct frame-slot addresses
with the correct stack/frame-pointer base. `f128.cpp` consumes those helpers
for memory-backed f128 carrier transport, so it no longer locally rebuilds the
prepared frame-slot `MemoryOperand` or owns frame-slot address materialization
for carrier memory. The f128-local policy remains the 16-byte/q-register
transport, symbol address handling, helper marshal validation, and diagnostics.

## Suggested Next

Next Step 5 packet: inspect whether any remaining variadic address formatting
can consume the shared frame-slot materialization helper without moving ABI
copy policy out of `variadic.cpp`. Keep the packet helper-consumption-only
unless a real prepared-memory authority duplication is found.

## Watchouts

The shared helper is intentionally generic and not tied to f128. It defaults to
`sp` unless a caller passes `uses_frame_pointer_base`; current f128 carrier
records still do not carry frame-plan context. Do not expand the next packet by
changing call-boundary record shape unless the supervisor explicitly delegates
that broader carrier-context work.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract|instruction_dispatch|mir_carrier)|codegen_route_aarch64_(variadic_aggregate_overflow_byte_copy|local_aggregate_address_pointer_copy_publishes_frame_address|pointer_select_aggregate_byte_copy|store_global_stack_publication|dynamic_stack_fixed_slot_uses_fp_anchor)|cli_dump_prepared_bir_(vla_goto_stackrestore_cfg|00204_stdarg_prepared_handoff_aarch64_publication))$'; } > test_after.log 2>&1
```
