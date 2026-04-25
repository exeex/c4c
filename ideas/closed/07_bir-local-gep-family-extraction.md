# BIR Local GEP Family Extraction

## Intent

Extract the local GEP family out of
`src/backend/bir/lir_to_bir/memory/local_slots.cpp` into a dedicated memory
implementation file, without changing BIR lowering behavior or adding headers.

This continues the small-step memory cleanup: keep `BirFunctionLowerer` member
ownership and `lowering.hpp` as the complete private index, but stop
`local_slots.cpp` from owning every local-memory family.

## Rationale

After the memory intrinsic extraction, `local_slots.cpp` is still the largest
memory file. A clear family remains there around local GEP resolution:

```text
resolve_local_aggregate_gep_slot
resolve_local_aggregate_pointer_array_slots
resolve_local_aggregate_dynamic_pointer_array_access
resolve_local_aggregate_gep_target
resolve_local_dynamic_aggregate_array_access
build_dynamic_local_aggregate_array_access
resolve_dynamic_local_aggregate_gep_projection
try_lower_dynamic_local_aggregate_gep_projection
try_lower_local_slot_pointer_gep
try_lower_local_array_slot_gep
try_lower_local_pointer_array_base_gep
try_lower_local_pointer_slot_base_gep
```

These functions are local address/projection behavior, not local slot storage
declaration or local load/store materialization. Moving them gives the memory
directory a cleaner implementation map while preserving the current lowerer
index.

## Constraints

- Do not add new `.hpp` files.
- Keep declarations in `src/backend/bir/lir_to_bir/lowering.hpp`.
- Keep these as `BirFunctionLowerer` member functions where they are members
  today.
- Do not convert the family to `foo(BirFunctionLowerer& self, ...)`.
- Do not change BIR output, diagnostics, or testcase expectations.
- Do not broaden the work into load/store redesign.

## Refactoring Steps

1. Create a new implementation file:

```text
src/backend/bir/lir_to_bir/memory/local_gep.cpp
```

2. Move only the local GEP family from `local_slots.cpp`.
   - Move file-private helper structs/functions only when they are exclusively
     used by the local GEP family.
   - Keep alloca, local slot declaration, load/store, and dynamic aggregate
     load/store in `local_slots.cpp` unless a tiny helper must move with the
     GEP family.

3. Preserve the private index.
   - `lowering.hpp` should continue to list the moved member declarations.
   - No new include surface should be introduced.

4. Keep dependencies explicit.
   - Include `../lowering.hpp` from the new `.cpp`.
   - Include `memory_helpers.hpp` only if the moved code needs shared
     projection helpers.
   - Prefer existing helpers over copying logic.

5. Build and prove no behavior change.
   - Build `c4c_codegen`.
   - Run the relevant BIR/LIR-to-BIR GEP and local memory tests.
   - Do not rewrite expectations.

## Acceptance Criteria

- `memory/local_gep.cpp` exists and owns the local GEP family.
- `local_slots.cpp` no longer contains the moved local GEP family.
- No new `.hpp` files are added.
- `lowering.hpp` remains the complete private index for the member functions.
- The moved functions remain `BirFunctionLowerer` members.
- `c4c_codegen` builds.
- Relevant local GEP/local memory tests pass with no expectation rewrites.

## Non-Goals

- Do not redesign local GEP semantics.
- Do not split load/store in this idea.
- Do not introduce `MemoryLoweringState`.
- Do not move state ownership out of `BirFunctionLowerer`.
- Do not add per-family headers.

## Closure

Closed after commit `87267299` extracted the local GEP family into
`src/backend/bir/lir_to_bir/memory/local_gep.cpp`, preserved
`lowering.hpp` as the private member index, avoided new headers and expectation
rewrites, and passed accepted review plus full-suite baseline proof.
