Status: Active
Source Idea Path: ideas/open/689_memory_address_provenance_import_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Continue Pointer And Provenance Import Isolation

# Current Packet

## Just Finished

Completed Step 3 import-boundary contraction for the memory-owned
global-address-slots side-table map type.

- Removed the `BirFunctionLowerer::GlobalAddressSlots` compatibility alias
  from `src/backend/bir/lir_to_bir/lowering.hpp`.
- Updated the owned lowerer helper signatures, provenance definitions, and
  lowerer member to use `c4c::backend::GlobalAddressSlots` directly.
- Left the memory-owned alias definition in
  `src/backend/bir/lir_to_bir/memory/memory_types.hpp` as the direct backend
  type owner.

## Suggested Next

Continue Step 3 with the next supervisor-selected import-boundary alias family
still re-exported by `BirFunctionLowerer`, one state family at a time. The next
coherent nearby candidate is an addressed/global pointer side-table family if
the supervisor selects it; this packet intentionally did not widen past the
global-address-slots boundary.

## Watchouts

- This packet intentionally only contracted the global-address-slots side-table
  map type alias; it did not change pointer provenance semantics, BIR route
  records, prepared data, target/MIR paths, runtime behavior, tests,
  expectations, unsupported markers, allowlists, or public BIR query APIs.
- `BirFunctionLowerer` no longer re-exports `LocalSlotTypes`,
  `LocalAggregateSlots`, `LocalAggregateSlotMap`, or
  `LocalAggregateGepTarget`, `AggregateArrayExtent`, `LocalSlotAddress`, or
  `LocalAggregateFieldSet`, `GlobalPointerMap`, or
  `GlobalObjectPointerMap`, or `GlobalAddressIntMap`, or
  `GlobalObjectAddressIntMap`, or `GlobalAddressSlots`.
- `rg 'BirFunctionLowerer::GlobalAddressSlots|using GlobalAddressSlots = c4c::backend::GlobalAddressSlots|\\bGlobalAddressSlots\\b' src/backend/bir/lir_to_bir/lowering.hpp src/backend/bir/lir_to_bir/memory/provenance.cpp src/backend/bir/lir_to_bir/memory/memory_types.hpp src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
  returns only the memory-owned alias definition in `memory_types.hpp` plus
  direct `c4c::backend::GlobalAddressSlots` lowerer/provenance uses.
- Do not edit prepared frame/storage policy, target addressing legality, MIR
  memory emission, tests, expectations, unsupported markers, allowlists,
  runtime behavior, or harness policy.
- Do not move public BIR Route 3 authority into private lowering.
- Do not widen `memory_helpers.hpp`; it should remain pure layout/projection
  helper declarations, not a home for stateful lowerer policy.
- Do not move `GlobalAddress`, `GlobalInfo`, `GlobalTypes`, function-symbol
  lookup, or known-global-address authority as part of this cleanup family.
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
