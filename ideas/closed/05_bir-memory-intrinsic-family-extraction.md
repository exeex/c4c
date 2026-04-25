# BIR Memory Intrinsic Family Extraction

## Intent

Extract the memory intrinsic family out of
`src/backend/bir/lir_to_bir/memory/local_slots.cpp` into its own implementation
file, without changing BIR lowering behavior or adding new headers.

This follows the memory entrypoint/helper boundary work: stateful lowering
remains owned by `BirFunctionLowerer` member functions, while implementation
placement becomes more semantic and `local_slots.cpp` stops absorbing unrelated
families.

## Rationale

`local_slots.cpp` is still the largest memory implementation file and now holds
more than local slot behavior. A clear contained family lives there:

```text
try_lower_immediate_local_memset
try_lower_immediate_local_memcpy
lower_memory_memcpy_inst
lower_memory_memset_inst
try_lower_direct_memory_intrinsic_call
```

These functions implement memcpy/memset and direct runtime memory intrinsic
lowering. They depend on local slot and aggregate helpers, but they are not
themselves the local slot core. Moving this family gives the memory directory a
cleaner implementation shape without changing the `lowering.hpp` index.

## Constraints

- Do not add new `.hpp` files.
- Keep declarations in `src/backend/bir/lir_to_bir/lowering.hpp`.
- Keep these as `BirFunctionLowerer` member functions.
- Do not convert the family to `foo(BirFunctionLowerer& self, ...)`.
- Do not change BIR output, diagnostics, or testcase expectations.
- Do not broaden the work into load/store/GEP redesign.

## Refactoring Steps

1. Create a new implementation file:

```text
src/backend/bir/lir_to_bir/memory/intrinsics.cpp
```

2. Move only the memory intrinsic family from `local_slots.cpp`.
   - Move helper structs that are exclusively used by memcpy/memset lowering.
   - Move anonymous helper lambdas only when they are inside the moved member
     bodies.
   - Keep local-slot-only helpers in `local_slots.cpp`.

3. Preserve member declarations.
   - `lowering.hpp` should continue to list the member functions explicitly.
   - No new include surface should be introduced.

4. Keep dependencies explicit.
   - Include `../lowering.hpp` from the new `.cpp`.
   - Include `memory_helpers.hpp` only if the moved code actually needs it.
   - Prefer existing helper calls over copying logic.

5. Build and prove no behavior change.
   - Build `c4c_codegen`.
   - Run the relevant BIR/LIR-to-BIR narrow tests covering memcpy, memset, and
     runtime memory intrinsic lowering.
   - Do not rewrite expectations.

## Acceptance Criteria

- `memory/intrinsics.cpp` exists and owns the memcpy/memset runtime memory
  intrinsic family.
- `local_slots.cpp` no longer contains the moved intrinsic family.
- No new `.hpp` files are added.
- `lowering.hpp` remains the complete private index for the member functions.
- The moved functions remain `BirFunctionLowerer` members.
- `c4c_codegen` builds.
- Relevant memcpy/memset/BIR memory lowering tests pass with no expectation
  rewrites.

## Non-Goals

- Do not redesign memory intrinsic semantics.
- Do not split load/store or GEP in this idea.
- Do not introduce `MemoryLoweringState`.
- Do not move state ownership out of `BirFunctionLowerer`.
- Do not add per-family headers.

## Closure

Closed after behavior-preserving extraction of the memcpy/memset runtime memory
intrinsic family into `src/backend/bir/lir_to_bir/memory/intrinsics.cpp`.
The accepted implementation keeps declarations in `lowering.hpp`, keeps the
moved definitions as `BirFunctionLowerer` members, adds no headers, and does
not rewrite expectations.

Close proof accepted the full-suite baseline candidate with `3071` passed and
`0` failed before and after.
