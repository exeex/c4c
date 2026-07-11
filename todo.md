Status: Active
Source Idea Path: ideas/open/689_memory_address_provenance_import_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Memory Import Coupling

# Current Packet

## Just Finished

Completed Step 1 inventory for
`ideas/open/689_memory_address_provenance_import_cleanup.md`.

Inspected the memory/provenance import surfaces with `c4c-clang-tools` plus
targeted reads:

- `src/backend/bir/lir_to_bir/memory/memory_types.hpp` owns adapter-local
  side-table records and maps for global/local pointer slots, local slot
  addresses, dynamic local/global arrays, local aggregate slots, pointer
  addresses, pointer-address ints, and pointer-value slots.
- `src/backend/bir/lir_to_bir/lowering.hpp` still defines
  `lir_to_bir_detail::LocalSlotTypes`, `LocalPointerSlots`, and
  `LocalIndirectPointerSlotSet` at lines 104-109, then re-exports them through
  `BirFunctionLowerer` at lines 344-346 and stores them as function-local state
  at lines 1523-1524 and 1536.
- AST type-ref checks confirmed those slot/provenance maps are consumed by
  memory lowering paths such as `local_slots.cpp`, `local_gep.cpp`, and
  `provenance.cpp`; they are import-local scratch keyed by route-local
  spellings, not public BIR Route 3/query authority.
- Public BIR route/query surfaces, memory address payloads, prepared storage,
  target addressing legality, MIR memory emission, tests, expectations, and
  runtime policy were not edited.

## Suggested Next

Execute Step 2 as a declaration-owner contraction for the route-local
slot/provenance maps.

Recommended owned files:

- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `todo.md`

Recommended change:

- Move the definitions of `LocalSlotTypes`, `LocalPointerSlots`, and
  `LocalIndirectPointerSlotSet` out of `lir_to_bir_detail` in `lowering.hpp`
  and into `memory_types.hpp` beside the rest of the memory/provenance
  side-table aliases.
- Preserve the existing `BirFunctionLowerer::{LocalSlotTypes,LocalPointerSlots,LocalIndirectPointerSlotSet}`
  aliases so implementation call sites and public behavior do not change.
- Keep this as a behavior-preserving ownership cleanup only; do not change map
  keys, value types, publication behavior, BIR memory records, diagnostics, or
  emitted instructions.

## Watchouts

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

No validation run required for this inventory-only packet.

Recommended proof command for the next implementation packet:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|string_authority_guard$)'
```

Expected proof log path for that implementation packet: `test_after.log`.
