Status: Active
Source Idea Path: ideas/open/134_shared_select_chain_same_block_dependency_queries.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The First Shared Prepared Query

# Current Packet

## Just Finished

Step 2 added `PreparedSameBlockValueMaterializationQuery` in
`src/backend/prealloc/prepared_lookups.hpp` with query-based overloads for
same-block scalar producer lookup and same-block integer-constant evaluation in
`src/backend/prealloc/prepared_lookups.cpp`.

`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` now builds
one shared prepared same-block value materialization query from the lowering
context and routes its same-block scalar producer and integer constant adapters
through the shared facade. AArch64 publication order, register selection,
scratch hazard checks, and select-chain materialization logic were left
unchanged.

Follow-up guard preservation applied: the local scalar-producer adapter now
retains its prior `context.function.control_flow == nullptr` fail-closed guard
before constructing the shared query. The integer-constant adapter still uses
the shared query builder without that additional guard, preserving its Step 2
behavior.

## Suggested Next

Step 3 should extend the shared prepared-query pattern to the next planned
dependency query family without moving target-specific emission, hazard,
scratch, or materialization-order policy out of AArch64 codegen.

## Watchouts

- Do not add named-case select-chain or direct-global shortcuts.
- Do not move AArch64 emission, hazard, scratch, or materialization-order
  policy into shared code.
- Do not weaken test expectations or mark supported paths unsupported.
- The new facade currently wraps only same-block value materialization facts;
  select-chain direct-global query consolidation remains separate.
- `dispatch_value_materialization.cpp` still keeps thin local callback wrappers
  because `value_publication_may_write_scratch_register` uses AArch64-local
  function pointer callbacks.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -R
'^backend_' --output-on-failure) > test_after.log 2>&1`.

Result: passed. `test_after.log` reports `100% tests passed, 0 tests failed out
of 179`.
