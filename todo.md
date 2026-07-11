Status: Active
Source Idea Path: ideas/open/689_memory_address_provenance_import_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Continue Pointer And Provenance Import Isolation

# Current Packet

## Just Finished

Completed Step 3 import-boundary contraction for the memory-owned
global-object-address-int side-table map type.

- Removed the `BirFunctionLowerer::GlobalObjectAddressIntMap` compatibility alias
  from `src/backend/bir/lir_to_bir/lowering.hpp`.
- Updated the owned lowerer member to use
  `c4c::backend::GlobalObjectAddressIntMap` directly.
- Left the memory-owned alias definition in
  `src/backend/bir/lir_to_bir/memory/memory_types.hpp` as the direct backend
  type owner.

## Suggested Next

Continue Step 3 with the next supervisor-selected import-boundary alias family
still re-exported by `BirFunctionLowerer`, one state family at a time. The
remaining visible candidate in this nearby memory/provenance cluster is global
address slots, which should stay supervisor-selected because this packet
intentionally did not widen past the global-object-address-int side-table
boundary.

## Watchouts

- This packet intentionally only contracted the global-object-address-int side-table
  map type alias; it did not change pointer provenance semantics, BIR route
  records, prepared data, target/MIR paths, runtime behavior, tests,
  expectations, unsupported markers, allowlists, or public BIR query APIs.
- `BirFunctionLowerer` no longer re-exports `LocalSlotTypes`,
  `LocalAggregateSlots`, `LocalAggregateSlotMap`, or
  `LocalAggregateGepTarget`, `AggregateArrayExtent`, `LocalSlotAddress`, or
  `LocalAggregateFieldSet`, `GlobalPointerMap`, or
  `GlobalObjectPointerMap`, or `GlobalAddressIntMap`, or
  `GlobalObjectAddressIntMap`.
- `rg 'BirFunctionLowerer::GlobalObjectAddressIntMap|using GlobalObjectAddressIntMap = c4c::backend::GlobalObjectAddressIntMap|\\bGlobalObjectAddressIntMap\\b' src/backend/bir/lir_to_bir/lowering.hpp src/backend/bir/lir_to_bir/memory/memory_types.hpp src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
  returns only the memory-owned alias definition in `memory_types.hpp` plus
  the direct `c4c::backend::GlobalObjectAddressIntMap` lowerer member.
- Do not edit prepared frame/storage policy, target addressing legality, MIR
  memory emission, tests, expectations, unsupported markers, allowlists,
  runtime behavior, or harness policy.
- Do not move public BIR Route 3 authority into private lowering.
- Do not widen `memory_helpers.hpp`; it should remain pure layout/projection
  helper declarations, not a home for stateful lowerer policy.
- Do not move `GlobalAddress`, `GlobalInfo`, `GlobalTypes`, function-symbol
  lookup, or known-global-address authority as part of this cleanup family.
  Treat `GlobalAddressSlots` as a separate supervisor-selected
  import-boundary family, not as part of this completed object-address-int
  packet.
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
