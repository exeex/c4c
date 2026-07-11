Status: Active
Source Idea Path: ideas/open/689_memory_address_provenance_import_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Narrow One Adapter-Local Memory State Boundary

# Current Packet

## Just Finished

Completed Step 2 declaration-owner contraction for the route-local memory
slot/provenance maps.

- Moved the definitions of `LocalSlotTypes`, `LocalPointerSlots`, and
  `LocalIndirectPointerSlotSet` from `lir_to_bir_detail` in
  `src/backend/bir/lir_to_bir/lowering.hpp` into
  `src/backend/bir/lir_to_bir/memory/memory_types.hpp`.
- Preserved the existing
  `BirFunctionLowerer::{LocalSlotTypes,LocalPointerSlots,LocalIndirectPointerSlotSet}`
  compatibility aliases by retargeting them to the memory-owned aliases.
- Updated only the existing `LocalSlotTypes`, `LocalPointerSlots`, and
  `LocalIndirectPointerSlotSet` string-authority classifications to follow the
  moved aliases at
  `src/backend/bir/lir_to_bir/memory/memory_types.hpp`.
- Did not change map key/value types, BIR memory records, prepared data,
  target/MIR paths, tests, expectations, allowlists, or runtime behavior.

## Suggested Next

Continue with the supervisor-selected next Step 3 packet for memory address
provenance import cleanup.

## Watchouts

- The string-authority classification update was allowed only because it
  follows the real declaration-owner move for these three aliases.
- Do not broaden this into unrelated guard metadata churn.
- Keep the work adapter-local to `src/backend/bir/lir_to_bir/memory/`,
  `memory_types.hpp`, `memory_helpers.hpp`, and only necessary supporting
  declarations.
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
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|string_authority_guard$)'
```

Result: passed. Build completed and 303 selected tests passed with 0 failures.

Proof log path: `test_after.log`.
