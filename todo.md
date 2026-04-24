Status: Active
Source Idea Path: ideas/open/02_bir-memory-helper-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Merge Repeated Aggregate And Pointer-Array Extent Helpers

# Current Packet

## Just Finished

Completed `Step 3: Merge Repeated Aggregate And Pointer-Array Extent Helpers`.
Added a pure `AggregateByteOffsetProjection` helper behind
`memory/memory_helpers.hpp` and reused it in `memory/addressing.cpp` for both
repeated aggregate extent lookup and pointer-array length-at-offset lookup.

The shared helper owns only common array/struct byte-offset descent. Repeated
extent acceptance, struct field-run policy, pointer-element requirements,
fallback behavior, and dynamic result construction remain visible in the
callers.

## Suggested Next

Execute `Step 4: Normalize Byte-Storage Reinterpretation Checks`.
Centralize only the common pure byte-storage reinterpretation checks while
keeping provenance updates, alias map mutation, and fallback behavior in the
existing callers.

## Watchouts

- Do not create additional `.hpp` files.
- Do not split `coordinator.cpp`.
- Keep `BirFunctionLowerer` as the memory state owner.
- Keep shared helpers pure and argument-driven behind `memory_helpers.hpp`.
- Preserve behavior; do not rewrite expectations as proof.
- For Step 4, normalize `can_reinterpret_byte_storage_view(...)`; do not fold
  it into broader GEP helpers or move provenance/map mutation into shared
  helpers.

## Proof

Proof command:
`cmake --build --preset default --target c4c_backend && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed; 97 backend tests passed, 0 failed, 12 disabled not run.
`git diff --check` also passed.
Proof log: `test_after.log`.
