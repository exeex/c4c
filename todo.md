# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Route load-local materialization through prepared memory and recovered-source facts

## Just Finished

Completed a Step 4 audit of the remaining ALU unpublished load-local source
consumers.

`src/backend/mir/aarch64/codegen/alu.cpp` still has a non-prepared load-local
consumer route: `make_unpublished_load_local_source_operand` finds a
same-block `LoadLocal` producer by result-name scan, checks intervening stores
through local slot/offset reconstruction, then builds a memory operand from the
value home. Its direct ALU users are `ScalarFallbackOperandSelector::select`,
`append_control_value_to_register`, and the two lhs/rhs override paths in
`lower_scalar_instruction`.

## Suggested Next

Keep Step 4 active. Repair the narrow ALU helper
`make_unpublished_load_local_source_operand` so the unpublished load-local
source operand is admitted only through prepared memory/access authority keyed
to the producer load and current consumer, or fail-closes when no matching
prepared fact exists. Suggested proof subset:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

## Watchouts

The stale helper family is:
`find_same_block_load_local_producer_index`, `store_may_alias_local_load`,
`has_intervening_store_to_local_load_source`, and
`make_unpublished_load_local_source_operand`. `make_prepared_scalar_load_source`
already consumes `PreparedMemoryAccess`, but the unpublished helper still uses
same-block producer discovery and local alias reconstruction to decide whether
that prepared operand may be used. `test_after.log` is not present in this
workspace; the latest backend proof is only recorded in the previous todo state.

## Proof

No build required or run for this audit-only packet. Used `rg` plus
`c4c-clang-tool-ccdb function-callers/function-callees` against
`src/backend/mir/aarch64/codegen/alu.cpp`; no `test_after.log` was generated.
