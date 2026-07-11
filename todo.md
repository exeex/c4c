Status: Active
Source Idea Path: ideas/open/689_memory_address_provenance_import_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Continue Pointer And Provenance Import Isolation

# Current Packet

## Just Finished

Completed Step 3 import-boundary contraction for the memory-owned local slot
address type.

- Removed the `BirFunctionLowerer::LocalSlotAddress` compatibility alias
  from `src/backend/bir/lir_to_bir/lowering.hpp`.
- Updated lowerer declarations in the owned header to use
  `c4c::backend::LocalSlotAddress` directly.
- Left adapter-local `.cpp` definitions unchanged because they already use the
  memory-owned type inside the backend namespace and do not depend on the
  removed lowerer alias.

## Suggested Next

Continue Step 3 with the next supervisor-selected import-boundary alias family
still re-exported by `BirFunctionLowerer`, or move to review if the selected
memory adapter scope has no remaining local memory provenance aliases to
contract.

## Watchouts

- This packet intentionally only contracted the local slot address type
  alias; it did not change GEP semantics, BIR route records, prepared data,
  target/MIR paths, runtime behavior, tests, expectations, unsupported markers,
  allowlists, or public BIR query APIs.
- `BirFunctionLowerer` no longer re-exports `LocalSlotTypes`,
  `LocalAggregateSlots`, `LocalAggregateSlotMap`, or
  `LocalAggregateGepTarget`, `AggregateArrayExtent`, or `LocalSlotAddress`.
- `rg 'BirFunctionLowerer::LocalSlotAddress|using LocalSlotAddress = c4c::backend::LocalSlotAddress' src/backend/bir/lir_to_bir`
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
