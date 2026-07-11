Status: Active
Source Idea Path: ideas/open/689_memory_address_provenance_import_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Continue Pointer And Provenance Import Isolation

# Current Packet

## Just Finished

Completed Step 3 import-boundary contraction for the memory-owned global
pointer value slot alias pair.

- Removed the `BirFunctionLowerer` compatibility aliases for
  `GlobalPointerValueSlots` and `AddressedGlobalPointerValueSlots`.
- Updated the affected provenance adapter load/store signatures and lowerer
  member declarations to use the memory-owned declarations directly from
  `src/backend/bir/lir_to_bir/memory/memory_types.hpp`.
- Did not change pointer value side-table behavior, BIR route records,
  prepared data, target/MIR paths, runtime behavior, tests, expectations,
  allowlists, or public BIR query APIs.

## Suggested Next

Continue Step 3 with another adapter-local state alias family the supervisor
selects for the same declaration-boundary cleanup.

## Watchouts

- This packet intentionally did not edit string-authority classification data;
  the declarations were already memory-owned and this slice only removed
  lowerer re-export/import coupling.
- `GlobalPointerValueSlots` and `AddressedGlobalPointerValueSlots` are now
  referenced through the memory-owned `c4c::backend` declarations instead of
  through `BirFunctionLowerer` aliases.
- Keep follow-up work adapter-local to `src/backend/bir/lir_to_bir/memory/`,
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
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|string_authority_guard$)' | tee test_after.log
```

Result: passed. Build completed and 303 selected tests passed with 0 failures,
including `string_authority_guard`.

Proof log path: `test_after.log`.
