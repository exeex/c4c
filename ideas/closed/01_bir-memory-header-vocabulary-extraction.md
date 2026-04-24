# BIR Memory Header Vocabulary Extraction

## Intent

Create the small, fixed header vocabulary needed for
`src/backend/bir/lir_to_bir/memory/` without changing BIR lowering behavior.

The current `src/backend/bir/lir_to_bir/lowering.hpp` shape is intentional: it
acts as the LLM-friendly private index for the full LIR-to-BIR lowerer. This
initiative must preserve that role while extracting the memory-specific
vocabulary that is now shared across `addressing.cpp`, `provenance.cpp`,
`local_slots.cpp`, and `coordinator.cpp`.

## Rationale

The memory implementation has already moved into its own directory, but the
shared memory data model and repeated layout/projection helpers still live in
or behind `lowering.hpp`. That makes every memory translation unit depend on
the full lowerer class surface even when the code only needs address structs,
dynamic access structs, or pure aggregate-layout helper logic.

The goal is not interface purity and not a large ownership migration. The goal
is to give the memory directory two clear local vocabulary headers while
keeping `lowering.hpp` as the complete lowerer index.

## Header Budget

This initiative may create exactly these headers:

```text
src/backend/bir/lir_to_bir/memory/memory_types.hpp
src/backend/bir/lir_to_bir/memory/memory_helpers.hpp
```

Do not create any other `.hpp` files in this initiative. In particular, do not
create per-implementation headers such as:

```text
addressing.hpp
provenance.hpp
local_slots.hpp
coordinator.hpp
memory_state.hpp
```

## Design Direction

Keep the layering simple:

```text
lowering.hpp
  private full-index for BirFunctionLowerer, method declarations, and owned state

memory/memory_types.hpp
  memory lowering data vocabulary and aliases

memory/memory_helpers.hpp
  shared pure helpers for layout walking, byte-offset projection, and scalar
  subobject reasoning
```

`lowering.hpp` should include the new memory headers and continue to show the
lowerer fields. The fields may use types from `memory_types.hpp`, but
`BirFunctionLowerer` should still own the state.

## Refactoring Steps

1. Add `memory/memory_types.hpp`.
   - Move memory-specific address structs and aliases out of
     `BirFunctionLowerer` where doing so does not change ownership or behavior.
   - Keep names direct and readable; avoid deep namespace nesting.
   - Let `lowering.hpp` import these types and, if useful for compatibility,
     provide short aliases in `BirFunctionLowerer`.

2. Move these memory data types first.
   - `GlobalPointerSlotKey`
   - `GlobalPointerSlotKeyHash`
   - `PointerAddress`
   - `LocalSlotAddress`
   - `LocalArraySlots`
   - `LocalPointerArrayBase`
   - `DynamicLocalPointerArrayAccess`
   - `DynamicLocalAggregateArrayAccess`
   - `DynamicPointerValueArrayAccess`
   - `DynamicGlobalPointerArrayAccess`
   - `DynamicGlobalAggregateArrayAccess`
   - `DynamicGlobalScalarArrayAccess`
   - `LocalAggregateSlots`
   - `AggregateArrayExtent`
   - `LocalAggregateGepTarget`

3. Move memory map aliases where they are part of the memory data vocabulary.
   - `GlobalPointerMap`
   - `GlobalObjectPointerMap`
   - `GlobalAddressIntMap`
   - `GlobalObjectAddressIntMap`
   - `LocalAddressSlots`
   - `LocalSlotAddressSlots`
   - `LocalSlotPointerValues`
   - `GlobalAddressSlots`
   - `AddressedGlobalPointerSlots`
   - `LocalArraySlotMap`
   - `DynamicLocalPointerArrayMap`
   - `DynamicLocalAggregateArrayMap`
   - `DynamicPointerValueArrayMap`
   - `LocalPointerArrayBaseMap`
   - `DynamicGlobalPointerArrayMap`
   - `DynamicGlobalAggregateArrayMap`
   - `DynamicGlobalScalarArrayMap`
   - `LocalAggregateSlotMap`
   - `LocalAggregateFieldSet`
   - `LocalPointerValueAliasMap`
   - `PointerAddressMap`
   - `PointerAddressIntMap`
   - `GlobalPointerValueSlots`
   - `AddressedGlobalPointerValueSlots`

4. Add `memory/memory_helpers.hpp`.
   - Move shared helper declarations that are reused by multiple memory files
     and are close to pure functions.
   - Prefer helpers that operate on explicit arguments instead of
     `BirFunctionLowerer` state.

5. Move or consolidate the repeated layout/projection helpers.
   - Shared aggregate layout walking.
   - Byte-offset projection through scalar, array, and struct layouts.
   - Scalar leaf/subobject lookup.
   - Byte-storage reinterpretation checks.
   - Repeated aggregate extent lookup.
   - Pointer-array length-at-offset lookup.

6. Preserve implementation behavior.
   - Do not split `coordinator.cpp` in this idea.
   - Do not introduce `MemoryLoweringState`.
   - Do not change load/store/GEP lowering control flow.
   - Do not change BIR output, diagnostics, or testcase expectations.

## Acceptance Criteria

- Exactly two new headers exist under `src/backend/bir/lir_to_bir/memory/`:
  `memory_types.hpp` and `memory_helpers.hpp`.
- No other new `.hpp` files are introduced.
- `lowering.hpp` remains the complete private index for `BirFunctionLowerer`.
- `BirFunctionLowerer` still owns memory lowering state directly.
- Shared memory structs and helper declarations no longer have to be
  rediscovered through unrelated implementation files.
- `addressing.cpp`, `provenance.cpp`, and `local_slots.cpp` reuse the common
  helper vocabulary instead of carrying duplicated layout/subobject logic.
- `c4c_codegen` builds after the extraction.
- Relevant BIR/LIR-to-BIR tests pass with no expectation rewrites.

## Non-Goals

- Do not create a header per `.cpp`.
- Do not move implementation control flow out of `coordinator.cpp`.
- Do not introduce a separate state owner object.
- Do not optimize or redesign memory lowering semantics.
- Do not change current testcase contracts.

## Closure Note

Closed after extracting exactly `memory_types.hpp` and `memory_helpers.hpp`,
keeping `lowering.hpp` as the complete private `BirFunctionLowerer` index and
direct state owner, and preserving backend behavior. Close validation used
`cmake --build --preset default --target c4c_backend && ctest --test-dir build -j --output-on-failure -R '^backend_'`
with regression guard passing at 97 passed before and after, 0 failed.
