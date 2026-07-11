Status: Active
Source Idea Path: ideas/open/689_memory_address_provenance_import_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Continue Pointer And Provenance Import Isolation

# Current Packet

## Just Finished

Completed Step 3 import-boundary contraction for the memory-owned aggregate
array extent type.

- Removed the `BirFunctionLowerer::AggregateArrayExtent` compatibility alias
  from `src/backend/bir/lir_to_bir/lowering.hpp`.
- Updated lowerer declarations in the owned header to use
  `c4c::backend::AggregateArrayExtent` directly.
- Updated owned memory adapter definitions in
  `src/backend/bir/lir_to_bir/memory/addressing.cpp` and
  `src/backend/bir/lir_to_bir/memory/local_slots.cpp` to use the memory-owned
  `AggregateArrayExtent` declaration directly.

## Suggested Next

Continue Step 3 with the next supervisor-selected adapter-local state alias
family that still routes memory-owned declarations through `BirFunctionLowerer`,
or move to review if no such alias family remains in the selected memory
adapter scope.

## Watchouts

- This packet intentionally only contracted the aggregate array extent type
  alias; it did not change GEP semantics, BIR route records, prepared data,
  target/MIR paths, runtime behavior, tests, expectations, unsupported markers,
  allowlists, or public BIR query APIs.
- `BirFunctionLowerer` no longer re-exports `LocalSlotTypes`,
  `LocalAggregateSlots`, `LocalAggregateSlotMap`, or
  `LocalAggregateGepTarget`, or `AggregateArrayExtent`.
- `rg 'BirFunctionLowerer::AggregateArrayExtent|using AggregateArrayExtent = c4c::backend::AggregateArrayExtent' src/backend/bir/lir_to_bir`
  returns no matches after this packet.
- Do not edit prepared frame/storage policy, target addressing legality, MIR
  memory emission, tests, expectations, unsupported markers, allowlists,
  runtime behavior, or harness policy.
- Do not move public BIR Route 3 authority into private lowering.
- Do not widen `memory_helpers.hpp`; it should remain pure layout/projection
  helper declarations, not a home for stateful lowerer policy.
- Do not move `GlobalAddress`, `GlobalInfo`, `GlobalTypes`, function-symbol
  lookup, or known-global-address authority as part of the first narrowing
  packet; those belong to initializer/global import boundaries, not this
  memory-side-table contraction.
- Do not touch canonical BIR memory route records or query APIs in
  `bir.hpp`/`bir.cpp`; the selected target should compile without any public
  BIR schema or downstream prepared/target/MIR edits.

## Proof

Delegated proof:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|string_authority_guard$)' | tee test_after.log
```

Result: passed. Build completed and 303 selected tests passed with 0 failures,
including `string_authority_guard`.

Proof log path: `test_after.log`.
