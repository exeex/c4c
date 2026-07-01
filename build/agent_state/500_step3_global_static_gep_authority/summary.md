# 500 Step 3 Global/Static GEP Authority Route

Step 3 attempted to implement the prerequisite
`global_static_gep_authority` certificate surface identified by Step 2.

## Result

Implementation is blocked by an ownership boundary. No C++ source changes were
made for this packet.

## Exact Missing Lower Authority

The certificate must be published at the point where all of these facts are
simultaneously visible:

- the original `LirGepOp` result pointer/value identity;
- the source global/static object and `LinkNameId`;
- the resolved aggregate/array layout path and normalized byte offset;
- dynamic scalar/aggregate/pointer array facts including index, element count,
  stride, and base byte offset;
- function/block/instruction coordinate and operation role;
- the selected direct-vs-relative global address path.

The current owned files do not contain that full fact set. The actual producer
inputs are split through:

- `src/backend/bir/lir_to_bir/memory/addressing.cpp`, especially
  `resolve_global_gep_address`, `resolve_relative_global_gep_address`,
  `resolve_global_dynamic_scalar_array_access`,
  `resolve_global_dynamic_aggregate_array_access`, and the GEP lowering branch
  that updates `global_*` maps.
- `src/backend/bir/lir_to_bir/lowering.hpp`, where `GlobalAddress`,
  dynamic-global access structs, lowerer map state, and helper declarations
  live.

Those files were not owned by this delegated packet.

## Why Not Publish From Current Owned Surfaces

- `bir::Global` alone lacks the producing GEP coordinate, derivation result,
  layout projection path, and dynamic range proof.
- `LoadGlobalInst` / `StoreGlobalInst` byte offsets are consumers and cannot
  prove the GEP that produced a pointer.
- `MemoryAddress` provenance can carry a range verdict for an access, but it
  does not publish a durable GEP-path certificate or coordinate identity.
- `publication_plans.cpp` sees prepared BIR after lowering; it cannot
  reconstruct the LIR GEP path without violating the no raw-shape/final-home
  inference rule.
- `local_gep.cpp` owns local-object path publication only; global GEP authority
  is in the global addressing path.

## Required Next Packet

Route Step 3 to an implementation packet with these additional owned files:

- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`

and likely the same BIR/prealloc/test ownership already listed for Step 3:

- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- focused backend tests

The packet should publish `global_static_gep_authority` records during global
GEP lowering and leave final semantic global/static GEP admission fail-closed
until a later consumer packet consumes those records.

## Boundary Rows

`src/20051104-1.c` remains a string/global-pointer provenance boundary, not a
direct global-object GEP authority candidate.

`src/ieee/copysign2.c` remains a runtime/string intrinsic boundary, not a
direct global-object GEP authority candidate.
