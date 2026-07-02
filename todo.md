# Current Packet

Status: Active
Source Idea Path: ideas/open/531_bir_memory_provenance_header_readiness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Memory Provenance Boundaries

## Just Finished

Completed Step 1 of `plan.md` as a mapping-only declaration-boundary packet.
Mapped the BIR memory provenance declaration surface and found the first safe
Step 2 boundary: an aggregator-preserving public
`src/backend/bir/bir_memory_provenance.hpp` support header for only the
`MemoryAccessProvenance` support cluster currently in `bir.hpp` lines
3471-3630.

Mapped declaration clusters:

- Provenance support cluster: `MemoryProvenanceBaseIdentityKind`,
  `MemoryObjectExtentCompleteness`, `MemoryLayoutAuthorityKind`,
  `MemoryRangeVerdict`, `MemoryDynamicArrayRangeVerdict`,
  `MemoryProvenanceBaseIdentity`, `MemoryObjectExtent`, `MemoryByteRange`,
  `make_memory_byte_range`, `MemoryDynamicArrayFacts`,
  `MemoryAccessProvenance`, `prove_memory_dynamic_array_range`, and
  `prove_memory_access_requested_range`.
- Core memory address cluster: `MemoryAddress` immediately depends on the
  provenance cluster but also embeds core BIR identity and operation fields:
  `Value`, `AddressSpace`, `LinkNameId`, `BlockLabelId`, `SlotNameId`, and
  public address fields consumed by BIR instructions and backend/prealloc/MIR
  code. Do not move this cluster under idea 531.
- Static GEP/dynamic-array authority cluster: `GlobalStaticGep*` declarations
  use `TypeKind`, `Value`, `LinkNameId`, `std::optional`, and the memory
  authority/range enums. They are consumers of the provenance support enums,
  not the first safe split target.
- Route3 memory access cluster: `Route3MemoryAccess*` records and functions
  use `MemoryAccessProvenance` through `Route3MemoryAccessRecord::provenance`
  and use `MemoryAddress` for route classification. Keep this cluster in
  `bir.hpp`.
- Lowering-private dynamic-array/object/pointer-value cluster:
  `src/backend/bir/lir_to_bir/memory/memory_types.hpp` defines
  `DynamicLocalPointerArrayAccess`, `DynamicPointerValueArrayAccess`,
  `DynamicGlobalPointerArrayAccess`, aggregate/scalar dynamic global accesses,
  `PointerAddress`, and helper constructors. These include
  `MemoryAccessProvenance`, `MemoryObjectExtent`, and
  `MemoryProvenanceBaseIdentityKind`, but also depend on private lowering
  state and `lir_to_bir.hpp`; do not use them as the public split boundary.

Complete-type/include prerequisites:

- The provenance support cluster requires a complete `bir::Value` because
  `MemoryProvenanceBaseIdentity` and `MemoryDynamicArrayFacts` store `Value`
  by value. A forward declaration is not enough.
- It also needs `LinkNameId`, `SlotNameId`, `kInvalidLinkName`,
  `kInvalidSlotName`, and standard `<cstddef>`, `<cstdint>`, `<limits>`,
  `<string>`, and `<utility>` support.
- Because `Value` is currently defined in `bir.hpp`, the safe Step 2 shape is
  to include the new support header from inside `bir.hpp` after `Value` and
  text-id prerequisites are available, keeping `bir.hpp` as the compatibility
  aggregator.

Unsafe direct include replacements:

- Do not replace route3, lowering, prealloc, object emission, MIR, or tests
  with direct includes of the new support header in Step 2. The observed
  consumers still need `bir.hpp` for complete `MemoryAddress`, `Inst`,
  `Block`, `Function`, `Value`, instruction variants, route records, or
  lowering entry points.
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp` is not a direct include
  candidate yet because it includes `../../lir_to_bir.hpp` and its records are
  tied to private lowering maps and `BirFunctionLowerer` state.

Consumer families mapped:

- Route3 memory: `src/backend/bir/bir_route3_memory.cpp` reads
  `MemoryAddress` and copies `MemoryAddress::provenance` into
  `Route3MemoryAccessRecord`.
- LIR-to-BIR memory lowering:
  `src/backend/bir/lir_to_bir/memory/provenance.cpp`,
  `value_materialization.cpp`, `addressing.cpp`, and `intrinsics.cpp` build
  pointer/global/local provenance, dynamic array ranges, static GEP authority,
  and `MemoryAddress` operands.
- Object/pointer-value consumers: `src/backend/prealloc/addressing.hpp`,
  `src/backend/prealloc/publication_plans.hpp/.cpp`,
  `src/backend/prealloc/intrinsics.cpp`,
  `src/backend/prealloc/inline_asm.cpp`, backend object emission paths, and
  MIR tests consume `MemoryAddress`, `MemoryAccessProvenance`, layout
  authority, and range verdicts through `bir.hpp`.

## Suggested Next

Delegate Step 2 as a behavior-preserving code packet: create public
`src/backend/bir/bir_memory_provenance.hpp`, move only the provenance support
cluster listed above out of `bir.hpp`, include that header from `bir.hpp` at
the current declaration site after `Value` is complete, and make no direct
consumer include replacements. Future proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

## Watchouts

- Keep `bir.hpp` as the compatibility aggregator unless direct include proof is
  available.
- Do not move declarations into private `lir_to_bir/memory/` headers.
- Do not change provenance authority verdicts, storage semantics, enum values,
  record layout, optionality, lookup behavior, or lowering behavior.
- Do not move core `MemoryAddress` usage or edit idea 422 producer behavior
  under this idea.
- The Step 2 header is not standalone for arbitrary consumers unless `Value`
  has already been made available; direct include replacement belongs to Step 3
  only after compile probes prove it.
- Parking is not required for idea 531 yet because an aggregator-only public
  provenance support-header boundary exists. Park only if the supervisor
  requires direct include reduction in Step 2 rather than declaration-surface
  extraction with `bir.hpp` compatibility.

## Proof

Mapping-only/no build. Used clang-backed symbol and type-reference queries plus
targeted include-user searches; no `test_after.log` was produced because no
compile probe was run.
