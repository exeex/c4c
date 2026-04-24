Status: Active
Source Idea Path: ideas/open/02_bir-memory-helper-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Merge Scalar Leaf And Scalar Subobject Reasoning

# Current Packet

## Just Finished

Completed `Step 2: Merge Scalar Leaf And Scalar Subobject Reasoning`.
Added a pure `ScalarLayoutByteOffsetFacts`/`ScalarLayoutLeafFacts` result
behind `memory/memory_helpers.hpp`, implemented shared scalar layout lookup in
`memory/local_slots.cpp`, and reused it from both raw byte-slice local aggregate
leaf resolution and provenance scalar subobject addressability.

The shared helper is argument-driven and only reports layout facts. Provenance
keeps access-size checks, stored-type handling, byte-wise scalar object
representation policy, and opaque pointer base policy local to
`memory/provenance.cpp`.

## Suggested Next

Execute `Step 3: Merge Repeated Aggregate And Pointer-Array Extent Helpers`.
Extract only the common array/struct byte-offset descent where traversal
semantics match, while keeping repeated-extent acceptance, pointer-element
requirements, ambiguity/fallback behavior, and dynamic result construction in
the callers.

## Watchouts

- Do not create additional `.hpp` files.
- Do not split `coordinator.cpp`.
- Keep `BirFunctionLowerer` as the memory state owner.
- Keep shared helpers pure and argument-driven behind `memory_helpers.hpp`.
- Preserve behavior; do not rewrite expectations as proof.
- For Step 3, `find_repeated_aggregate_extent_at_offset_impl(...)` and
  `find_pointer_array_length_at_offset(...)` have similar descent mechanics but
  different success policy and return shapes; avoid hiding those differences in
  a generic helper.
- For Step 4, normalize `can_reinterpret_byte_storage_view(...)`; do not fold
  it into broader GEP helpers or move provenance/map mutation into shared
  helpers.

## Proof

Proof command:
`cmake --build --preset default --target c4c_backend && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed; 97 backend tests passed, 0 failed, 12 disabled not run.
`git diff --check` also passed.
Proof log: `test_after.log`.
