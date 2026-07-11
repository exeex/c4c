Status: Active
Source Idea Path: ideas/open/689_memory_address_provenance_import_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Continue Pointer And Provenance Import Isolation

# Current Packet

## Just Finished

Completed Step 3 import-boundary contraction for the memory-owned local
aggregate slot side-table family.

- Removed the `BirFunctionLowerer` compatibility aliases for
  `LocalAggregateSlots` and `LocalAggregateSlotMap` from
  `src/backend/bir/lir_to_bir/lowering.hpp`.
- Updated lowerer declarations/member state in the owned header to use
  `c4c::backend::LocalAggregateSlots` and
  `c4c::backend::LocalAggregateSlotMap` directly.
- Updated the remaining owned call and memory helper references in
  `src/backend/bir/lir_to_bir/calling.cpp`,
  `src/backend/bir/lir_to_bir/memory/local_slots.cpp`, and
  `src/backend/bir/lir_to_bir/memory/local_gep.cpp` away from
  `BirFunctionLowerer::LocalAggregateSlots` /
  `BirFunctionLowerer::LocalAggregateSlotMap`.

## Suggested Next

Continue Step 3 with the next supervisor-selected adapter-local state alias
family that still routes memory-owned declarations through `BirFunctionLowerer`.

## Watchouts

- This packet intentionally only contracted the local aggregate slot
  side-table alias family; it did not change local aggregate semantics, BIR
  route records, prepared data, target/MIR paths, runtime behavior, tests,
  expectations, unsupported markers, allowlists, or public BIR query APIs.
- `BirFunctionLowerer` no longer re-exports `LocalSlotTypes`,
  `LocalAggregateSlots`, or `LocalAggregateSlotMap`.
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
