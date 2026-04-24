Status: Active
Source Idea Path: ideas/open/02_bir-memory-helper-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Normalize Byte-Storage Reinterpretation Checks

# Current Packet

## Just Finished

Completed `Step 4: Normalize Byte-Storage Reinterpretation Checks`.
Moved the common byte-storage reinterpretation predicate behind the pure
`memory/memory_helpers.hpp` helper surface as
`can_reinterpret_byte_storage_as_type(...)` and reused it from the existing GEP
walkers in `memory/addressing.cpp` and `memory/local_slots.cpp`.

The shared helper owns only layout checks for unsigned-byte storage views and
target type size compatibility. GEP fallback policy, provenance updates, alias
map mutation, and caller-specific traversal behavior remain in the existing
callers.

## Suggested Next

Execute `Step 5: Validate Helper Boundaries`.
Run the structural checks for helper boundaries and the delegated backend proof
before handing the plan back for close handling.

## Watchouts

- Do not create additional `.hpp` files.
- Do not split `coordinator.cpp`.
- Keep `BirFunctionLowerer` as the memory state owner.
- Keep shared helpers pure and argument-driven behind `memory_helpers.hpp`.
- Preserve behavior; do not rewrite expectations as proof.
- The byte-storage helper currently preserves existing whole-view semantics by
  accepting only `target_byte_offset == 0` and equal storage/target sizes.
- Do not broaden offset or partial-view behavior during validation.

## Proof

Proof command:
`cmake --build --preset default --target c4c_backend && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed; 97 backend tests passed, 0 failed, 12 disabled not run.
`git diff --check` also passed.
Proof log: `test_after.log`.
