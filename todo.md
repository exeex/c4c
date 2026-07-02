# Current Packet

Status: Active
Source Idea Path: ideas/open/531_bir_memory_provenance_header_readiness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Validate Direct Include Opportunities Or Park

## Just Finished

Completed Step 3 of `plan.md` as a direct-include probe/rejection packet. No
repository include replacements were made; `src/backend/bir/bir_memory_provenance.hpp`
remains an aggregator-only header included by `bir.hpp` after the complete
`bir::Value` prerequisite is available.

Selected consumer probes:

- `src/backend/bir/bir_memory_provenance.hpp` direct parse via
  `c4c-clang-tool list-symbols ... -- --std=c++20 ...`: rejected with
  `unknown type name 'Value'` at the `MemoryProvenanceBaseIdentity::value` and
  `MemoryDynamicArrayFacts::index` fields. The header is not self-contained.
- `src/backend/prealloc/addressing.hpp` direct replacement probe in
  `/tmp/c4c_step3_probe`: replacing only its direct `../bir/bir.hpp` include
  with `../bir/bir_memory_provenance.hpp` parsed, but only because
  `src/backend/prealloc/names.hpp` still transitively included `../bir/bir.hpp`
  first. A masked probe that replaced both the `addressing.hpp` and
  `names.hpp` `bir.hpp` includes failed on `Value`, `NameTables`, `TypeKind`,
  `GlobalAddressMaterializationPolicy`, `AddressSpace`, `LoadGlobalInst`, and
  `Block`. This is not an independent direct include reduction.
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp` replacement probe in
  `/tmp/c4c_step3_probe`: replacing `../../lir_to_bir.hpp` with
  `../../bir_memory_provenance.hpp` failed immediately on missing `Value`, then
  broader BIR model prerequisites such as `TypeKind`; this consumer still needs
  the `lir_to_bir.hpp`/`bir.hpp` aggregator path.
- `src/backend/bir/lir_to_bir/lowering.hpp` replacement probe in
  `/tmp/c4c_step3_probe`: replacing `../lir_to_bir.hpp` with
  `../bir_memory_provenance.hpp` failed on `Value`, `TypeKind`,
  `BirLoweringOptions`, `BirModuleAnalysis`, `Module`, `Inst`, `Block`,
  `Function`, ABI records, and other lowering declarations.
- `src/backend/bir/bir_route3_memory.cpp` replacement probe in
  `/tmp/c4c_step3_probe`: replacing `bir.hpp` with `bir_memory_provenance.hpp`
  failed on `Value` in the provenance header and route/core declarations such
  as `Route3MemoryAccessNodeKind`, `MemoryAddress`,
  `Route3MemoryAccessRecord`, `Route3MemoryAccessIndex`, and `Block`.

Step 3 direct include reduction is parked because selected consumers still
require complete `Value`, `MemoryAddress`, `Inst`, `Block`, `Function`, route,
lowering, or prealloc declaration surfaces that remain outside idea 531.

## Suggested Next

Delegate Step 4 to close or park the memory provenance header route. Summarize
that the only accepted declaration-surface movement is the aggregator-preserving
`bir_memory_provenance.hpp` split, with no direct include replacement until a
separate core BIR declaration split provides `Value` and related model
prerequisites without `bir.hpp`.

## Watchouts

- The new header is aggregator-only after Step 3. It depends on a complete
  `bir::Value` because `MemoryProvenanceBaseIdentity` and
  `MemoryDynamicArrayFacts` store `Value` by value.
- `prealloc/addressing.hpp` has a tempting direct include replacement, but the
  replacement is masked by `prealloc/names.hpp` still including `bir.hpp`; it
  does not independently reduce coupling under idea 531.
- Keep `MemoryAddress`, route3 records, static GEP authority records,
  LIR-to-BIR lowering-private records, prealloc/MIR consumers, and tests on
  `bir.hpp` unless a separate future core-model header route owns the missing
  `Value`/`TypeKind`/`Inst`/`Block`/`Function` prerequisites.
- Do not change provenance authority verdicts, storage semantics, enum values,
  record layout, optionality, lookup behavior, lowering behavior, or idea 422
  producer behavior under this route.

## Proof

Direct-include probe/no build. No repository code/include replacement was made,
so the full backend proof was intentionally not run and `test_after.log` was
not rewritten by this packet.

Evidence used:

- `c4c-clang-tool list-symbols src/backend/bir/bir_memory_provenance.hpp -- --std=c++20 ...`
  returned header declarations plus errors for unknown `Value`.
- `c4c-clang-tool type-refs` on `src/backend/prealloc/addressing.hpp`,
  `src/backend/bir/lir_to_bir/memory/memory_types.hpp`, and
  `src/backend/bir/lir_to_bir/lowering.hpp` confirmed the selected
  `MemoryAccessProvenance` type-reference sites.
- Syntax-only temporary replacement probes used `/usr/bin/c++ -std=gnu++17 -fsyntax-only`
  with the backend compile include flags against `/tmp/c4c_step3_probe`; failing
  probes are recorded above. No root-level probe log was created.
